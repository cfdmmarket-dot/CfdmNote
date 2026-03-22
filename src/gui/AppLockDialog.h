#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>

/**
 * AppLockDialog
 *
 * Tela de login do CfdmNote — aparece antes da janela principal.
 * Senha separada do cofre .kdbx (que tem sua própria senha mestra).
 *
 * Primeira vez: pede para criar a senha do app + configura perguntas de segurança.
 * Próximas vezes: pede a senha para desbloquear.
 *                 Link "Esqueceu a senha?" abre recuperação por perguntas secretas.
 *
 * Senha armazenada em hash (SHA-256) no arquivo de configuração do CfdmNote.
 */
class AppLockDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AppLockDialog(QWidget* parent = nullptr);
    ~AppLockDialog() override = default;

    // true se o app ainda não tem senha configurada
    static bool isFirstRun();
    // Verifica se a senha bate com o hash salvo
    static bool checkPassword(const QString& password);
    // Salva o hash da senha
    static void savePassword(const QString& password);
    // Caminho do arquivo de config
    static QString configPath();

private slots:
    void onUnlock();
    void onToggleVisibility();
    void onForgotPassword();

private:
    void setupUi(bool firstRun);
    static QString hashPassword(const QString& password);

    QLabel*      m_logoLabel     = nullptr;
    QLabel*      m_titleLabel    = nullptr;
    QLabel*      m_subtitleLabel = nullptr;
    QLineEdit*   m_passwordEdit  = nullptr;
    QLineEdit*   m_confirmEdit   = nullptr;   // só na primeira vez
    QPushButton* m_btnToggle     = nullptr;
    QPushButton* m_btnUnlock     = nullptr;
    QPushButton* m_btnForgot     = nullptr;   // link "Esqueceu a senha?"
    QLabel*      m_errorLabel    = nullptr;
    bool         m_firstRun      = false;
};
