#pragma once

#include <QString>
#include <QSharedPointer>

class Database;
class MainWindow;

/**
 * MainDatabaseManager
 *
 * Gerencia o banco principal do CfdmNote (.kdbx sem senha mestra).
 * Este banco é o "cofre sempre aberto" — abre automaticamente após o login do app.
 *
 * Separado do cofre criptografado opcional (.kdbx com senha mestra).
 */
class MainDatabaseManager
{
public:
    // Caminho padrão interno (AppLocalData/main.kdbx)
    static QString defaultDatabasePath();

    // Caminho configurado pelo usuário (QSettings); cai no padrão se não definido
    static QString configuredPath();
    static void    setConfiguredPath(const QString& path);

    // Varre locais comuns da máquina por arquivos .kdbx/.ksdb
    static QStringList discoverOnMachine();

    // Cria o banco principal no caminho configurado, se não existir
    static bool createIfNeeded();

    // Abre o banco principal na MainWindow (auto-unlock, sem diálogo de senha)
    static bool openInMainWindow(MainWindow* mainWindow);

private:
    static QSharedPointer<Database> openDatabase(const QString& path, QString* error = nullptr);
};
