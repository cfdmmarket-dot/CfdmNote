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
    // Retorna o caminho padrão do banco principal
    static QString defaultDatabasePath();

    // Cria o banco principal se não existir (primeira vez)
    static bool createIfNeeded();

    // Abre o banco principal na MainWindow (auto-unlock, sem diálogo de senha)
    static bool openInMainWindow(MainWindow* mainWindow);

private:
    static QSharedPointer<Database> openDatabase(const QString& path, QString* error = nullptr);
};
