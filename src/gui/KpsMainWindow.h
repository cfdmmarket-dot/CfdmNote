#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QTreeWidget>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include <QToolBar>
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>

class NoteEditorWidget;

/**
 * KpsMainWindow
 *
 * Janela principal do CfdmNote.
 * Layout fiel ao KeePassXC: árvore de grupos | tabela de entradas | painel de notas.
 * Futuramente herda MainWindow do KeePassXC diretamente.
 */
class KpsMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit KpsMainWindow(QWidget* parent = nullptr);
    ~KpsMainWindow() override = default;

    void openDatabase(const QString& filePath);

private slots:
    void onEntrySelected(int row, int column);
    void onGroupSelected();
    void onNewDatabase();
    void onOpenDatabase();
    void onSaveDatabase();
    void onNewEntry();
    void onSearchTextChanged(const QString& text);

private:
    void setupMenuBar();
    void setupToolBar();
    void setupCentralWidget();
    void setupStatusBar();
    void populateDemoData();

    // Layout principal: grupos | entradas | notas
    QSplitter*        m_mainSplitter   = nullptr;
    QSplitter*        m_rightSplitter  = nullptr;

    // Painel esquerdo
    QWidget*          m_leftPanel      = nullptr;
    QTreeWidget*      m_groupTree      = nullptr;

    // Painel central
    QWidget*          m_centerPanel    = nullptr;
    QTableWidget*     m_entryTable     = nullptr;
    QLineEdit*        m_searchBox      = nullptr;

    // Painel direito — notas Markdown
    NoteEditorWidget* m_noteEditor     = nullptr;

    // Status bar
    QLabel*           m_statusEntries  = nullptr;
    QLabel*           m_statusSync     = nullptr;
    QLabel*           m_statusVersion  = nullptr;

    QString           m_currentDbPath;
};
