#include "KpsMainWindow.h"
#include "NoteEditorWidget.h"

#include <QApplication>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTreeWidgetItem>
#include <QTableWidgetItem>
#include <QAction>
#include <QMenu>
#include <QIcon>
#include <QFont>

KpsMainWindow::KpsMainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("CfdmNote");
    setMinimumSize(1024, 600);
    resize(1280, 720);

    setupMenuBar();
    setupToolBar();
    setupCentralWidget();
    setupStatusBar();
    populateDemoData();
}

void KpsMainWindow::setupMenuBar()
{
    // Menu Arquivo
    QMenu* menuArquivo = menuBar()->addMenu(tr("&Arquivo"));
    menuArquivo->addAction(tr("&Novo banco..."), this, &KpsMainWindow::onNewDatabase, QKeySequence::New);
    menuArquivo->addAction(tr("&Abrir banco..."), this, &KpsMainWindow::onOpenDatabase, QKeySequence::Open);
    menuArquivo->addSeparator();
    menuArquivo->addAction(tr("&Salvar"), this, &KpsMainWindow::onSaveDatabase, QKeySequence::Save);
    menuArquivo->addSeparator();
    menuArquivo->addAction(tr("&Sair"), qApp, &QApplication::quit, QKeySequence::Quit);

    // Menu Editar
    QMenu* menuEditar = menuBar()->addMenu(tr("&Editar"));
    menuEditar->addAction(tr("&Nova entrada"), this, &KpsMainWindow::onNewEntry, QKeySequence(Qt::CTRL | Qt::Key_N));
    menuEditar->addAction(tr("&Editar entrada"), this, [](){}, QKeySequence(Qt::Key_Return));
    menuEditar->addSeparator();
    menuEditar->addAction(tr("Copiar &usuário"), this, [](){}, QKeySequence(Qt::CTRL | Qt::Key_B));
    menuEditar->addAction(tr("Copiar &senha"), this, [](){}, QKeySequence(Qt::CTRL | Qt::Key_C));

    // Menu Visualizar
    QMenu* menuView = menuBar()->addMenu(tr("&Visualizar"));
    menuView->addAction(tr("&Painel de notas"), this, [](){}, QKeySequence(Qt::CTRL | Qt::Key_F9));

    // Menu Ferramentas
    QMenu* menuFerramentas = menuBar()->addMenu(tr("Fe&rramentas"));
    menuFerramentas->addAction(tr("&Gerador de senhas..."));
    menuFerramentas->addAction(tr("&Configurações..."));

    // Menu Ajuda
    QMenu* menuAjuda = menuBar()->addMenu(tr("A&juda"));
    menuAjuda->addAction(tr("&Sobre CfdmNote"), this, [this](){
        QMessageBox::about(this, tr("Sobre CfdmNote"),
            tr("<b>CfdmNote v1.0</b><br>"
               "Gerenciador de senhas com notas Markdown integradas.<br><br>"
               "Baseado no KeePassXC.<br>"
               "Banco nativo: formato .ksdb (SQLite)"));
    });
}

void KpsMainWindow::setupToolBar()
{
    QToolBar* toolbar = addToolBar(tr("Principal"));
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(22, 22));

    toolbar->addAction(tr("Novo"), this, &KpsMainWindow::onNewDatabase);
    toolbar->addAction(tr("Abrir"), this, &KpsMainWindow::onOpenDatabase);
    toolbar->addAction(tr("Salvar"), this, &KpsMainWindow::onSaveDatabase);
    toolbar->addSeparator();
    toolbar->addAction(tr("Bloquear"));
    toolbar->addAction(tr("Auto-Type"));
    toolbar->addSeparator();

    m_searchBox = new QLineEdit();
    m_searchBox->setPlaceholderText(tr("Buscar entradas..."));
    m_searchBox->setMinimumWidth(200);
    m_searchBox->setClearButtonEnabled(true);
    connect(m_searchBox, &QLineEdit::textChanged, this, &KpsMainWindow::onSearchTextChanged);
    toolbar->addWidget(m_searchBox);
}

void KpsMainWindow::setupCentralWidget()
{
    // Splitter principal: [esquerda] | [centro + direita]
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);

    // ── Painel esquerdo: árvore de grupos ──────────────────────────────────
    m_leftPanel = new QWidget();
    auto* leftLayout = new QVBoxLayout(m_leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* groupsLabel = new QLabel(tr(" Grupos"));
    QFont boldFont = groupsLabel->font();
    boldFont.setBold(true);
    groupsLabel->setFont(boldFont);
    leftLayout->addWidget(groupsLabel);

    m_groupTree = new QTreeWidget();
    m_groupTree->setHeaderHidden(true);
    m_groupTree->setRootIsDecorated(true);
    connect(m_groupTree, &QTreeWidget::itemClicked,
            this, &KpsMainWindow::onGroupSelected);
    leftLayout->addWidget(m_groupTree);

    m_leftPanel->setMinimumWidth(160);
    m_leftPanel->setMaximumWidth(280);

    // ── Painel central: tabela de entradas ─────────────────────────────────
    m_centerPanel = new QWidget();
    auto* centerLayout = new QVBoxLayout(m_centerPanel);
    centerLayout->setContentsMargins(0, 0, 0, 0);

    m_entryTable = new QTableWidget(0, 4);
    m_entryTable->setHorizontalHeaderLabels({tr("Título"), tr("Usuário"), tr("URL"), tr("Modificado")});
    m_entryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_entryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_entryTable->setAlternatingRowColors(true);
    m_entryTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_entryTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_entryTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_entryTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_entryTable->verticalHeader()->setVisible(false);
    m_entryTable->setSortingEnabled(true);
    connect(m_entryTable, &QTableWidget::cellClicked,
            this, &KpsMainWindow::onEntrySelected);
    centerLayout->addWidget(m_entryTable);

    // ── Painel direito: editor de notas Markdown ───────────────────────────
    m_noteEditor = new NoteEditorWidget();
    m_noteEditor->setMinimumWidth(280);

    // ── Splitter direito: centro | notas ───────────────────────────────────
    m_rightSplitter = new QSplitter(Qt::Horizontal);
    m_rightSplitter->addWidget(m_centerPanel);
    m_rightSplitter->addWidget(m_noteEditor);
    m_rightSplitter->setStretchFactor(0, 2);
    m_rightSplitter->setStretchFactor(1, 1);

    m_mainSplitter->addWidget(m_leftPanel);
    m_mainSplitter->addWidget(m_rightSplitter);
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 1);

    setCentralWidget(m_mainSplitter);
}

