#include "ManualDialog.h"
#include "CfdmVersion.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QTextBrowser>
#include <QLineEdit>
#include <QFile>
#include <QRegularExpression>
#include <QShortcut>
#include <QKeySequence>
#include <QApplication>
#include <QScreen>

// cmark-gfm
#include "cmark-gfm.h"
#include "cmark-gfm-core-extensions.h"

// ─── CSS do visualizador ──────────────────────────────────────────────────────

static const char* MANUAL_CSS = R"(
<style>
body {
    font-family: 'Segoe UI', 'Liberation Sans', Arial, sans-serif;
    font-size: 13px;
    line-height: 1.7;
    color: #1a202c;
    padding: 16px 24px;
    max-width: 860px;
}
h1 {
    font-size: 20px;
    color: #1a365d;
    border-bottom: 3px solid #3182ce;
    padding-bottom: 6px;
    margin-top: 28px;
}
h2 {
    font-size: 16px;
    color: #2c5282;
    border-bottom: 1px solid #bee3f8;
    padding-bottom: 4px;
    margin-top: 22px;
}
h3 {
    font-size: 13px;
    color: #2d3748;
    margin-top: 16px;
}
code {
    background: #edf2f7;
    color: #c53030;
    padding: 1px 5px;
    border-radius: 3px;
    font-family: 'Cascadia Code', 'Fira Code', 'Courier New', monospace;
    font-size: 12px;
}
pre {
    background: #2d3748;
    color: #e2e8f0;
    padding: 14px 16px;
    border-radius: 6px;
    margin: 10px 0;
    font-size: 12px;
    line-height: 1.5;
    overflow-x: auto;
}
pre code {
    background: transparent;
    color: #e2e8f0;
    padding: 0;
}
table {
    border-collapse: collapse;
    width: 100%;
    margin: 10px 0;
    font-size: 12px;
}
th {
    background: #ebf8ff;
    color: #2b6cb0;
    font-weight: bold;
    text-align: left;
    padding: 7px 12px;
    border: 1px solid #bee3f8;
}
td {
    padding: 6px 12px;
    border: 1px solid #e2e8f0;
    vertical-align: top;
}
tr:nth-child(even) td { background: #f7fafc; }
a { color: #3182ce; text-decoration: none; }
a:hover { text-decoration: underline; }
blockquote {
    border-left: 4px solid #3182ce;
    margin: 10px 0;
    padding: 4px 12px;
    color: #4a5568;
    background: #ebf8ff;
    border-radius: 0 4px 4px 0;
}
hr {
    border: none;
    border-top: 2px solid #e2e8f0;
    margin: 20px 0;
}
li { margin: 2px 0; }
</style>
)";

// ─── Construtor ───────────────────────────────────────────────────────────────

ManualDialog::ManualDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Manual de Consulta — CfdmNote"));
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);

    // Tamanho: 80% da tela
    const QRect screen = QApplication::primaryScreen()->availableGeometry();
    resize(qMin(1100, screen.width()  - 80),
           qMin(760,  screen.height() - 80));

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ── Barra de busca (topo) ────────────────────────────────────────────────
    auto* searchBar = new QWidget();
    searchBar->setStyleSheet(
        "QWidget { background: #ebf8ff; border-bottom: 1px solid #bee3f8; padding: 4px; }");
    auto* searchLayout = new QHBoxLayout(searchBar);
    searchLayout->setContentsMargins(12, 4, 12, 4);
    searchLayout->setSpacing(6);

    auto* searchIcon = new QLabel(tr("🔍"));
    m_searchBox = new QLineEdit();
    m_searchBox->setPlaceholderText(tr("Buscar no manual..."));
    m_searchBox->setFixedHeight(26);
    m_searchBox->setStyleSheet("font-size: 12px; border: 1px solid #bee3f8; "
                                "border-radius: 4px; padding: 2px 6px;");

    auto* btnPrev = new QPushButton(tr("↑"));
    auto* btnNext = new QPushButton(tr("↓"));
    auto* btnClear = new QPushButton(tr("✕"));
    for (auto* b : {btnPrev, btnNext, btnClear}) {
        b->setFixedSize(26, 26);
        b->setStyleSheet("font-size: 12px; border: 1px solid #bee3f8; "
                         "border-radius: 4px; background: white;");
    }

    m_searchStatus = new QLabel();
    m_searchStatus->setStyleSheet("font-size: 11px; color: #4a5568; min-width: 100px;");

    searchLayout->addWidget(searchIcon);
    searchLayout->addWidget(m_searchBox, 1);
    searchLayout->addWidget(btnPrev);
    searchLayout->addWidget(btnNext);
    searchLayout->addWidget(btnClear);
    searchLayout->addWidget(m_searchStatus);
    mainLayout->addWidget(searchBar);

    // ── Splitter: TOC | Conteúdo ─────────────────────────────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setHandleWidth(1);

    // Painel TOC (esquerda)
    auto* tocPanel = new QWidget();
    tocPanel->setStyleSheet("QWidget { background: #f7fafc; }");
    auto* tocLayout = new QVBoxLayout(tocPanel);
    tocLayout->setContentsMargins(0, 0, 0, 0);
    tocLayout->setSpacing(0);

    auto* tocHeader = new QLabel(tr("  Índice"));
    tocHeader->setFixedHeight(32);
    tocHeader->setStyleSheet(
        "QLabel { background: #2c5282; color: white; font-weight: bold; "
        "font-size: 12px; padding-left: 10px; }");
    tocLayout->addWidget(tocHeader);

    m_tocList = new QListWidget();
    m_tocList->setStyleSheet(
        "QListWidget { border: none; background: #f7fafc; font-size: 11px; }"
        "QListWidget::item { padding: 4px 8px; border-bottom: 1px solid #edf2f7; }"
        "QListWidget::item:hover { background: #ebf8ff; }"
        "QListWidget::item:selected { background: #bee3f8; color: #2c5282; }");
    m_tocList->setWordWrap(true);
    tocLayout->addWidget(m_tocList, 1);
    splitter->addWidget(tocPanel);

    // Painel de conteúdo (direita)
    m_browser = new QTextBrowser();
    m_browser->setOpenLinks(true);
    m_browser->setOpenExternalLinks(true);
    m_browser->setStyleSheet("QTextBrowser { border: none; background: white; }");
    splitter->addWidget(m_browser);

    splitter->setSizes({220, width() - 220});
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter, 1);

    // ── Rodapé ───────────────────────────────────────────────────────────────
    auto* footer = new QWidget();
    footer->setFixedHeight(40);
    footer->setStyleSheet("QWidget { background: #f7fafc; border-top: 1px solid #e2e8f0; }");
    auto* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(12, 4, 12, 4);

    auto* versionLabel = new QLabel(tr("CfdmNote v%1 — Manual de Consulta · Atualizado 2026-03-24")
                                        .arg(CFDMNOTE_VERSION));
    versionLabel->setStyleSheet("font-size: 10px; color: #718096;");

    auto* btnFeatures = new QPushButton(tr("⚡ Funcionalidades"));
    btnFeatures->setFixedHeight(28);
    btnFeatures->setToolTip(tr("Ver lista completa de funcionalidades por versão"));
    btnFeatures->setStyleSheet("font-size: 11px; padding: 0 12px; color: #276749;");

    auto* btnDevlog = new QPushButton(tr("📋 Diário de Dev"));
    btnDevlog->setFixedHeight(28);
    btnDevlog->setToolTip(tr("Abrir o diário completo de desenvolvimento (DEVLOG.md)"));
    btnDevlog->setStyleSheet("font-size: 11px; padding: 0 12px; color: #2c5282;");

    auto* btnClose = new QPushButton(tr("Fechar"));
    btnClose->setFixedHeight(28);
    btnClose->setStyleSheet("font-size: 12px; padding: 0 20px;");

    footerLayout->addWidget(versionLabel);
    footerLayout->addStretch();
    footerLayout->addWidget(btnFeatures);
    footerLayout->addWidget(btnDevlog);
    footerLayout->addWidget(btnClose);
    mainLayout->addWidget(footer);

    // ── Conexões ─────────────────────────────────────────────────────────────
    connect(m_tocList, &QListWidget::itemClicked,
            this, &ManualDialog::onTocItemClicked);
    connect(m_searchBox, &QLineEdit::textChanged,
            this, &ManualDialog::onSearchChanged);
    connect(m_searchBox, &QLineEdit::returnPressed,
            this, &ManualDialog::onSearchNext);
    connect(btnNext,  &QPushButton::clicked, this, &ManualDialog::onSearchNext);
    connect(btnPrev,  &QPushButton::clicked, this, &ManualDialog::onSearchPrev);
    connect(btnClear, &QPushButton::clicked, this, &ManualDialog::onClearSearch);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnFeatures, &QPushButton::clicked, this, [this]() {
        // Recarrega o manual e navega para a seção 29
        m_browser->setHtml(renderToHtml(m_rawMarkdown));
        buildToc(m_rawMarkdown);
        // Localiza a âncora da seção "29. Lista Completa"
        const QString html = m_browser->toHtml();
        int tocIdx = 0;
        for (const QString& line : m_rawMarkdown.split('\n')) {
            if (line.startsWith("## 29.")) {
                m_browser->scrollToAnchor(QString("sec%1").arg(tocIdx));
                // Selecionar item no TOC
                for (int i = 0; i < m_tocList->count(); ++i) {
                    if (m_tocList->item(i)->data(Qt::UserRole).toInt() == tocIdx) {
                        m_tocList->setCurrentRow(i);
                        break;
                    }
                }
                break;
            }
            if (line.startsWith("# ") || line.startsWith("## "))
                ++tocIdx;
        }
        Q_UNUSED(html);
    });
    connect(btnDevlog, &QPushButton::clicked, this, [this]() {
        QFile f(":/cfdmnote/DEVLOG.md");
        QString md;
        if (f.open(QIODevice::ReadOnly | QIODevice::Text))
            md = QString::fromUtf8(f.readAll());
        else {
            QFile f2(QApplication::applicationDirPath() + "/../DEVLOG.md");
            if (f2.open(QIODevice::ReadOnly | QIODevice::Text))
                md = QString::fromUtf8(f2.readAll());
        }
        if (!md.isEmpty()) {
            m_browser->setHtml(renderToHtml(md));
            buildToc(md);
            m_tocList->clearSelection();
        }
    });

    // Atalho Esc fecha o dialog
    new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(accept()));

    // ── Carrega e renderiza o manual ─────────────────────────────────────────
    m_rawMarkdown = loadMarkdown();
    buildToc(m_rawMarkdown);
    m_browser->setHtml(renderToHtml(m_rawMarkdown));
}

