#include "NoteManager.h"

#include <QSqlError>
#include <QSqlRecord>
#include <QUuid>
#include <QDateTime>
#include <QDebug>

NoteManager& NoteManager::instance()
{
    static NoteManager inst;
    return inst;
}

NoteManager::~NoteManager()
{
    closeDatabase();
}

bool NoteManager::openDatabase(const QString& filePath)
{
    if (m_open) closeDatabase();

    m_db = QSqlDatabase::addDatabase("QSQLITE", "cfdmnote_main");
    m_db.setDatabaseName(filePath);

    if (!m_db.open()) {
        qWarning() << "NoteManager: falha ao abrir banco:" << m_db.lastError().text();
        return false;
    }

    m_dbPath = filePath;

    // WAL mode para performance em escrita concorrente
    QSqlQuery q(m_db);
    q.exec("PRAGMA journal_mode=WAL");
    q.exec("PRAGMA foreign_keys=ON");
    q.exec("PRAGMA synchronous=NORMAL");
    q.exec("PRAGMA cache_size=-32000");   // ~32 MB cache
    q.exec("PRAGMA temp_store=MEMORY");

    if (!createSchema()) return false;
    if (!createIndexes()) return false;
    if (!createFtsTables()) return false;

    m_open = true;
    return true;
}

bool NoteManager::createDatabase(const QString& filePath)
{
    return openDatabase(filePath);
}

void NoteManager::closeDatabase()
{
    if (m_open) {
        m_db.close();
        QSqlDatabase::removeDatabase("cfdmnote_main");
        m_open = false;
        m_dbPath.clear();
    }
}

bool NoteManager::isOpen() const
{
    return m_open;
}

bool NoteManager::createSchema()
{
    QSqlQuery q(m_db);

    const QStringList tables = {
        R"(CREATE TABLE IF NOT EXISTS groups (
            id       TEXT PRIMARY KEY,
            parent   TEXT,
            name     TEXT NOT NULL,
            icon     INTEGER DEFAULT 0,
            notes    TEXT,
            created  DATETIME DEFAULT CURRENT_TIMESTAMP,
            modified DATETIME DEFAULT CURRENT_TIMESTAMP
        ))",

        R"(CREATE TABLE IF NOT EXISTS entries (
            id          TEXT PRIMARY KEY,
            group_id    TEXT NOT NULL REFERENCES groups(id) ON DELETE CASCADE,
            title       TEXT,
            username    TEXT,
            password    TEXT,
            url         TEXT,
            notes       TEXT,
            icon        INTEGER DEFAULT 0,
            tags        TEXT,
            note_id     TEXT REFERENCES notes(id) ON DELETE SET NULL,
            expires     DATETIME,
            created     DATETIME DEFAULT CURRENT_TIMESTAMP,
            modified    DATETIME DEFAULT CURRENT_TIMESTAMP,
            accessed    DATETIME
        ))",

        R"(CREATE TABLE IF NOT EXISTS notes (
            id          TEXT PRIMARY KEY,
            title       TEXT NOT NULL,
            content     TEXT DEFAULT '',
            created     DATETIME DEFAULT CURRENT_TIMESTAMP,
            modified    DATETIME DEFAULT CURRENT_TIMESTAMP
        ))",

        R"(CREATE TABLE IF NOT EXISTS note_links (
            from_id     TEXT NOT NULL REFERENCES notes(id) ON DELETE CASCADE,
            to_id       TEXT NOT NULL REFERENCES notes(id) ON DELETE CASCADE,
            PRIMARY KEY (from_id, to_id)
        ))",

        R"(CREATE TABLE IF NOT EXISTS attachments (
            id          TEXT PRIMARY KEY,
            entry_id    TEXT REFERENCES entries(id) ON DELETE CASCADE,
            filename    TEXT NOT NULL,
            data        BLOB
        ))",

        R"(CREATE TABLE IF NOT EXISTS glossary (
            id          TEXT PRIMARY KEY,
            term        TEXT NOT NULL COLLATE NOCASE,
            definition  TEXT NOT NULL DEFAULT '',
            category    TEXT NOT NULL DEFAULT 'Geral',
            synonyms    TEXT NOT NULL DEFAULT '',
            example     TEXT NOT NULL DEFAULT '',
            created     DATETIME DEFAULT CURRENT_TIMESTAMP,
            modified    DATETIME DEFAULT CURRENT_TIMESTAMP
        ))"
    };

    for (const QString& sql : tables) {
        if (!q.exec(sql)) {
            qWarning() << "NoteManager::createSchema falhou:" << q.lastError().text();
            return false;
        }
    }
    return true;
}

