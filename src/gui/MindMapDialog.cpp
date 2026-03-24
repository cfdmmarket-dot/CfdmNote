#include "MindMapDialog.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/EntryAttributes.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QToolBar>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QApplication>
#include <QScreen>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QFile>
#include <QTemporaryFile>
#include <QDir>

// ─── MindMapDialog ────────────────────────────────────────────────────────────

MindMapDialog::MindMapDialog(QSharedPointer<Database> db, QWidget* parent)
    : QDialog(parent), m_db(db)
{
    setWindowTitle(tr("🧠 Mapa Mental — CfdmNote"));
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint
                   | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);
    const QRect scr = QApplication::primaryScreen()->availableGeometry();
    resize(qMin(1300, scr.width() - 40), qMin(840, scr.height() - 40));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Toolbar ───────────────────────────────────────────────────────────────
    auto* bar = new QToolBar();
    bar->setStyleSheet(
        "QToolBar{background:#1a365d;border:none;padding:4px;spacing:6px;}"
        "QToolButton{color:white;padding:4px 12px;border-radius:4px;}"
        "QToolButton:hover{background:#3182ce;}"
        "QCheckBox{color:#bee3f8;margin:0 8px;}"
        "QLabel{color:#bee3f8;}");

    auto* lbl = new QLabel("  🧠 Mapa Mental");
    lbl->setStyleSheet("font-weight:bold; font-size:13px; color:#ebf8ff;");
    bar->addWidget(lbl);
    bar->addSeparator();

    auto* actReload = bar->addAction("↺ Recarregar");
    auto* actZoomIn  = bar->addAction("🔍+");
    auto* actZoomOut = bar->addAction("🔍−");
    auto* actFit     = bar->addAction("⊡ Caber");
    auto* actExport  = bar->addAction("💾 PNG");
    bar->addSeparator();

    auto* actUndo        = bar->addAction("↩ Desfazer");
    auto* actDetail      = bar->addAction("📋 Detalhes");
    actDetail->setCheckable(true);
    actDetail->setChecked(true);
    actDetail->setShortcut(QKeySequence("D"));
    bar->addSeparator();
    auto* actLayoutBoth  = bar->addAction("↔ Lateral");
    auto* actLayoutRight = bar->addAction("→ Direita");
    auto* actLayoutLeft  = bar->addAction("← Esquerda");
    bar->addSeparator();

    m_chkEntries = new QCheckBox(tr("Entradas"));
    m_chkEntries->setChecked(false);
    bar->addWidget(m_chkEntries);

    auto* hint = new QLabel(tr("   Clique no nó = expandir/recolher filhos · Arrastar = mover"));
    hint->setStyleSheet("color:#90cdf4; font-size:11px; font-style:italic;");
    bar->addWidget(hint);
    root->addWidget(bar);

    // ── WebView + painel lateral ──────────────────────────────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setChildrenCollapsible(false);

    m_webView = new QWebEngineView();
    m_webView->setContextMenuPolicy(Qt::NoContextMenu);

    // Intercepta mudanças de título para receber eventos do JS
    connect(m_webView, &QWebEngineView::titleChanged, this,
            [this](const QString& title) {
        if (!title.startsWith("SELECT:")) return;
        const QString json = title.mid(7);
        const QJsonObject obj = QJsonDocument::fromJson(json.toUtf8()).object();
        updateDetail(obj["id"].toString(),
                     obj["topic"].toString(),
                     obj["type"].toString());
    });

    // Injeta o mapa quando a página carregar — delay 300ms para garantir layout do WebView
    connect(m_webView, &QWebEngineView::loadFinished, this,
            [this](bool ok) {
        if (!ok) return;
        m_mapLoaded = true;
        QTimer::singleShot(300, this, [this]() { injectMap(); });
    });

    // Escreve o HTML (com JS/CSS inline) em arquivo temporário e carrega via file://
    // Isso dá ao Chromium uma origin file:// real, evitando restrições do setHtml
    {
        QFile jsFile(":/cfdmnote/mindmap/mind-elixir.js");
        QFile cssFile(":/cfdmnote/mindmap/mind-elixir.css");
        QFile htmlFile(":/cfdmnote/mindmap/mindmap.html");
        jsFile.open(QIODevice::ReadOnly);
        cssFile.open(QIODevice::ReadOnly);
        htmlFile.open(QIODevice::ReadOnly);
        const QString js  = QString::fromUtf8(jsFile.readAll());
        const QString css = QString::fromUtf8(cssFile.readAll());
        QString html = QString::fromUtf8(htmlFile.readAll());
        html.replace("<link rel=\"stylesheet\" href=\"qrc:/cfdmnote/mindmap/mind-elixir.css\">",
                     "<style>" + css + "</style>");
        html.replace("<script src=\"qrc:/cfdmnote/mindmap/mind-elixir.js\"></script>",
                     "<script>" + js + "</script>");

        QTemporaryFile tmp(QDir::tempPath() + "/cfdmnote_mindmap_XXXXXX.html");
        tmp.setAutoRemove(false);
        if (tmp.open()) {
            tmp.write(html.toUtf8());
            tmp.close();
            m_tmpHtmlPath = tmp.fileName();
            m_webView->load(QUrl::fromLocalFile(m_tmpHtmlPath));
        }
    }

    // Remove arquivo temporário quando o dialog fechar
    connect(this, &QDialog::finished, this, [this]() {
        if (!m_tmpHtmlPath.isEmpty())
            QFile::remove(m_tmpHtmlPath);
    });
    splitter->addWidget(m_webView);

    // ── Painel lateral de detalhes ────────────────────────────────────────────
    auto* panel = new QWidget();
    panel->setFixedWidth(235);
    panel->setStyleSheet("QWidget{background:#f0f7ff;} QLabel{color:#1a365d;}");
    auto* pLay = new QVBoxLayout(panel);
    pLay->setContentsMargins(12, 12, 12, 12);
    pLay->setSpacing(8);

    auto* pH = new QLabel("📋 Detalhes");
    pH->setStyleSheet("font-weight:bold; font-size:12px; color:#2c5282; "
                      "border-bottom:2px solid #3182ce; padding-bottom:5px;");
    pLay->addWidget(pH);

    m_detailTitle = new QLabel(tr("Clique em um nó"));
    m_detailTitle->setWordWrap(true);
    m_detailTitle->setStyleSheet("font-size:12px; font-weight:bold; color:#1a365d;");
    pLay->addWidget(m_detailTitle);

    m_detailSub = new QLabel();
    m_detailSub->setWordWrap(true);
    m_detailSub->setStyleSheet("font-size:11px; color:#4a5568; background:#dbeafe; "
                               "border-radius:4px; padding:4px 6px;");
    pLay->addWidget(m_detailSub);

    m_detailBody = new QLabel();
    m_detailBody->setWordWrap(true);
    m_detailBody->setStyleSheet("font-size:11px; color:#1a202c; background:#f7faff; "
                                "border:1px solid #bee3f8; border-radius:4px; padding:6px;");
    pLay->addWidget(m_detailBody);

    m_detailPath = new QLabel();
    m_detailPath->setWordWrap(true);
    m_detailPath->setStyleSheet("font-size:10px; color:#2b6cb0; background:#ebf8ff; "
                                "border:1px solid #90cdf4; border-radius:4px; padding:5px 6px; "
                                "font-family:monospace;");
    m_detailPath->setTextInteractionFlags(Qt::TextSelectableByMouse);
    pLay->addWidget(m_detailPath);
    pLay->addStretch();

    for (const auto& s : {tr("🖱 Scroll = zoom"),
                           tr("🖐 Arraste = mover nó"),
                           tr("Tab = novo filho"),
                           tr("Enter = irmão")}) {
        auto* l = new QLabel(s);
        l->setStyleSheet("font-size:10px; color:#718096; padding:1px 0;");
        pLay->addWidget(l);
    }
    pLay->addSpacing(8);

    auto* btnClose = new QPushButton(tr("Fechar"));
    btnClose->setStyleSheet(
        "background:#e2e8f0; color:#1a365d; border-radius:5px; padding:8px;");
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    pLay->addWidget(btnClose);

    splitter->addWidget(panel);
    splitter->setSizes({width() - 235, 235});
    root->addWidget(splitter, 1);

    // ── Conexões ──────────────────────────────────────────────────────────────
    connect(actReload,  &QAction::triggered, this, [this]() { injectMap(); });
    connect(actZoomIn,  &QAction::triggered, this, [this]() {
        m_webView->page()->runJavaScript("zoomIn();");
    });
    connect(actZoomOut, &QAction::triggered, this, [this]() {
        m_webView->page()->runJavaScript("zoomOut();");
    });
    connect(actFit,     &QAction::triggered, this, [this]() {
        m_webView->page()->runJavaScript("zoomReset();");
    });

    connect(actExport, &QAction::triggered, this, [this]() {
        const QString path = QFileDialog::getSaveFileName(
            this, tr("Exportar Mapa Mental"), QDir::homePath(), tr("PNG (*.png)"));
        if (path.isEmpty()) return;
        m_webView->grab().save(path, "PNG");
        QMessageBox::information(this, tr("Exportado"), tr("Salvo em:\n%1").arg(path));
    });

    connect(actUndo, &QAction::triggered, this, [this]() {
        m_webView->page()->runJavaScript("undoNodeMove();");
    });

    connect(actDetail, &QAction::toggled, this, [this, actDetail](bool on) {
        // Encontra o painel lateral (segundo widget do splitter)
        auto* sp = qobject_cast<QSplitter*>(m_webView->parentWidget());
        if (sp && sp->count() >= 2) {
            sp->widget(1)->setVisible(on);
            actDetail->setText(on ? "📋 Detalhes" : "📋");
        }
    });

    connect(actLayoutBoth,  &QAction::triggered, this, [this]() {
        m_webView->page()->runJavaScript("setLayout(2);");
    });
    connect(actLayoutRight, &QAction::triggered, this, [this]() {
        m_webView->page()->runJavaScript("setLayout(1);");
    });
    connect(actLayoutLeft,  &QAction::triggered, this, [this]() {
        m_webView->page()->runJavaScript("setLayout(0);");
    });

    connect(m_chkEntries, &QCheckBox::toggled, this, [this]() {
        if (m_mapLoaded) injectMap();
    });
}

