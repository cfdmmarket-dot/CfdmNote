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
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
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
    auto* actExport = bar->addAction("💾 PNG");
    bar->addSeparator();

    m_chkEntries = new QCheckBox(tr("Mostrar entradas"));
    m_chkEntries->setChecked(false);
    bar->addWidget(m_chkEntries);

    auto* hint = new QLabel(tr("   Arraste nós · Scroll = zoom · Clique = detalhes"));
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

    // Injeta o mapa quando a página carregar
    connect(m_webView, &QWebEngineView::loadFinished, this,
            [this](bool ok) {
        if (!ok) return;
        m_mapLoaded = true;
        injectMap();
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
    connect(actReload, &QAction::triggered, this, [this]() { injectMap(); });

    connect(actExport, &QAction::triggered, this, [this]() {
        const QString path = QFileDialog::getSaveFileName(
            this, tr("Exportar Mapa Mental"), QDir::homePath(), tr("PNG (*.png)"));
        if (path.isEmpty()) return;
        m_webView->grab().save(path, "PNG");
        QMessageBox::information(this, tr("Exportado"), tr("Salvo em:\n%1").arg(path));
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

static const QStringList COLORS = {
    "#3182ce", "#38a169", "#dd6b20", "#805ad5",
    "#e53e3e", "#00b5d8", "#d69e2e", "#319795",
    "#667eea", "#ed64a6"
};

QString MindMapDialog::groupToJson(Group* g, int colorIdx) const
{
    if (!g) return "{}";
    if (m_db && g == m_db->metadata()->recycleBin()) return "";

    const QString color = COLORS[colorIdx % COLORS.size()];

    QJsonObject obj;
    obj["id"]    = QString(g->uuid().toRfc4122().toHex());
    obj["topic"] = g->name();
    obj["type"]  = "group";
    obj["style"] = QJsonObject{
        {"background", color},
        {"color", "#ffffff"},
        {"border-radius", "8px"},
        {"padding", "4px 12px"}
    };

    QJsonArray children;

    // Sub-grupos
    for (auto* child : g->children()) {
        if (m_db && child == m_db->metadata()->recycleBin()) continue;
        const QString childJson = groupToJson(child, colorIdx);
        if (!childJson.isEmpty())
            children.append(QJsonDocument::fromJson(childJson.toUtf8()).object());
    }

    // Entradas (se checkbox ativo)
    if (m_chkEntries->isChecked()) {
        for (auto* e : g->entries()) {
            QJsonObject eObj;
            eObj["id"]    = QString(e->uuid().toRfc4122().toHex());
            eObj["topic"] = e->title();
            eObj["type"]  = "entry";
            eObj["style"] = QJsonObject{
                {"background", QColor(color).lighter(175).name()},
                {"color", "#1a202c"},
                {"border", "1px solid " + color},
                {"border-radius", "5px"},
                {"font-size", "12px"}
            };
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
    root["style"] = QJsonObject{
        {"background", "#2c5282"},
        {"color", "#ffffff"},
        {"font-weight", "bold"},
        {"border-radius", "50%"},
        {"width", "90px"},
        {"height", "90px"},
        {"display", "flex"},
        {"align-items", "center"},
        {"justify-content", "center"},
        {"text-align", "center"}
    };

    QJsonArray children;
    const auto topGroups = rootGrp->children();
    for (int i = 0; i < topGroups.size(); ++i) {
        auto* g = topGroups[i];
        if (m_db && g == m_db->metadata()->recycleBin()) continue;
        const QString gJson = groupToJson(g, i);
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

void MindMapDialog::updateDetail(const QString& id, const QString& topic,
                                  const QString& type)
{
    Q_UNUSED(id)
    m_detailTitle->setText(type == "entry" ? "📄 " + topic : "📁 " + topic);

    if (!m_db) return;

    if (type == "entry") {
        // Procura a entrada pelo título (rápido para bancos normais)
        for (auto* e : m_db->rootGroup()->entriesRecursive()) {
            if (e->title() == topic) {
                m_detailSub->setText(e->group() ? "📁 " + e->group()->name() : "");
                QStringList body;
                if (!e->username().isEmpty()) body << "👤 " + e->username();
                if (!e->url().isEmpty())      body << "🔗 " + e->url();
                if (!e->tags().isEmpty())     body << "🏷️ " + e->tags();
                m_detailBody->setText(body.join("\n"));
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
                return;
            }
        }
    }
}
