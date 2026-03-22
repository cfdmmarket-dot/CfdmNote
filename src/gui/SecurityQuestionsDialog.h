#pragma once

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QStringList>

/**
 * SecurityQuestionsDialog
 *
 * Modo Setup   → configurar 3 perguntas de segurança (primeira vez)
 * Modo Recover → responder perguntas para redefinir a senha do app
 */
class SecurityQuestionsDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Mode { Setup, Recover };

    explicit SecurityQuestionsDialog(Mode mode, QWidget* parent = nullptr);

    // Perguntas disponíveis
    static QStringList availableQuestions();

    // Recuperação já configurada?
    static bool hasRecoverySetup();

    // Índices das perguntas salvas (3 valores)
    static QList<int> savedQuestionIndices();

    // Salva configuração de recuperação
    static void saveRecovery(const QList<int>& indices, const QStringList& answers);

    // Verifica se as respostas estão corretas
    static bool verifyAnswers(const QStringList& answers, const QList<int>& indices);

private slots:
    void onConfirm();

private:
    void setupUiSetup();
    void setupUiRecover();
    void showNewPasswordFields();

    static QString hashAnswer(const QString& answer);
    static QString configPath();

    Mode m_mode;

    struct QuestionRow {
        QComboBox* combo = nullptr;   // só no modo Setup
        QLabel*    label = nullptr;   // só no modo Recover
        QLineEdit* edit  = nullptr;
    };
    QuestionRow  m_rows[3];

    // Campos nova senha (modo Recover — aparece após validação)
    QLineEdit*   m_newPassEdit     = nullptr;
    QLineEdit*   m_newPassConfirm  = nullptr;
    QLabel*      m_newPassLabel    = nullptr;
    QLabel*      m_newPassConfLbl  = nullptr;
    bool         m_answersVerified = false;

    QLabel*      m_errorLabel  = nullptr;
    QPushButton* m_btnConfirm  = nullptr;
};
