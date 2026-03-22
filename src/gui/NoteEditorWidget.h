#pragma once

#include <QWidget>
#include <QSplitter>
#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

class NoteManager;

/**
 * NoteEditorWidget
 *
 * Painel direito do CfdmNote.
 * Lado esquerdo: editor Markdown (QPlainTextEdit)
 * Lado direito:  preview HTML em tempo real (QTextBrowser)
 *
 * Usa cmark-gfm para converter Markdown → HTML.
 * Salva automaticamente via NoteManager (SQLite .ksdb).
 */
class NoteEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NoteEditorWidget(QWidget* parent = nullptr);
    ~NoteEditorWidget() override = default;

    void loadNoteForEntry(const QString& entryId);
    void loadNote(const QString& noteId);
    void clearNote();

    QString currentNoteId() const { return m_currentNoteId; }

private slots:
    void onTextChanged();
    void saveNote();
    void updatePreview();
    void toggleEditMode();

private:
    void setupUi();
    QString renderMarkdown(const QString& markdown);
    void applyWikiLinks(QString& html);

    QSplitter*      m_splitter       = nullptr;
    QPlainTextEdit* m_editor         = nullptr;
    QTextBrowser*   m_preview        = nullptr;

    // Header do painel
    QLabel*         m_titleLabel     = nullptr;
    QPushButton*    m_btnEdit        = nullptr;
    QPushButton*    m_btnGraph       = nullptr;

    // Auto-save com debounce
    QTimer*         m_saveTimer      = nullptr;
    QTimer*         m_previewTimer   = nullptr;

    QString         m_currentNoteId;
    QString         m_currentEntryId;
    bool            m_editMode       = true;
    bool            m_dirty          = false;
};
