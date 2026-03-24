#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QSharedPointer>

class Entry;
class Database;

/**
 * LinksPanelWidget — aba "Links" no preview de entrada.
 *
 * Dois tipos de link por entrada (múltiplos caminhos cada):
 *   Local Disk          → pasta no disco local
 *   External/Removable  → pasta em disco removível (indica se montado)
 *
 * Armazenamento: atributos personalizados da entrada KeePassXC
 *   "CfdmNote::LocalLinks"     → caminhos separados por \n
 *   "CfdmNote::RemovableLinks" → caminhos separados por \n
 */
class LinksPanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LinksPanelWidget(QWidget* parent = nullptr);

    void loadEntry(Entry* entry, const QSharedPointer<Database>& db);
    void clearLinks();

    static const QString KEY_LOCAL;
    static const QString KEY_REMOVABLE;

private slots:
    void onAddLocal();
    void onRemoveLocal();
    void onOpenLocal();
    void onAddRemovable();
    void onRemoveRemovable();
    void onOpenRemovable();

private:
    // Constrói uma seção (título + lista + botões) e retorna o widget
    QWidget* buildSection(const QString& title,
                          const QString& icon,
                          QListWidget*& listOut,
                          QPushButton*& btnAddOut,
                          QPushButton*& btnRemOut,
                          QPushButton*& btnOpenOut);

    void refreshItemIcons(QListWidget* list);
    void saveLinks();
    void openPath(const QString& path);
    void openContainingFolder(const QString& path);
    void openWith(const QString& path);
    void openWithApp(const QString& path, const QString& app);
    void setupContextMenu(QListWidget* list);
    void addPathToList(QListWidget* list, const QString& typeKey);
    void addFileToList(QListWidget* list, const QString& typeKey);

    static QStringList suggestAppsForPath(const QString& path);

    QListWidget* m_localList     = nullptr;
    QPushButton* m_btnAddLocal   = nullptr;
    QPushButton* m_btnRemLocal   = nullptr;
    QPushButton* m_btnOpenLocal  = nullptr;

    QListWidget* m_removableList    = nullptr;
    QPushButton* m_btnAddRemovable  = nullptr;
    QPushButton* m_btnRemRemovable  = nullptr;
    QPushButton* m_btnOpenRemovable = nullptr;

    Entry*                   m_entry = nullptr;
    QSharedPointer<Database> m_db;
};
