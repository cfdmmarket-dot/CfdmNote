#pragma once

#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>

/**
 * NoteManager
 *
 * Singleton responsável pelo banco .ksdb (SQLite).
 * Gerencia: groups, entries, notes, note_links, attachments.
 * FTS5 para busca full-text em 40.000+ entradas.
 */
class NoteManager
{
public:
    static NoteManager& instance();

    // Banco de dados
    bool openDatabase(const QString& filePath);
    bool createDatabase(const QString& filePath);
    void closeDatabase();
    bool isOpen() const;
    QString databasePath() const { return m_dbPath; }

    // CRUD Notas
    QString createNote(const QString& title, const QString& content = QString());
    QString getNoteContent(const QString& noteId) const;
    QString getNoteTitle(const QString& noteId) const;
    bool    saveNoteContent(const QString& noteId, const QString& content);
    bool    saveNoteTitle(const QString& noteId, const QString& title);
    bool    deleteNote(const QString& noteId);

    // Busca full-text (FTS5)
    QStringList searchNotes(const QString& query, int limit = 50) const;
    QStringList searchEntries(const QString& query, int limit = 50) const;

    // Vínculos entrada <-> nota
    bool    linkNoteToEntry(const QString& noteId, const QString& entryId);
    QString getNoteIdForEntry(const QString& entryId) const;

    // Wiki-links (grafo de conhecimento)
    bool        saveNoteLinks(const QString& fromNoteId, const QStringList& toNoteTitles);
    QStringList getLinksFrom(const QString& noteId) const;
    QStringList getLinksTo(const QString& noteId) const;

    // ── Glossário (Seção do Saber) ────────────────────────────────────────────
    struct GlossaryTerm {
        QString id;
        QString term;
        QString definition;
        QString category;
        QString synonyms;   // separados por vírgula
        QString example;
        QString created;
        QString modified;
    };

    QString                  addGlossaryTerm(const GlossaryTerm& t);
    bool                     updateGlossaryTerm(const GlossaryTerm& t);
    bool                     deleteGlossaryTerm(const QString& id);
    QList<GlossaryTerm>      getGlossaryTerms(const QString& category = {}) const;
    QList<GlossaryTerm>      searchGlossaryTerms(const QString& query) const;
    QStringList              getGlossaryCategories() const;

private:
    NoteManager() = default;
    ~NoteManager();

    NoteManager(const NoteManager&) = delete;
    NoteManager& operator=(const NoteManager&) = delete;

    bool createSchema();
    bool createIndexes();
    bool createFtsTables();
    QString newUuid() const;

    QSqlDatabase m_db;
    QString      m_dbPath;
    bool         m_open = false;
};
