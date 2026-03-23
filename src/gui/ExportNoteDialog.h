#pragma once

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QSharedPointer>
#include <QString>

class Entry;
class Database;

/**
 * ExportNoteDialog — Exporta a nota de uma entrada para arquivo.
 *
 * Formatos:
 *   .txt  — texto simples (sem criptografia)
 *   .cfdmnote — criptografado com AES-256-GCM (PBKDF2-SHA256, 200k iters)
 *
 * Formato binário .cfdmnote:
 *   [8B magic "CFDMNOTE"] [1B version=1]
 *   [32B salt] [12B nonce]
 *   [4B tamanho conteúdo LE] [N+16B ciphertext+tag GCM]
 */
class ExportNoteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportNoteDialog(Entry* entry,
                               const QSharedPointer<Database>& db,
                               QWidget* parent = nullptr);

    static void exportNote(Entry* entry,
                            const QSharedPointer<Database>& db,
                            QWidget* parent = nullptr);

private slots:
    void onExport();
    void onEncryptToggled(bool checked);

private:
    QString  getNoteContent() const;
    bool     exportPlain(const QString& path, const QString& content);
    bool     exportEncrypted(const QString& path, const QString& content,
                              const QString& password);

    Entry*                   m_entry;
    QSharedPointer<Database> m_db;

    QTextEdit*  m_preview     = nullptr;
    QCheckBox*  m_chkEncrypt  = nullptr;
    QWidget*    m_encryptBox  = nullptr;
    QLineEdit*  m_passEdit    = nullptr;
    QLineEdit*  m_passConfirm = nullptr;
    QLabel*     m_statusLabel = nullptr;
};
