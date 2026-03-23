#pragma once

#include <QDialog>
#include <QSharedPointer>

class Database;
class MainWindow;

/**
 * CsvImportDialog
 *
 * Importador CSV próprio do CfdmNote com barra de progresso real.
 * Processa em lotes de 200 entradas, atualizando a UI a cada lote.
 *
 * Colunas esperadas (detectadas automaticamente):
 *   Title, Username, Password, URL, Notes, Group  (qualquer ordem, case-insensitive)
 *   Ou ordem padrão KeePassXC: Group, Title, Username, Password, URL, Notes
 */
class CsvImportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CsvImportDialog(QSharedPointer<Database> db,
                              MainWindow* mainWindow,
                              QWidget* parent = nullptr);

    // Abre seletor de arquivo e executa o import
    static void runImport(QSharedPointer<Database> db,
                          MainWindow* mainWindow,
                          QWidget* parent = nullptr);

private:
    void doImport(const QString& csvPath);

    QSharedPointer<Database> m_db;
    MainWindow* m_mainWindow;
};
