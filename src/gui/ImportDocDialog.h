#pragma once

#include <QDialog>
#include <QListWidget>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSharedPointer>

class Database;
class Group;

/**
 * ImportDocDialog — Importar documentos como entradas no banco.
 *
 * Formatos suportados:
 *   .txt .md .log .csv   → leitura direta (QFile)
 *   .pdf                 → pdftotext (poppler-utils)
 *   .docx                → unzip -p word/document.xml + strip XML
 *   .odt                 → unzip -p content.xml + strip XML
 *   .rtf                 → unrtf --text
 *   .html .htm           → strip tags HTML
 *
 * Cada arquivo vira uma entrada: título = nome do arquivo, nota = conteúdo extraído.
 */
class ImportDocDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImportDocDialog(QSharedPointer<Database> db, QWidget* parent = nullptr);

    static void importDocs(QSharedPointer<Database> db, QWidget* parent = nullptr);

    // Extrai texto de um arquivo — público para uso externo (main, botão na aba Notas)
    static QString extractText(const QString& filePath, QString& errorOut);

private slots:
    void onAddFiles();
    void onRemoveSelected();
    void onImport();

private:
    // Extratores por formato
    static QString extractTxt(const QString& path);
    static QString extractPdf(const QString& path, QString& err);
    static QString extractDocx(const QString& path, QString& err);
    static QString extractOdt(const QString& path, QString& err);
    static QString extractRtf(const QString& path, QString& err);
    static QString extractHtml(const QString& path);
    static QString extractImage(const QString& path, QString& err); // OCR via tesseract
    static QString stripXmlTags(const QString& xml);
    static QString stripHtmlTags(const QString& html);
    static QString runProcess(const QString& program,
                               const QStringList& args, QString& err,
                               int timeoutMs = 30000);

    void populateGroupCombo(Group* group, int depth = 0);

    QSharedPointer<Database> m_db;

    QListWidget*  m_fileList    = nullptr;
    QComboBox*    m_groupCombo  = nullptr;
    QProgressBar* m_progress    = nullptr;
    QLabel*       m_statusLabel = nullptr;
    QPushButton*  m_btnImport   = nullptr;
    QPushButton*  m_btnAdd      = nullptr;
    QPushButton*  m_btnRemove   = nullptr;

    // Groups indexados pelo combo
    QList<Group*> m_groups;
};
