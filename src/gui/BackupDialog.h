#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QListWidget>
#include <QProgressBar>
#include <QSharedPointer>

class Database;
class MainWindow;

/**
 * BackupDialog — Backup Externo do CfdmNote
 *
 * Copia o banco ativo (ou principal) para uma mídia removível/externa.
 * Salva histórico de backups em cfdmnote.conf (últimas 20 entradas).
 * Permite adicionar nota descritiva ("USB Kingston preto", etc.)
 *
 * Arquivo gerado: CfdmNote_backup_AAAA-MM-DD_HH-MM.kdbx
 */
class BackupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BackupDialog(QSharedPointer<Database> db,
                          const QString& dbPath,
                          QWidget* parent = nullptr);

    // Conveniência: abre o diálogo com o banco ativo da janela principal
    static void runBackup(MainWindow* mainWindow, QWidget* parent = nullptr);

private slots:
    void onDoBackup();
    void onBrowse();
    void onDriveSelected(int index);
    void onHistoryItemDoubleClicked(QListWidgetItem* item);
    void onRemoveHistory();

private:
    void setupUi();
    void refreshDrives();
    void refreshHistory();
    void saveToHistory(const QString& destPath, const QString& note);
    QString suggestedFileName() const;
    QString destFullPath() const;

    QSharedPointer<Database> m_db;
    QString                  m_dbPath;

    QLabel*      m_sourceLabel    = nullptr;
    QComboBox*   m_driveCombo     = nullptr;
    QLineEdit*   m_destEdit       = nullptr;
    QPushButton* m_btnBrowse      = nullptr;
    QLineEdit*   m_noteEdit       = nullptr;
    QLabel*      m_fileNameLabel  = nullptr;
    QListWidget* m_historyList    = nullptr;
    QPushButton* m_btnBackup      = nullptr;
    QPushButton* m_btnRemHistory  = nullptr;
    QLabel*      m_statusLabel    = nullptr;
    QProgressBar* m_progressBar   = nullptr;

    static const int MAX_HISTORY = 20;
};