// ─── Estático ─────────────────────────────────────────────────────────────────

void ManualDialog::showManual(QWidget* parent)
{
    ManualDialog dlg(parent);
    dlg.exec();
}

// ─── Carrega Markdown ─────────────────────────────────────────────────────────

QString ManualDialog::loadMarkdown() const
{
    // 1. Recurso embarcado
    QFile res(":/cfdmnote/MANUAL.md");
    if (res.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString::fromUtf8(res.readAll());

    // 2. Fallback: arquivo ao lado do executável
    QFile fs(QApplication::applicationDirPath() + "/../MANUAL.md");
    if (fs.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString::fromUtf8(fs.readAll());

    return tr("# Manual não encontrado\n\nO arquivo `MANUAL.md` não foi localizado.");
}

// ─── Renderiza Markdown → HTML ────────────────────────────────────────────────

QString ManualDialog::renderToHtml(const QString& markdown) const
{
    cmark_gfm_core_extensions_ensure_registered();

    cmark_parser* parser = cmark_parser_new(CMARK_OPT_DEFAULT);
    const char* exts[] = {"table", "strikethrough", "autolink", "tasklist"};
    for (const char* name : exts) {
        auto* ext = cmark_find_syntax_extension(name);
        if (ext) cmark_parser_attach_syntax_extension(parser, ext);
    }

    const QByteArray utf8 = markdown.toUtf8();
    cmark_parser_feed(parser, utf8.constData(), utf8.size());
    cmark_node* doc = cmark_parser_finish(parser);

    char* result = cmark_render_html(doc, CMARK_OPT_DEFAULT, nullptr);
    QString html = QString::fromUtf8(result);
    free(result);
    cmark_node_free(doc);
    cmark_parser_free(parser);

    // Adiciona âncoras às seções para navegação pelo índice
    // Processa h1 e h2 e insere <a name="secN"> com índice sequencial
    int secIdx = 0;
    QRegularExpression hre("<(h[12])>(.*?)</\\1>",
                           QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator it = hre.globalMatch(html);
    QStringList parts;
    int lastEnd = 0;
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        parts << html.mid(lastEnd, m.capturedStart() - lastEnd);
        parts << QString("<%1><a name=\"sec%2\"></a>%3</%1>")
                    .arg(m.captured(1))
                    .arg(secIdx++)
                    .arg(m.captured(2));
        lastEnd = m.capturedEnd();
    }
    parts << html.mid(lastEnd);
    html = parts.join("");

    return QString("<!DOCTYPE html><html><head>"
                   "<meta charset=\"utf-8\">%1</head><body>%2</body></html>")
               .arg(MANUAL_CSS, html);
}

// ─── Constrói o índice (TOC) ──────────────────────────────────────────────────

void ManualDialog::buildToc(const QString& markdown)
{
    m_tocList->clear();
    m_tocTitles.clear();

    int secIdx = 0;
    for (const QString& line : markdown.split('\n')) {
        QString title;
        bool isH1 = false;

        if (line.startsWith("# ") && !line.startsWith("## ")) {
            title = line.mid(2).trimmed();
            isH1  = true;
        } else if (line.startsWith("## ")) {
            title = line.mid(3).trimmed();
        }

        if (title.isEmpty()) continue;

        // Remove sufixo de âncora markdown se houver
        title.remove(QRegularExpression("\\{#[^}]+\\}$"));
        title = title.trimmed();

        auto* item = new QListWidgetItem(title);
        item->setData(Qt::UserRole, secIdx++);

        if (isH1) {
            QFont f = item->font();
            f.setBold(true);
            item->setFont(f);
            item->setForeground(QColor("#1a365d"));
        } else {
            item->setForeground(QColor("#2c5282"));
            // Indenta seções ## levemente
            item->setText("  " + title);
        }

        m_tocList->addItem(item);
        m_tocTitles << title;
    }
}

// ─── Navegação pelo TOC ───────────────────────────────────────────────────────

void ManualDialog::onTocItemClicked(QListWidgetItem* item)
{
    if (!item) return;
    const int idx = item->data(Qt::UserRole).toInt();
    m_browser->scrollToAnchor(QString("sec%1").arg(idx));
}

// ─── Busca ────────────────────────────────────────────────────────────────────

void ManualDialog::onSearchChanged(const QString& text)
{
    if (text.isEmpty()) {
        m_searchStatus->clear();
        m_searchBox->setStyleSheet(
            "font-size: 12px; border: 1px solid #bee3f8; border-radius: 4px; padding: 2px 6px;");
        return;
    }
    // Tenta encontrar para dar feedback visual imediato
    const bool found = m_browser->find(text);
    if (!found) {
        // Volta ao início e tenta novamente
        m_browser->moveCursor(QTextCursor::Start);
        const bool foundFromStart = m_browser->find(text);
        m_searchStatus->setText(foundFromStart ? tr("encontrado") : tr("não encontrado"));
        m_searchBox->setStyleSheet(foundFromStart
            ? "font-size: 12px; border: 1px solid #68d391; border-radius: 4px; padding: 2px 6px; background: #f0fff4;"
            : "font-size: 12px; border: 1px solid #fc8181; border-radius: 4px; padding: 2px 6px; background: #fff5f5;");
    } else {
        m_searchStatus->setText(tr("encontrado"));
        m_searchBox->setStyleSheet(
            "font-size: 12px; border: 1px solid #68d391; border-radius: 4px; padding: 2px 6px; background: #f0fff4;");
    }
}

void ManualDialog::onSearchNext()
{
    const QString text = m_searchBox->text();
    if (text.isEmpty()) return;
    if (!m_browser->find(text)) {
        // Volta ao início
        m_browser->moveCursor(QTextCursor::Start);
        m_browser->find(text);
        m_searchStatus->setText(tr("início"));
    }
}

void ManualDialog::onSearchPrev()
{
    const QString text = m_searchBox->text();
    if (text.isEmpty()) return;
    if (!m_browser->find(text, QTextDocument::FindBackward)) {
        m_browser->moveCursor(QTextCursor::End);
        m_browser->find(text, QTextDocument::FindBackward);
        m_searchStatus->setText(tr("fim"));
    }
}

void ManualDialog::onClearSearch()
{
    m_searchBox->clear();
    m_searchStatus->clear();
    m_searchBox->setStyleSheet(
        "font-size: 12px; border: 1px solid #bee3f8; border-radius: 4px; padding: 2px 6px;");
}
