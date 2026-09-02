#include "NoteEditorWidget.h"
#include "notes/NoteManager.h"

// cmark-gfm
#include "cmark-gfm.h"
#include "cmark-gfm-core-extensions.h"

#include <QRegularExpression>
#include <QScrollBar>
#include <QTextCursor>

NoteEditorWidget::NoteEditorWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUi();

    // Timers com debounce: preview 300ms, auto-save 1500ms
    m_previewTimer = new QTimer(this);
    m_previewTimer->setSingleShot(true);
    m_previewTimer->setInterval(300);
    connect(m_previewTimer, &QTimer::timeout, this, &NoteEditorWidget::updatePreview);

    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(1500);
    connect(m_saveTimer, &QTimer::timeout, this, &NoteEditorWidget::saveNote);
}

void NoteEditorWidget::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Header
    auto* header = new QWidget();
    header->setFixedHeight(32);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(6, 0, 6, 0);

    m_titleLabel = new QLabel(tr("Nota"));
    QFont f = m_titleLabel->font();
    f.setBold(true);
    m_titleLabel->setFont(f);

    m_btnEdit  = new QPushButton(tr("Editar"));
    m_btnEdit->setFlat(true);
    m_btnEdit->setCheckable(true);
    m_btnEdit->setChecked(true);
    connect(m_btnEdit, &QPushButton::clicked, this, &NoteEditorWidget::toggleEditMode);

    m_btnGraph = new QPushButton(tr("Grafo"));
    m_btnGraph->setFlat(true);

    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_btnEdit);
    headerLayout->addWidget(m_btnGraph);

    mainLayout->addWidget(header);

    // Splitter editor | preview
    m_splitter = new QSplitter(Qt::Horizontal);

    m_editor = new QPlainTextEdit();
    m_editor->setPlaceholderText(tr("Escreva sua nota em Markdown...\n\nSuporta [[wiki-links]], **negrito**, *itálico*, # Títulos e muito mais."));
    m_editor->setFont(QFont("Monospace", 10));
    connect(m_editor, &QPlainTextEdit::textChanged, this, &NoteEditorWidget::onTextChanged);

    m_preview = new QTextBrowser();
    m_preview->setOpenExternalLinks(true);
    m_preview->setReadOnly(true);

    m_splitter->addWidget(m_editor);
    m_splitter->addWidget(m_preview);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(m_splitter);

    // Estado inicial: mostra placeholder
    clearNote();
}

void NoteEditorWidget::loadNoteForEntry(const QString& entryId)
{
    m_currentEntryId = entryId;

    // Busca nota linkada à entrada no NoteManager
    NoteManager& nm = NoteManager::instance();
    const QString noteId = nm.getNoteIdForEntry(entryId);

    if (noteId.isEmpty()) {
        // Entrada sem nota ainda — mostra editor vazio pronto para criar
        m_currentNoteId = QString();
        m_titleLabel->setText(tr("Nota — %1").arg(entryId));
        m_editor->setPlainText(QString());
        m_preview->setHtml(tr("<p style='color:gray'>Nenhuma nota para esta entrada.<br>"
                               "Comece a digitar para criar uma.</p>"));
    } else {
        loadNote(noteId);
    }
    m_dirty = false;
}

void NoteEditorWidget::loadNote(const QString& noteId)
{
    m_currentNoteId = noteId;
    NoteManager& nm = NoteManager::instance();

    const QString content = nm.getNoteContent(noteId);
    const QString title   = nm.getNoteTitle(noteId);

    m_titleLabel->setText(title.isEmpty() ? tr("Nota") : title);

    // Bloqueia sinal para não disparar onTextChanged ao carregar
    QSignalBlocker blocker(m_editor);
    m_editor->setPlainText(content);

    updatePreview();
    m_dirty = false;
}

void NoteEditorWidget::clearNote()
{
    m_currentNoteId  = QString();
    m_currentEntryId = QString();
    m_titleLabel->setText(tr("Nota"));
    {
        QSignalBlocker blocker(m_editor);
        m_editor->setPlainText(QString());
    }
    m_preview->setHtml(tr("<p style='color:gray'>Selecione uma entrada para ver sua nota.</p>"));
    m_dirty = false;
}