void KpsMainWindow::setupStatusBar()
{
    m_statusEntries = new QLabel(tr(" Nenhum banco aberto"));
    m_statusSync    = new QLabel(tr("Sync: --"));
    m_statusVersion = new QLabel(tr("CfdmNote v1.0 "));

    statusBar()->addWidget(m_statusEntries, 1);
    statusBar()->addPermanentWidget(m_statusSync);
    statusBar()->addPermanentWidget(m_statusVersion);
}

void KpsMainWindow::populateDemoData()
{
    // Grupos de demonstração
    auto* root = new QTreeWidgetItem(m_groupTree, {tr("banco.ksdb")});
    root->setExpanded(true);

    auto* personal = new QTreeWidgetItem(root, {tr("Pessoal")});
    new QTreeWidgetItem(personal, {tr("Email")});
    new QTreeWidgetItem(personal, {tr("Redes Sociais")});

    auto* work = new QTreeWidgetItem(root, {tr("Trabalho")});
    new QTreeWidgetItem(work, {tr("DevOps")});
    new QTreeWidgetItem(work, {tr("Cloud")});

    new QTreeWidgetItem(root, {tr("Dev")});

    m_groupTree->expandAll();

    // Entradas de demonstração
    auto addEntry = [this](const QString& title, const QString& user, const QString& url) {
        int row = m_entryTable->rowCount();
        m_entryTable->insertRow(row);
        m_entryTable->setItem(row, 0, new QTableWidgetItem(title));
        m_entryTable->setItem(row, 1, new QTableWidgetItem(user));
        m_entryTable->setItem(row, 2, new QTableWidgetItem(url));
        m_entryTable->setItem(row, 3, new QTableWidgetItem("2026-03-22"));
    };

    addEntry("Gmail",  "usuario@gmail.com",  "mail.google.com");
    addEntry("GitHub", "cfdmmarket-dot",      "github.com");
    addEntry("AWS",    "admin",               "aws.amazon.com");
    addEntry("GitLab", "dev",                 "gitlab.com");

    m_statusEntries->setText(tr(" banco.ksdb  |  4 entradas (demo)"));
}

void KpsMainWindow::openDatabase(const QString& filePath)
{
    m_currentDbPath = filePath;
    setWindowTitle(tr("CfdmNote — %1").arg(filePath));
    m_statusEntries->setText(tr(" %1").arg(filePath));
}

void KpsMainWindow::onEntrySelected(int row, int /*column*/)
{
    auto* titleItem = m_entryTable->item(row, 0);
    if (!titleItem) return;

    // Carrega nota correspondente à entrada selecionada
    const QString entryTitle = titleItem->text();
    m_noteEditor->loadNoteForEntry(entryTitle);
}

void KpsMainWindow::onGroupSelected()
{
    // TODO: filtrar entradas pelo grupo selecionado
}

void KpsMainWindow::onNewDatabase()
{
    QString path = QFileDialog::getSaveFileName(this,
        tr("Novo banco CfdmNote"), QDir::homePath(),
        tr("CfdmNote Database (*.ksdb)"));
    if (!path.isEmpty()) {
        if (!path.endsWith(".ksdb")) path += ".ksdb";
        openDatabase(path);
    }
}

void KpsMainWindow::onOpenDatabase()
{
    QString path = QFileDialog::getOpenFileName(this,
        tr("Abrir banco CfdmNote"), QDir::homePath(),
        tr("CfdmNote Database (*.ksdb);;KeePass Database (*.kdbx);;Todos os arquivos (*)"));
    if (!path.isEmpty()) {
        openDatabase(path);
    }
}

void KpsMainWindow::onSaveDatabase()
{
    if (m_currentDbPath.isEmpty()) {
        onNewDatabase();
        return;
    }
    statusBar()->showMessage(tr("Banco salvo."), 2000);
}

void KpsMainWindow::onNewEntry()
{
    // TODO: abrir EditEntryWidget do KeePassXC
    statusBar()->showMessage(tr("Nova entrada — em implementação"), 2000);
}

void KpsMainWindow::onSearchTextChanged(const QString& text)
{
    // Filtra entradas na tabela pela busca
    for (int row = 0; row < m_entryTable->rowCount(); ++row) {
        bool match = false;
        for (int col = 0; col < m_entryTable->columnCount(); ++col) {
            auto* item = m_entryTable->item(row, col);
            if (item && item->text().contains(text, Qt::CaseInsensitive)) {
                match = true;
                break;
            }
        }
        m_entryTable->setRowHidden(row, !text.isEmpty() && !match);
    }
}
