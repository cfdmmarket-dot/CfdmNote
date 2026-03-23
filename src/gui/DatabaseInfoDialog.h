#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QLabel>

class MainWindow;

/**
 * DatabaseInfoDialog — Informações de todos os bancos abertos.
 *
 * Para cada banco mostra:
 *   Nome · Caminho · Tamanho em disco · Entradas · Grupos · Criado em · Atualizado em
 *
 * Acessível via:
 *   - Menu CfdmNote → "Informações dos Bancos..." (Ctrl+Shift+I)
 *   - Botão 📊 na barra de status da janela principal
 */
class DatabaseInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DatabaseInfoDialog(MainWindow* mainWindow, QWidget* parent = nullptr);

    static void showInfo(MainWindow* mainWindow, QWidget* parent = nullptr);

private slots:
    void onRefresh();
    void onCopy();

private:
    void populate();
    static QString formatSize(qint64 bytes);

    MainWindow*   m_mainWindow = nullptr;
    QTableWidget* m_table      = nullptr;
    QLabel*       m_summary    = nullptr;
};