bool NoteManager::createIndexes()
{
    QSqlQuery q(m_db);

    const QStringList indexes = {
        "CREATE INDEX IF NOT EXISTS idx_entries_group    ON entries(group_id)",
        "CREATE INDEX IF NOT EXISTS idx_entries_title    ON entries(title COLLATE NOCASE)",
        "CREATE INDEX IF NOT EXISTS idx_entries_url      ON entries(url)",
        "CREATE INDEX IF NOT EXISTS idx_entries_username ON entries(username COLLATE NOCASE)",
        "CREATE INDEX IF NOT EXISTS idx_entries_tags     ON entries(tags)",
        "CREATE INDEX IF NOT EXISTS idx_entries_note     ON entries(note_id)",
        "CREATE INDEX IF NOT EXISTS idx_notes_title      ON notes(title COLLATE NOCASE)",
        "CREATE INDEX IF NOT EXISTS idx_note_links_from  ON note_links(from_id)",
        "CREATE INDEX IF NOT EXISTS idx_note_links_to    ON note_links(to_id)"
    };

    for (const QString& sql : indexes) {
        if (!q.exec(sql)) {
            qWarning() << "NoteManager::createIndexes falhou:" << q.lastError().text();
            return false;
        }
    }
    return true;
}

bool NoteManager::createFtsTables()
{
    QSqlQuery q(m_db);

    const QStringList ftsTables = {
        R"(CREATE VIRTUAL TABLE IF NOT EXISTS entries_fts USING fts5(
            title, username, url, notes, tags,
            content=entries, content_rowid=rowid
        ))",

        R"(CREATE VIRTUAL TABLE IF NOT EXISTS notes_fts USING fts5(
            title, content,
            content=notes, content_rowid=rowid
        ))",

        // Triggers para manter FTS sincronizado
        R"(CREATE TRIGGER IF NOT EXISTS entries_fts_insert AFTER INSERT ON entries BEGIN
            INSERT INTO entries_fts(rowid, title, username, url, notes, tags)
            VALUES (new.rowid, new.title, new.username, new.url, new.notes, new.tags);
        END)",

        R"(CREATE TRIGGER IF NOT EXISTS entries_fts_delete AFTER DELETE ON entries BEGIN
            INSERT INTO entries_fts(entries_fts, rowid, title, username, url, notes, tags)
            VALUES ('delete', old.rowid, old.title, old.username, old.url, old.notes, old.tags);
        END)",

        R"(CREATE TRIGGER IF NOT EXISTS entries_fts_update AFTER UPDATE ON entries BEGIN
            INSERT INTO entries_fts(entries_fts, rowid, title, username, url, notes, tags)
            VALUES ('delete', old.rowid, old.title, old.username, old.url, old.notes, old.tags);
            INSERT INTO entries_fts(rowid, title, username, url, notes, tags)
            VALUES (new.rowid, new.title, new.username, new.url, new.notes, new.tags);
        END)",

        R"(CREATE TRIGGER IF NOT EXISTS notes_fts_insert AFTER INSERT ON notes BEGIN
            INSERT INTO notes_fts(rowid, title, content)
            VALUES (new.rowid, new.title, new.content);
        END)",

        R"(CREATE TRIGGER IF NOT EXISTS notes_fts_delete AFTER DELETE ON notes BEGIN
            INSERT INTO notes_fts(notes_fts, rowid, title, content)
            VALUES ('delete', old.rowid, old.title, old.content);
        END)",

        R"(CREATE TRIGGER IF NOT EXISTS notes_fts_update AFTER UPDATE ON notes BEGIN
            INSERT INTO notes_fts(notes_fts, rowid, title, content)
            VALUES ('delete', old.rowid, old.title, old.content);
            INSERT INTO notes_fts(rowid, title, content)
            VALUES (new.rowid, new.title, new.content);
        END)"
    };

    for (const QString& sql : ftsTables) {
        if (!q.exec(sql)) {
            qWarning() << "NoteManager::createFtsTables falhou:" << q.lastError().text();
            // FTS pode já existir — não é fatal
        }
    }
    return true;
}

// ─── CRUD Notas ─────────────────────────────────────────────────────────────

