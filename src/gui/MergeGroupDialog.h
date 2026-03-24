#pragma once

#include <QDialog>
#include <QTreeWidget>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSharedPointer>
#include <QList>

class Database;
class Group;
class DatabaseTabWidget;

/**
 * MergeGroupDialog — Importar grupo completo entre bancos .kdbx abertos.
 *
 * Origem e destino podem ser QUALQUER banco aberto nas abas do app.
 * Usa Group::clone() → clone recursivo completo (todos os subgrupos e
 * entradas, com novos UUIDs gerados automaticamente).
 *
 * Acesso: Menu CfdmNote → "📥 Importar Grupo entre Bancos..."
 * Atalho: Ctrl+Shift+M
 */
class MergeGroupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MergeGroupDialog(DatabaseTabWidget* tabWidget, QWidget* parent = nullptr);

    static void show(DatabaseTabWidget* tabWidget, QWidget* parent = nullptr);

private slots:
    void onSourceDbChanged(int index);
    void onDestDbChanged(int index);
    void onSourceGroupSelected();
    void onDestGroupSelected();
    void onImport();

private:
    void populateGroupTree(QTreeWidget* tree, Group* root,
                           QTreeWidgetItem* parentItem = nullptr);
    Group*                   selectedGroup(QTreeWidget* tree) const;
    QSharedPointer<Database> dbAt(int index) const;
    void                     updateStatus();
    int                      countEntriesRecursive(Group* g) const;
    int                      countGroupsRecursive(Group* g) const;

    DatabaseTabWidget* m_tabWidget;

    QComboBox*   m_srcDbCombo  = nullptr;
    QComboBox*   m_dstDbCombo  = nullptr;
    QTreeWidget* m_sourceTree  = nullptr;
    QTreeWidget* m_destTree    = nullptr;
    QLabel*      m_statusLabel = nullptr;
    QPushButton* m_btnImport   = nullptr;

    // todos os bancos abertos (índice == combo)
    QList<QSharedPointer<Database>> m_openDbs;
    QStringList                     m_openDbNames;
};