void MindMapDialog::showMap(QSharedPointer<Database> db, QWidget* parent)
{
    auto* dlg = new MindMapDialog(db, parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

// ─── Construção do JSON para Mind Elixir ─────────────────────────────────────

// Paleta: grpBg/Fg = grupo principal, subBg/Fg = sub-grupo, entBg/Fg = entrada
// grpBorder = contorno grupo, subBorder = contorno sub, entBorder = contorno entrada
static const struct {
    const char* grpBg; const char* grpFg; const char* grpBorder;
    const char* subBg; const char* subFg; const char* subBorder;
    const char* entBg; const char* entFg; const char* entBorder;
} kPalette[] = {
    { "#2b6cb0","#ffffff","#1a365d",  "#bee3f8","#1a365d","#2b6cb0",  "#ebf8ff","#2c5282","#63b3ed" }, // azul
    { "#276749","#ffffff","#1c4532",  "#c6f6d5","#1c4532","#276749",  "#f0fff4","#22543d","#68d391" }, // verde
    { "#553c9a","#ffffff","#322659",  "#e9d8fd","#322659","#553c9a",  "#faf5ff","#44337a","#b794f4" }, // roxo
    { "#c05621","#ffffff","#7b341e",  "#feebc8","#7b341e","#c05621",  "#fffaf0","#7b341e","#f6ad55" }, // laranja
    { "#285e61","#ffffff","#1d4044",  "#b2f5ea","#1d4044","#285e61",  "#e6fffa","#234e52","#4fd1c5" }, // teal
    { "#975a16","#ffffff","#744210",  "#fefcbf","#744210","#975a16",  "#fffff0","#744210","#ecc94b" }, // amarelo
    { "#9b2c2c","#ffffff","#63171b",  "#fed7d7","#63171b","#9b2c2c",  "#fff5f5","#742a2a","#fc8181" }, // vermelho
    { "#434190","#ffffff","#2a4365",  "#c3dafe","#2a4365","#434190",  "#ebf4ff","#2a4365","#76e4f7" }, // índigo
};
static const int kPaletteSize = 8;

QString MindMapDialog::groupToJson(Group* g, int colorIdx, int depth, const QString& prefix) const
{
    if (!g) return "{}";
    if (m_db && g == m_db->metadata()->recycleBin()) return "";

    const int ci = colorIdx % kPaletteSize;
    const auto& pal = kPalette[ci];

    QJsonObject obj;
    obj["id"]   = QString(g->uuid().toRfc4122().toHex());
    obj["type"] = "group";

    // Conta filhos válidos para saber se é nó folha
    int validChildren = 0;
    for (auto* child : g->children())
        if (!m_db || child != m_db->metadata()->recycleBin()) ++validChildren;
    if (m_chkEntries->isChecked()) validChildren += g->entries().size();
    const bool isLeaf = (validChildren == 0);

    QJsonObject gStyle;
    // Linha mais grossa para nós folha (sem filhos) — torna a conexão mais visível
    QJsonObject linkStyle;
    linkStyle["stroke"]      = QString(pal.grpBg);
    linkStyle["strokeWidth"] = isLeaf ? 3 : 2;

    obj["num"] = prefix;   // número hierárquico — aparece no tooltip via JS

    if (depth == 0) {
        obj["topic"] = "📁 " + g->name();
        gStyle["background"]   = pal.grpBg;
        gStyle["color"]        = pal.grpFg;
        gStyle["borderRadius"] = "999px";
        gStyle["border"]       = QString("3px solid %1").arg(pal.grpBorder);
        gStyle["fontWeight"]   = "bold";
        gStyle["padding"]      = "6px 18px";
    } else {
        obj["topic"] = "🗂 " + g->name();
        gStyle["background"]   = pal.subBg;
        gStyle["color"]        = pal.subFg;
        gStyle["borderRadius"] = "999px";
        gStyle["border"]       = QString("2px solid %1").arg(pal.subBorder);
        gStyle["padding"]      = "4px 14px";
    }
    obj["style"]       = gStyle;
    obj["branchColor"] = QString(pal.grpBg);
    obj["linkStyle"]   = linkStyle;

    QJsonArray children;
    int childIdx = 1;

    for (auto* child : g->children()) {
        if (m_db && child == m_db->metadata()->recycleBin()) continue;
        const QString childPrefix = QString("%1.%2").arg(prefix).arg(childIdx++);
        const QString childJson = groupToJson(child, colorIdx, depth + 1, childPrefix);
        if (!childJson.isEmpty())
            children.append(QJsonDocument::fromJson(childJson.toUtf8()).object());
    }

    if (m_chkEntries->isChecked()) {
        for (auto* e : g->entries()) {
            QJsonObject eObj;
            eObj["id"]    = QString(e->uuid().toRfc4122().toHex());
            const QString ePrefix = QString("%1.%2").arg(prefix).arg(childIdx++);
            eObj["topic"] = "📄 " + e->title();
            eObj["num"]   = ePrefix;
            eObj["type"]  = "entry";
            QJsonObject eStyle;
            eStyle["background"]   = pal.entBg;
            eStyle["color"]        = pal.entFg;
            eStyle["borderRadius"] = "999px";
            eStyle["border"]       = QString("1px solid %1").arg(pal.entBorder);
            eStyle["fontSize"]     = "12px";
            eStyle["padding"]      = "2px 10px";
            QJsonObject eLinkStyle;
            eLinkStyle["stroke"]      = QString(pal.grpBg);
            eLinkStyle["strokeWidth"] = 2;
            eObj["style"]       = eStyle;
            eObj["branchColor"] = QString(pal.grpBg);
            eObj["linkStyle"]   = eLinkStyle;
            children.append(eObj);
        }
    }

    if (!children.isEmpty())
        obj["children"] = children;

    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

QString MindMapDialog::buildJson() const
{
    if (!m_db) return "{}";

    Group* rootGrp = m_db->rootGroup();
    const QString dbName = m_db->metadata()->name().isEmpty()
                           ? "Banco" : m_db->metadata()->name();

    QJsonObject root;
    root["id"]    = "root";
    root["topic"] = dbName;

    QJsonArray children;
    const auto topGroups = rootGrp->children();
    int topIdx = 1;
    for (int i = 0; i < topGroups.size(); ++i) {
        auto* g = topGroups[i];
        if (m_db && g == m_db->metadata()->recycleBin()) continue;
        const QString prefix = QString::number(topIdx++);
        const QString gJson = groupToJson(g, i, 0, prefix);
        if (!gJson.isEmpty())
            children.append(QJsonDocument::fromJson(gJson.toUtf8()).object());
    }
    root["children"] = children;

    QJsonObject data;
    data["nodeData"] = root;
    return QJsonDocument(data).toJson(QJsonDocument::Compact);
}

void MindMapDialog::injectMap()
{
    if (!m_mapLoaded) return;
    // Injeta o JSON como objeto JS diretamente — evita problemas de escaping
    const QString js = QString("initMindMap(%1);").arg(buildJson());
    m_webView->page()->runJavaScript(js);
}

// Constrói o caminho completo de um grupo: "Root › Pessoal › Trabalho › CfdmNote"
static QString groupPath(Group* g)
{
    QStringList parts;
    for (Group* cur = g; cur; cur = cur->parentGroup())
        parts.prepend(cur->name());
    return parts.join(" › ");
}

void MindMapDialog::updateDetail(const QString& id, const QString& topic,
                                  const QString& type)
{
    Q_UNUSED(id)
    m_detailTitle->setText(type == "entry" ? "📄 " + topic : "📁 " + topic);
    m_detailSub->clear();
    m_detailBody->clear();
    m_detailPath->clear();

    if (!m_db) return;

    if (type == "entry") {
        for (auto* e : m_db->rootGroup()->entriesRecursive()) {
            if (e->title() == topic) {
                m_detailSub->setText(e->group() ? "📁 " + e->group()->name() : "");
                QStringList body;
                if (!e->username().isEmpty()) body << "👤 " + e->username();
                if (!e->url().isEmpty())      body << "🔗 " + e->url();
                if (!e->tags().isEmpty())     body << "🏷️ " + e->tags();
                m_detailBody->setText(body.join("\n"));
                // Caminho no banco
                if (e->group())
                    m_detailPath->setText("📍 " + groupPath(e->group()) + " › " + e->title());
                return;
            }
        }
    } else {
        for (auto* g : m_db->rootGroup()->groupsRecursive(true)) {
            if (g->name() == topic) {
                const int direct = g->entries().size();
                const int total  = g->entriesRecursive().size();
                m_detailSub->setText(tr("%1 entradas · %2 total").arg(direct).arg(total));
                m_detailBody->setText(tr("%1 subgrupo(s)").arg(g->children().size()));
                m_detailPath->setText("📍 " + groupPath(g));
                return;
            }
        }
    }
}
