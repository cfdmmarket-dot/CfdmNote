#include "MainDatabaseManager.h"

#include <QStandardPaths>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QUuid>
#include <QTimer>
#include <QDebug>
#include <QSplitter>
#include <QVBoxLayout>
#include <QPushButton>
#include <QIcon>
#include <QLabel>
#include <QSettings>

#include "core/Database.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "crypto/kdf/Argon2Kdf.h"
#include "format/Kdbx4Writer.h"
#include "keys/CompositeKey.h"
#include "keys/PasswordKey.h"
#include "gui/MainWindow.h"
#include "gui/DatabaseWidget.h"
#include "gui/DatabaseTabWidget.h"
#include "gui/group/GroupView.h"
#include "gui/group/GroupModel.h"
#include "core/Config.h"

// ─── Painel de Lixeira fora da árvore de grupos ───────────────────────────────

static void setupLixeiraPanel(DatabaseWidget* dbWidget, QSharedPointer<Database> db)
{
    if (!dbWidget || !db) return;

    auto* recycleBin = db->metadata()->recycleBin();
    if (!recycleBin) return;

    // Encontra o m_groupSplitter (único splitter vertical no DatabaseWidget)
    QSplitter* groupSplitter = nullptr;
    for (auto* s : dbWidget->findChildren<QSplitter*>()) {
        if (s->orientation() == Qt::Vertical) {
            groupSplitter = s;
            break;
        }
    }
    if (!groupSplitter) return;

    // Encontra a GroupView pelo objectName
    auto* groupView = qobject_cast<GroupView*>(dbWidget->findChild<QWidget*>("groupView"));
    if (!groupView) return;

    // Esconde a linha da Lixeira dentro da árvore de grupos
    auto* groupModel = qobject_cast<GroupModel*>(groupView->model());
    if (groupModel) {
        const QModelIndex idx = groupModel->index(recycleBin);
        if (idx.isValid()) {
            groupView->setRowHidden(idx.row(), idx.parent(), true);
        }
    }

    // Cria o widget da Lixeira
    auto* lixeiraWidget = new QWidget();
    lixeiraWidget->setObjectName("lixeiraWidget");
    auto* lay = new QVBoxLayout(lixeiraWidget);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    auto* lixeiraBtn = new QPushButton(
        QIcon::fromTheme("user-trash", QIcon::fromTheme("edit-delete")),
        QObject::tr("  Lixeira")
    );
    lixeiraBtn->setFlat(true);
    lixeiraBtn->setCursor(Qt::PointingHandCursor);
    lixeiraBtn->setStyleSheet(
        "QPushButton {"
        "  text-align: left; padding: 6px 10px; font-size: 12px;"
        "  border: none;"
        "  border-top: 1px solid palette(mid);"
        "  border-bottom: 1px solid palette(mid);"
        "}"
        "QPushButton:hover {"
        "  background: palette(highlight); color: palette(highlighted-text);"
        "}"
        "QPushButton:pressed {"
        "  background: palette(highlight); color: palette(highlighted-text);"
        "}"
    );

    // Ao clicar: seleciona o grupo Lixeira na GroupView
    QObject::connect(lixeiraBtn, &QPushButton::clicked, [groupView, recycleBin]() {
        groupView->setCurrentGroup(recycleBin);
    });

    lay->addWidget(lixeiraBtn);

    // Insere antes do tagsWidget (posição 1)
    groupSplitter->insertWidget(1, lixeiraWidget);
    groupSplitter->setCollapsible(1, false);
    groupSplitter->setStretchFactor(0, 100); // groupView expande
    groupSplitter->setStretchFactor(1, 0);   // lixeiraWidget — tamanho fixo
    groupSplitter->setStretchFactor(2, 0);   // tagsWidget — tamanho fixo
}

QString MainDatabaseManager::defaultDatabasePath()
{
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dataDir);
    return dataDir + "/main.kdbx";
}

QString MainDatabaseManager::configuredPath()
{
    QSettings s("CfdmNote", "CfdmNote");
    const QString saved = s.value("database/path").toString();
    if (!saved.isEmpty() && QFile::exists(saved))
        return saved;
    return defaultDatabasePath();
}

void MainDatabaseManager::setConfiguredPath(const QString& path)
{
    QSettings s("CfdmNote", "CfdmNote");
    s.setValue("database/path", path);
}

