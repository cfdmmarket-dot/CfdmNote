#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QSharedPointer>

class Database;
class MainWindow;

/**
 * VaultUnlockDialog
 *
 * Diálogo para abrir um cofre .kdbx com senha — com visual CfdmNote.
 * Substitui o diálogo interno do KeePassXC ("Unlock KeePassXC Database").
 *
 * Após autenticação bem-sucedida, injeta o banco no DatabaseTabWidget
 * da MainWindow sem passar pelo fluxo interno do KeePassXC.
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

private:
    void setupUi();
    bool injectDatabase(QSharedPointer<Database> db);

    QString      m_vaultPath;
    MainWindow*  m_mainWindow = nullptr;

    QLabel*      m_pathLabel   = nullptr;
    QLineEdit*   m_passEdit    = nullptr;
    QPushButton* m_btnToggle   = nullptr;
    QPushButton* m_btnUnlock   = nullptr;
    QLabel*      m_errorLabel  = nullptr;
};