void NoteEditorWidget::appendText(const QString& text)
{
    const QString current = m_editor->toPlainText();
    const QString sep = current.trimmed().isEmpty() ? QString() : QStringLiteral("\n\n---\n");
    // setPlainText dispara onTextChanged → auto-save via timer
    m_editor->setPlainText(current + sep + text);
    // Rola para o final
    QTextCursor c = m_editor->textCursor();
    c.movePosition(QTextCursor::End);
    m_editor->setTextCursor(c);
}

void NoteEditorWidget::onTextChanged()
{
    m_dirty = true;
    m_previewTimer->start();
    m_saveTimer->start();
}

void NoteEditorWidget::updatePreview()
{
    const QString markdown = m_editor->toPlainText();
    QString html = renderMarkdown(markdown);
    applyWikiLinks(html);

    // Preserva posição do scroll
    const int scrollPos = m_preview->verticalScrollBar()->value();
    m_preview->setHtml(html);
    m_preview->verticalScrollBar()->setValue(scrollPos);
}

void NoteEditorWidget::saveNote()
{
    if (!m_dirty) return;

    NoteManager& nm = NoteManager::instance();
    const QString content = m_editor->toPlainText();

    if (m_currentNoteId.isEmpty()) {
        // Cria nova nota e linka à entrada
        const QString title = content.split('\n').first()
                                     .remove(QRegularExpression("^#+\\s*"))
                                     .trimmed();
        m_currentNoteId = nm.createNote(
            title.isEmpty() ? tr("Sem título") : title,
            content
        );
        if (!m_currentEntryId.isEmpty()) {
            nm.linkNoteToEntry(m_currentNoteId, m_currentEntryId);
        }
    } else {
        nm.saveNoteContent(m_currentNoteId, content);
    }

    m_dirty = false;
}

QString NoteEditorWidget::renderMarkdown(const QString& markdown)
{
    if (markdown.isEmpty()) return QString();

    // Registra extensões GFM do cmark-gfm
    cmark_gfm_core_extensions_ensure_registered();

    cmark_parser* parser = cmark_parser_new(CMARK_OPT_DEFAULT);

    // Ativa extensões GFM: tabelas, strikethrough, autolink, tasklist
    const char* extensionNames[] = {"table", "strikethrough", "autolink", "tasklist"};
    for (const char* name : extensionNames) {
        cmark_syntax_extension* ext = cmark_find_syntax_extension(name);
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

    // CSS minimalista para o preview
    const QString css = R"(
        <style>
        body { font-family: sans-serif; font-size: 14px; line-height: 1.6;
               margin: 12px; color: #ddd; background: #1e1e1e; }
        h1,h2,h3 { color: #eee; border-bottom: 1px solid #444; padding-bottom: 4px; }
        a { color: #4da6ff; }
        code { background: #2d2d2d; padding: 2px 5px; border-radius: 3px; font-family: monospace; }
        pre { background: #2d2d2d; padding: 10px; border-radius: 4px; overflow-x: auto; }
        blockquote { border-left: 3px solid #555; margin: 0; padding-left: 12px; color: #aaa; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #444; padding: 6px 10px; }
        th { background: #2d2d2d; }
        .wiki-link { color: #7ec8e3; text-decoration: none; cursor: pointer; }
        .wiki-link:hover { text-decoration: underline; }
        </style>
    )";

    return css + html;
}

void NoteEditorWidget::applyWikiLinks(QString& html)
{
    // Converte [[Nome da Nota]] em links clicáveis
    static const QRegularExpression wikiRe(R"(\[\[([^\[\]]+)\]\])");
    html.replace(wikiRe,
        R"(<a class="wiki-link" href="wikilink://\1">[[\1]]</a>)");
}

void NoteEditorWidget::toggleEditMode()
{
    m_editMode = !m_editMode;
    m_editor->setVisible(m_editMode);
    m_btnEdit->setChecked(m_editMode);
    m_btnEdit->setText(m_editMode ? tr("Editar") : tr("Visualizar"));
}