QStringList MainDatabaseManager::discoverOnMachine()
{
    QStringList found;
    const QString home = QDir::homePath();

    // Locais prioritários a varrer (não-recursivo para ser rápido)
    QStringList roots = {
        home,
        home + "/Documentos",
        home + "/Documents",
        home + "/Desktop",
        home + "/Área de Trabalho",
        home + "/Dropbox",
        home + "/OneDrive",
        home + "/Google Drive",
        home + "/Drive",
        home + "/Sync",
        home + "/NextCloud",
        home + "/ownCloud",
    };

    // Mídias removíveis / pendrives
    const QString mediaBase = "/media/" + qgetenv("USER");
    for (const QString& vol : QDir(mediaBase).entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        roots << mediaBase + "/" + vol;
    // Também /run/media
    const QString runMedia = "/run/media/" + qgetenv("USER");
    for (const QString& vol : QDir(runMedia).entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        roots << runMedia + "/" + vol;

    const QStringList filters = {"*.kdbx", "*.ksdb"};
    QSet<QString> seen;

    for (const QString& root : roots) {
        if (!QDir(root).exists()) continue;
        QDirIterator it(root, filters,
                        QDir::Files | QDir::Readable,
                        QDirIterator::Subdirectories);
        int limit = 0;
        while (it.hasNext() && limit < 50) {
            const QString f = it.next();
            if (!seen.contains(f)) { seen.insert(f); found << f; }
            ++limit;
        }
    }
    return found;
}

bool MainDatabaseManager::createIfNeeded()
{
    const QString path = defaultDatabasePath();
    if (QFile::exists(path) && QFileInfo(path).size() > 0) return true;

    // Remove arquivo corrompido/vazio se existir
    if (QFile::exists(path)) QFile::remove(path);

    // Cria banco com CompositeKey vazia (sem senha mestra)
    auto db = QSharedPointer<Database>::create();
    db->metadata()->setName("CfdmNote-Base");
    db->metadata()->setRecycleBinEnabled(true);

    // Grupos padrão
    auto* root = db->rootGroup();
    root->setName("CfdmNote-Base");

    auto* personal = new Group();
    personal->setUuid(QUuid::createUuid());
    personal->setName("Pessoal");
    personal->setParent(root);

    auto* work = new Group();
    work->setUuid(QUuid::createUuid());
    work->setName("Trabalho");
    work->setParent(root);

    auto* dev = new Group();
    dev->setUuid(QUuid::createUuid());
    dev->setName("Dev");
    dev->setParent(root);

    // Lixeira — criada explicitamente para aparecer na árvore de grupos
    auto* recycleBin = new Group();
    recycleBin->setUuid(QUuid::createUuid());
    recycleBin->setName("Lixeira");
    recycleBin->setIcon(43);  // Group::RecycleBinIconNumber
    recycleBin->setSearchingEnabled(Group::Disable);
    recycleBin->setAutoTypeEnabled(Group::Disable);
    recycleBin->setParent(root);
    db->metadata()->setRecycleBin(recycleBin);

    // PasswordKey com string vazia — faz isInitialized() retornar true
    // (CompositeKey totalmente vazio faz isInitialized() retornar false,
    //  impedindo o DatabaseWidget de ir para MainView sem mostrar unlock dialog)
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create(QString("")));

    // KDF mínimo (não precisa de segurança pesada sem senha)
    auto kdf = QSharedPointer<Argon2Kdf>::create(Argon2Kdf::Type::Argon2d);
    kdf->setRounds(1);
    kdf->setMemory(1024);
    kdf->setParallelism(1);

    db->setKey(key);
    db->setKdf(kdf);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "MainDatabaseManager: não foi possível criar" << path;
        return false;
    }

    Kdbx4Writer writer;
    const bool ok = writer.writeDatabase(&file, db.data());
    file.close();

    if (!ok) {
        qWarning() << "MainDatabaseManager: erro ao salvar banco principal";
        QFile::remove(path);
        return false;
    }

    qDebug() << "MainDatabaseManager: banco principal criado em" << path;
    return true;
}

QSharedPointer<Database> MainDatabaseManager::openDatabase(const QString& path, QString* error)
{
    auto db = QSharedPointer<Database>::create(path);
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create(QString("")));

    if (!db->open(key, error)) {
        return {};
    }
    return db;
}

bool MainDatabaseManager::openInMainWindow(MainWindow* mainWindow)
{
    const QString path = configuredPath();

    // Só cria banco novo se for o caminho padrão (não queremos criar no path do usuário)
    if (path == defaultDatabasePath() && !createIfNeeded()) return false;

    QString error;
    auto db = openDatabase(path, &error);
    if (!db) {
        qWarning() << "MainDatabaseManager: falha ao abrir banco principal:" << error;
        return false;
    }

    // Garante que o nome do banco é sempre "CfdmNote-Base"
    if (db->metadata()->name() != "CfdmNote-Base") {
        db->metadata()->setName("CfdmNote-Base");
    }

    // Injeta o banco já aberto no DatabaseTabWidget — sem diálogo de senha
    auto* tabWidget = mainWindow->findChild<DatabaseTabWidget*>();
    if (!tabWidget) {
        qWarning() << "MainDatabaseManager: DatabaseTabWidget não encontrado";
        return false;
    }

    // Define tamanhos do splitter de grupos antes de adicionar o widget
    // (evita height()=0 quando applySplitterSizes() é chamado)
    if (config()->get(Config::GUI_GroupSplitterState).toList().isEmpty()) {
        QVariantList defaultSizes = {400, 180};
        config()->set(Config::GUI_GroupSplitterState, defaultSizes);
    }

    auto* dbWidget = new DatabaseWidget(db, tabWidget);
    tabWidget->addDatabaseTab(dbWidget);

    // Reaplica o splitter e configura painel de Lixeira após widget visível
    QTimer::singleShot(300, dbWidget, [dbWidget, db]() {
        setupLixeiraPanel(dbWidget, db);

        const int h = dbWidget->height();
        if (h > 0) {
            dbWidget->setSplitterSizes({
                {Config::GUI_GroupSplitterState,
                 QList<int>({static_cast<int>(h * 0.65), 28, static_cast<int>(h * 0.35)})}
            });
        }
    });

    return true;
}
