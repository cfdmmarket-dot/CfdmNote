#pragma once

#include <QDialog>
#include <QSharedPointer>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>

/**
 * DependencyCheckDialog — Verifica dependências do CfdmNote na inicialização.
 *
 * Executa na primeira execução e sempre que chamado pelo menu.
 * Mostra tabela com: status ✅/⚠/❌, dependência, feature associada, comando de instalação.
 *
 * Uso:
 *   DependencyCheckDialog::checkOnStartup(parent);  // só mostra se houver algo faltando
 *   DependencyCheckDialog::showFull(parent);         // sempre mostra (chamado pelo menu)
 */
class DependencyCheckDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DependencyCheckDialog(QWidget* parent = nullptr);

    // Mostra apenas se houver dependências faltando (ou na primeira execução)
    static void checkOnStartup(QWidget* parent);

    // Abre o dialog completo (chamado pelo menu)
    static void showFull(QWidget* parent);

    // Retorna true se todas as dependências obrigatórias estão presentes
    static bool allRequiredPresent();

private:
    void runChecks();

    struct DepResult {
        QString name;
        QString feature;
        QString installCmd;
        bool    required;
        bool    found;
        QString detail;     // versão ou mensagem de erro
    };

    static QList<DepResult> check();

    QTableWidget*  m_table    = nullptr;
    QLabel*        m_summary  = nullptr;
    QPushButton*   m_btnClose = nullptr;
    QPushButton*   m_btnCopy  = nullptr;
    QProgressBar*  m_progress = nullptr;
};