QString NoteManager::createNote(const QString& title, const QString& content)
{
    const QString id = newUuid();
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO notes (id, title, content) VALUES (:id, :title, :content)");
    q.bindValue(":id",      id);
    q.bindValue(":title",   title);
    q.bindValue(":content", content);

    if (!q.exec()) {
        qWarning() << "NoteManager::createNote falhou:" << q.lastError().text();
        return QString();
    }
    return id;
}

QString NoteManager::getNoteContent(const QString& noteId) const
{
    QSqlQuery q(m_db);
    q.prepare("SELECT content FROM notes WHERE id = :id");
    q.bindValue(":id", noteId);
    if (q.exec() && q.next()) return q.value(0).toString();
    return QString();
}

QString NoteManager::getNoteTitle(const QString& noteId) const
{
    QSqlQuery q(m_db);
    q.prepare("SELECT title FROM notes WHERE id = :id");
    q.bindValue(":id", noteId);
    if (q.exec() && q.next()) return q.value(0).toString();
    return QString();
}

bool NoteManager::saveNoteContent(const QString& noteId, const QString& content)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE notes SET content = :content, modified = CURRENT_TIMESTAMP WHERE id = :id");
    q.bindValue(":content", content);
    q.bindValue(":id",      noteId);
    return q.exec();
}

bool NoteManager::saveNoteTitle(const QString& noteId, const QString& title)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE notes SET title = :title, modified = CURRENT_TIMESTAMP WHERE id = :id");
    q.bindValue(":title", title);
    q.bindValue(":id",    noteId);
    return q.exec();
}

bool NoteManager::deleteNote(const QString& noteId)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM notes WHERE id = :id");
    q.bindValue(":id", noteId);
    return q.exec();
}

// ─── Busca FTS5 ──────────────────────────────────────────────────────────────

QStringList NoteManager::searchNotes(const QString& query, int limit) const
{
    QStringList results;
    QSqlQuery q(m_db);
    q.prepare("SELECT id FROM notes_fts WHERE notes_fts MATCH :q LIMIT :lim");
    q.bindValue(":q",   query + "*");
    q.bindValue(":lim", limit);
    if (q.exec()) {
        while (q.next()) results << q.value(0).toString();
    }
    return results;
}

QStringList NoteManager::searchEntries(const QString& query, int limit) const
{
    QStringList results;
    QSqlQuery q(m_db);
    q.prepare("SELECT id FROM entries_fts WHERE entries_fts MATCH :q LIMIT :lim");
    q.bindValue(":q",   query + "*");
    q.bindValue(":lim", limit);
    if (q.exec()) {
        while (q.next()) results << q.value(0).toString();
    }
    return results;
}

// ─── Vínculos entrada <-> nota ───────────────────────────────────────────────

bool NoteManager::linkNoteToEntry(const QString& noteId, const QString& entryId)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE entries SET note_id = :note_id WHERE id = :entry_id");
    q.bindValue(":note_id",  noteId);
    q.bindValue(":entry_id", entryId);
    return q.exec();
}

QString NoteManager::getNoteIdForEntry(const QString& entryId) const
{
    QSqlQuery q(m_db);
    q.prepare("SELECT note_id FROM entries WHERE id = :id");
    q.bindValue(":id", entryId);
    if (q.exec() && q.next()) return q.value(0).toString();
    return QString();
}

// ─── Wiki-links ──────────────────────────────────────────────────────────────

bool NoteManager::saveNoteLinks(const QString& fromNoteId, const QStringList& toNoteTitles)
{
    QSqlQuery q(m_db);
    // Remove links antigos
    q.prepare("DELETE FROM note_links WHERE from_id = :id");
    q.bindValue(":id", fromNoteId);
    q.exec();

    for (const QString& title : toNoteTitles) {
        // Resolve título -> id
        QSqlQuery findQ(m_db);
        findQ.prepare("SELECT id FROM notes WHERE title = :title LIMIT 1");
        findQ.bindValue(":title", title);
        if (findQ.exec() && findQ.next()) {
            const QString toId = findQ.value(0).toString();
            q.prepare("INSERT OR IGNORE INTO note_links (from_id, to_id) VALUES (:from, :to)");
            q.bindValue(":from", fromNoteId);
            q.bindValue(":to",   toId);
            q.exec();
        }
    }
    return true;
}

