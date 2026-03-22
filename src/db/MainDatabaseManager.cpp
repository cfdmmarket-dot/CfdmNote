#include "MainDatabaseManager.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUuid>
#include <QDebug>

#include "core/Database.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "crypto/kdf/Argon2Kdf.h"
#include "format/Kdbx4Writer.h"
#include "keys/CompositeKey.h"
#include "gui/MainWindow.h"
#include "gui/DatabaseWidget.h"
#include "gui/DatabaseTabWidget.h"

QString MainDatabaseManager::defaultDatabasePath()
{
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dataDir);
    return dataDir + "/main.kdbx";
}

bool MainDatabaseManager::createIfNeeded()
{
    const QString path = defaultDatabasePath();
    if (QFile::exists(path) && QFileInfo(path).size() > 0) return true;

    // Remove arquivo corrompido/vazio se existir
    if (QFile::exists(path)) QFile::remove(path);

    // Cria banco com CompositeKey vazia (sem senha mestra)
    auto db = QSharedPointer<Database>::create();
    db->metadata()->setName("CfdmNote");
    db->metadata()->setRecycleBinEnabled(true);

    // Grupos padrão
    auto* root = db->rootGroup();
    root->setName("CfdmNote");

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

    // Chave vazia — nenhuma senha mestra
    auto key = QSharedPointer<CompositeKey>::create();

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
    auto key = QSharedPointer<CompositeKey>::create();  // chave vazia

    if (!db->open(key, error)) {
        return {};
    }
    return db;
}

bool MainDatabaseManager::openInMainWindow(MainWindow* mainWindow)
{
    const QString path = defaultDatabasePath();

    if (!createIfNeeded()) return false;

    QString error;
    auto db = openDatabase(path, &error);
    if (!db) {
        qWarning() << "MainDatabaseManager: falha ao abrir banco principal:" << error;
        return false;
    }

    // Injeta o banco já aberto no DatabaseTabWidget — sem diálogo de senha
    auto* tabWidget = mainWindow->findChild<DatabaseTabWidget*>();
    if (!tabWidget) {
        qWarning() << "MainDatabaseManager: DatabaseTabWidget não encontrado";
        return false;
    }

    auto* dbWidget = new DatabaseWidget(db, tabWidget);
    tabWidget->addDatabaseTab(dbWidget);

    return true;
}
