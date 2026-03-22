#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QSharedPointer>

class Database;
class MainWindow;

/**
 * VaultUnlockDialog
 *
 * Diálogo para abrir um cofre .kdbx — visual CfdmNote.
 * Suporta: senha + arquivo chave (individualmente ou combinados).
 */
class VaultUnlockDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VaultUnlockDialog(const QString& vaultPath,
                                MainWindow* mainWindow,
                                QWidget* parent = nullptr);

private slots:
    void onUnlock();
    void onToggleVisibility();
    void onBrowseKeyFile();
    void onClearKeyFile();

private:
    void setupUi();
    bool injectDatabase(QSharedPointer<Database> db);

    QString      m_vaultPath;
    MainWindow*  m_mainWindow  = nullptr;

    QLabel*      m_pathLabel    = nullptr;
    QLineEdit*   m_passEdit     = nullptr;
    QPushButton* m_btnToggle    = nullptr;

    // Arquivo chave
    QLineEdit*   m_keyFileEdit  = nullptr;
    QPushButton* m_btnKeyBrowse = nullptr;
    QPushButton* m_btnKeyClear  = nullptr;

    QPushButton* m_btnUnlock    = nullptr;
    QLabel*      m_errorLabel   = nullptr;
};