QStringList NoteManager::getLinksFrom(const QString& noteId) const
{
    QStringList results;
    QSqlQuery q(m_db);
    q.prepare("SELECT to_id FROM note_links WHERE from_id = :id");
    q.bindValue(":id", noteId);
    if (q.exec()) {
        while (q.next()) results << q.value(0).toString();
    }
    return results;
}

QStringList NoteManager::getLinksTo(const QString& noteId) const
{
    QStringList results;
    QSqlQuery q(m_db);
    q.prepare("SELECT from_id FROM note_links WHERE to_id = :id");
    q.bindValue(":id", noteId);
    if (q.exec()) {
        while (q.next()) results << q.value(0).toString();
    }
    return results;
}

// ─── Util ────────────────────────────────────────────────────────────────────

QString NoteManager::newUuid() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// ─── Glossário ────────────────────────────────────────────────────────────────

static NoteManager::GlossaryTerm rowToTerm(const QSqlQuery& q)
{
    NoteManager::GlossaryTerm t;
    t.id         = q.value("id").toString();
    t.term       = q.value("term").toString();
    t.definition = q.value("definition").toString();
    t.category   = q.value("category").toString();
    t.synonyms   = q.value("synonyms").toString();
    t.example    = q.value("example").toString();
    t.created    = q.value("created").toString();
    t.modified   = q.value("modified").toString();
    return t;
}

QString NoteManager::addGlossaryTerm(const GlossaryTerm& t)
{
    if (!m_open) return {};
    const QString id = newUuid();
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO glossary(id,term,definition,category,synonyms,example) "
              "VALUES(:id,:term,:def,:cat,:syn,:ex)");
    q.bindValue(":id",   id);
    q.bindValue(":term", t.term);
    q.bindValue(":def",  t.definition);
    q.bindValue(":cat",  t.category.isEmpty() ? "Geral" : t.category);
    q.bindValue(":syn",  t.synonyms);
    q.bindValue(":ex",   t.example);
    return q.exec() ? id : QString();
}

bool NoteManager::updateGlossaryTerm(const GlossaryTerm& t)
{
    if (!m_open) return false;
    QSqlQuery q(m_db);
    q.prepare("UPDATE glossary SET term=:term,definition=:def,category=:cat,"
              "synonyms=:syn,example=:ex,modified=CURRENT_TIMESTAMP WHERE id=:id");
    q.bindValue(":id",   t.id);
    q.bindValue(":term", t.term);
    q.bindValue(":def",  t.definition);
    q.bindValue(":cat",  t.category.isEmpty() ? "Geral" : t.category);
    q.bindValue(":syn",  t.synonyms);
    q.bindValue(":ex",   t.example);
    return q.exec();
}

bool NoteManager::deleteGlossaryTerm(const QString& id)
{
    if (!m_open) return false;
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM glossary WHERE id=:id");
    q.bindValue(":id", id);
    return q.exec();
}

QList<NoteManager::GlossaryTerm> NoteManager::getGlossaryTerms(const QString& category) const
{
    QList<GlossaryTerm> list;
    if (!m_open) return list;
    QSqlQuery q(m_db);
    if (category.isEmpty())
        q.prepare("SELECT * FROM glossary ORDER BY term COLLATE NOCASE");
    else {
        q.prepare("SELECT * FROM glossary WHERE category=:cat ORDER BY term COLLATE NOCASE");
        q.bindValue(":cat", category);
    }
    if (!q.exec()) return list;
    while (q.next()) list << rowToTerm(q);
    return list;
}

QList<NoteManager::GlossaryTerm> NoteManager::searchGlossaryTerms(const QString& query) const
{
    QList<GlossaryTerm> list;
    if (!m_open || query.trimmed().isEmpty()) return list;
    const QString like = "%" + query.trimmed() + "%";
    QSqlQuery q(m_db);
    q.prepare("SELECT * FROM glossary WHERE term LIKE :q OR definition LIKE :q2 "
              "OR synonyms LIKE :q3 ORDER BY term COLLATE NOCASE");
    q.bindValue(":q",  like);
    q.bindValue(":q2", like);
    q.bindValue(":q3", like);
    if (!q.exec()) return list;
    while (q.next()) list << rowToTerm(q);
    return list;
}

QStringList NoteManager::getGlossaryCategories() const
{
    QStringList cats;
    if (!m_open) return cats;
    QSqlQuery q(m_db);
    q.exec("SELECT DISTINCT category FROM glossary ORDER BY category COLLATE NOCASE");
    while (q.next()) cats << q.value(0).toString();
    return cats;
}
