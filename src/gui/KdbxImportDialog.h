#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QSharedPointer>
#include <QCheckBox>

class Database;
class Group;
class MainWindow;

/**
 * KdbxImportDialog — Importar banco .kdbx externo (sem precisar abri-lo no app).
 *
 * Abre qualquer .kdbx (com ou sem senha / arquivo chave) e importa
 * TODOS os grupos e entradas para o banco atualmente ativo no CfdmNote,
 * preservando a hierarquia de grupos completa.
 *
 * Acesso: Menu CfdmNote → "📂 Importar .kdbx Externo..."
 * Atalho: Ctrl+Shift+K
 */
class KdbxImportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KdbxImportDialog(QSharedPointer<Database> destDb,
                               MainWindow* mainWindow,
                               QWidget* parent = nullptr);

    static void runImport(QSharedPointer<Database> destDb,
                          MainWindow* mainWindow,
                          QWidget* parent = nullptr);

private slots:
    void onBrowseKdbx();
    void onBrowseKeyFile();
    void onTogglePassword();
    void onImport();

private:
    void setupUi();
    bool doImport();
    void importGroupRecursive(Group* src, Group* dest, int& count);

    QSharedPointer<Database> m_destDb;
    MainWindow*              m_mainWindow;

    QLineEdit*   m_kdbxEdit      = nullptr;
    QLineEdit*   m_passEdit      = nullptr;
    QLineEdit*   m_keyFileEdit   = nullptr;
    QPushButton* m_btnToggle     = nullptr;
    QPushButton* m_btnKeyClear   = nullptr;
    QLabel*      m_errorLabel    = nullptr;
    QLabel*      m_statusLabel   = nullptr;
    QPushButton* m_btnImport     = nullptr;
};
