#include "SecurityQuestionsDialog.h"
#include "AppLockDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QCryptographicHash>
#include <QSettings>
#include <QStandardPaths>
#include <QMessageBox>

static const QString KEY_Q1 = "recovery/q1";
static const QString KEY_Q2 = "recovery/q2";
static const QString KEY_Q3 = "recovery/q3";
static const QString KEY_A1 = "recovery/a1";
static const QString KEY_A2 = "recovery/a2";
static const QString KEY_A3 = "recovery/a3";

// ─── Lista de perguntas ───────────────────────────────────────────────────────

QStringList SecurityQuestionsDialog::availableQuestions()
{
    return {
        tr("Nome do seu primeiro animal de estimação"),
        tr("Apelido da sua infância"),
        tr("Escola onde estudou pela primeira vez"),
        tr("Nome do seu melhor amigo de infância"),
        tr("Cidade onde você nasceu"),
        tr("Nome do seu time favorito"),
        tr("Prato favorito da sua mãe"),
        tr("Primeiro carro que você teve"),
        tr("Nome do seu professor(a) favorito(a)"),
        tr("Rua onde você cresceu"),
        tr("Nome do hospital onde nasceu"),
        tr("Primeiro emprego que você teve"),
    };
}

// ─── Construtor ───────────────────────────────────────────────────────────────

SecurityQuestionsDialog::SecurityQuestionsDialog(Mode mode, QWidget* parent)
    : QDialog(parent), m_mode(mode)
{
    setModal(true);
    setFixedWidth(420);

    if (m_mode == Mode::Setup)
        setupUiSetup();
    else
        setupUiRecover();
}

// ─── UI — Modo Setup ──────────────────────────────────────────────────────────

void SecurityQuestionsDialog::setupUiSetup()
{
    setWindowTitle(tr("Perguntas de Segurança"));

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    layout->setContentsMargins(28, 24, 28, 24);

    auto* title = new QLabel(tr("<b>Configure sua recuperação de acesso</b>"));
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto* sub = new QLabel(tr("Escolha 3 perguntas e responda.\n"
                               "Se esquecer a senha, use estas perguntas para recuperar o acesso."));
    sub->setAlignment(Qt::AlignCenter);
    sub->setWordWrap(true);
    sub->setStyleSheet("color: gray; font-size: 12px;");
    layout->addWidget(sub);

    layout->addSpacing(8);

    const QStringList questions = availableQuestions();

    for (int i = 0; i < 3; ++i) {
        auto* rowLabel = new QLabel(tr("Pergunta %1").arg(i + 1));
        rowLabel->setStyleSheet("font-weight: bold;");
        layout->addWidget(rowLabel);

        m_rows[i].combo = new QComboBox();
        for (const QString& q : questions)
            m_rows[i].combo->addItem(q);
        m_rows[i].combo->setCurrentIndex(i);  // pré-seleciona índices diferentes
        layout->addWidget(m_rows[i].combo);

        m_rows[i].edit = new QLineEdit();
        m_rows[i].edit->setPlaceholderText(tr("Sua resposta"));
        m_rows[i].edit->setFixedHeight(34);
        layout->addWidget(m_rows[i].edit);

        if (i < 2) layout->addSpacing(4);
    }

    layout->addSpacing(8);

    m_errorLabel = new QLabel();
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setStyleSheet("color: #e05050; font-size: 12px;");
    m_errorLabel->setWordWrap(true);
    layout->addWidget(m_errorLabel);

    m_btnConfirm = new QPushButton(tr("Salvar perguntas"));
    m_btnConfirm->setFixedHeight(36);
    m_btnConfirm->setDefault(true);
    m_btnConfirm->setStyleSheet(
        "QPushButton { background: #2a5caa; color: white; border-radius: 5px; font-weight: bold; }"
        "QPushButton:hover { background: #3a6cba; }"
        "QPushButton:pressed { background: #1a4c9a; }"
    );
    connect(m_btnConfirm, &QPushButton::clicked, this, &SecurityQuestionsDialog::onConfirm);
    layout->addWidget(m_btnConfirm);

    // Botão pular
    auto* btnSkip = new QPushButton(tr("Pular (não recomendado)"));
    btnSkip->setFixedHeight(30);
    btnSkip->setStyleSheet("color: gray; border: none; text-decoration: underline;");
    btnSkip->setFlat(true);
    connect(btnSkip, &QPushButton::clicked, this, &QDialog::reject);
    layout->addWidget(btnSkip);
}

// ─── UI — Modo Recover ────────────────────────────────────────────────────────

void SecurityQuestionsDialog::setupUiRecover()
{
    setWindowTitle(tr("Recuperar Acesso — CfdmNote"));

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    layout->setContentsMargins(28, 24, 28, 24);

    auto* title = new QLabel(tr("<b>Recuperar Acesso</b>"));
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto* sub = new QLabel(tr("Responda as 3 perguntas para redefinir sua senha."));
    sub->setAlignment(Qt::AlignCenter);
    sub->setWordWrap(true);
    sub->setStyleSheet("color: gray; font-size: 12px;");
    layout->addWidget(sub);

    layout->addSpacing(8);

    const QStringList questions    = availableQuestions();
    const QList<int>  savedIndices = savedQuestionIndices();

    for (int i = 0; i < 3; ++i) {
        const int idx = (i < savedIndices.size()) ? savedIndices[i] : 0;
        const QString questionText = (idx >= 0 && idx < questions.size())
                                     ? questions[idx]
                                     : tr("Pergunta %1").arg(i + 1);

        m_rows[i].label = new QLabel(QString("<b>%1.</b> %2").arg(i + 1).arg(questionText));
        m_rows[i].label->setWordWrap(true);
        layout->addWidget(m_rows[i].label);

        m_rows[i].edit = new QLineEdit();
        m_rows[i].edit->setPlaceholderText(tr("Sua resposta"));
        m_rows[i].edit->setFixedHeight(34);
        layout->addWidget(m_rows[i].edit);

        if (i < 2) layout->addSpacing(4);
    }

    layout->addSpacing(8);

    // Campos nova senha (ocultos até acertar as respostas)
    m_newPassLabel = new QLabel(tr("<b>Nova senha do app:</b>"));
    m_newPassLabel->setVisible(false);
    layout->addWidget(m_newPassLabel);

    m_newPassEdit = new QLineEdit();
    m_newPassEdit->setEchoMode(QLineEdit::Password);
    m_newPassEdit->setPlaceholderText(tr("Nova senha"));
    m_newPassEdit->setFixedHeight(34);
    m_newPassEdit->setVisible(false);
    layout->addWidget(m_newPassEdit);

    m_newPassConfLbl = new QLabel(tr("Confirme a nova senha:"));
    m_newPassConfLbl->setVisible(false);
    layout->addWidget(m_newPassConfLbl);

    m_newPassConfirm = new QLineEdit();
    m_newPassConfirm->setEchoMode(QLineEdit::Password);
    m_newPassConfirm->setPlaceholderText(tr("Confirme"));
    m_newPassConfirm->setFixedHeight(34);
    m_newPassConfirm->setVisible(false);
    layout->addWidget(m_newPassConfirm);

    m_errorLabel = new QLabel();
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setStyleSheet("color: #e05050; font-size: 12px;");
    m_errorLabel->setWordWrap(true);
    layout->addWidget(m_errorLabel);

    m_btnConfirm = new QPushButton(tr("Verificar respostas"));
    m_btnConfirm->setFixedHeight(36);
    m_btnConfirm->setDefault(true);
    m_btnConfirm->setStyleSheet(
        "QPushButton { background: #2a5caa; color: white; border-radius: 5px; font-weight: bold; }"
        "QPushButton:hover { background: #3a6cba; }"
        "QPushButton:pressed { background: #1a4c9a; }"
    );
    connect(m_btnConfirm, &QPushButton::clicked, this, &SecurityQuestionsDialog::onConfirm);
    layout->addWidget(m_btnConfirm);
}

// ─── Slot confirm ─────────────────────────────────────────────────────────────

void SecurityQuestionsDialog::onConfirm()
{
    if (m_mode == Mode::Setup) {
        // Valida que as 3 perguntas são diferentes
        const int i0 = m_rows[0].combo->currentIndex();
        const int i1 = m_rows[1].combo->currentIndex();
        const int i2 = m_rows[2].combo->currentIndex();

        if (i0 == i1 || i0 == i2 || i1 == i2) {
            m_errorLabel->setText(tr("Escolha 3 perguntas diferentes."));
            return;
        }

        // Valida que todas as respostas foram preenchidas
        for (int i = 0; i < 3; ++i) {
            if (m_rows[i].edit->text().trimmed().isEmpty()) {
                m_errorLabel->setText(tr("Preencha todas as respostas."));
                return;
            }
        }

        saveRecovery(
            {i0, i1, i2},
            { m_rows[0].edit->text(), m_rows[1].edit->text(), m_rows[2].edit->text() }
        );
        accept();

    } else {
        // Modo Recover
        if (!m_answersVerified) {
            // Fase 1: verificar respostas
            QStringList answers;
            for (int i = 0; i < 3; ++i)
                answers << m_rows[i].edit->text();

            const QList<int> indices = savedQuestionIndices();

            if (!verifyAnswers(answers, indices)) {
                m_errorLabel->setText(tr("Uma ou mais respostas estão incorretas."));
                for (int i = 0; i < 3; ++i)
                    m_rows[i].edit->clear();
                m_rows[0].edit->setFocus();
                return;
            }

            // Respostas corretas — mostra campos de nova senha
            m_answersVerified = true;
            m_errorLabel->clear();
            showNewPasswordFields();

        } else {
            // Fase 2: redefinir senha
            const QString newPass = m_newPassEdit->text();
            const QString confirm = m_newPassConfirm->text();

            if (newPass.length() < 4) {
                m_errorLabel->setText(tr("Senha muito curta (mínimo 4 caracteres)."));
                return;
            }
            if (newPass != confirm) {
                m_errorLabel->setText(tr("As senhas não coincidem."));
                m_newPassConfirm->clear();
                m_newPassConfirm->setFocus();
                return;
            }

            AppLockDialog::savePassword(newPass);
            QMessageBox::information(this, tr("Acesso recuperado"),
                tr("Senha redefinida com sucesso! Faça login com a nova senha."));
            accept();
        }
    }
}

void SecurityQuestionsDialog::showNewPasswordFields()
{
    m_newPassLabel->setVisible(true);
    m_newPassEdit->setVisible(true);
    m_newPassConfLbl->setVisible(true);
    m_newPassConfirm->setVisible(true);

    // Desabilita campos de resposta
    for (int i = 0; i < 3; ++i)
        m_rows[i].edit->setEnabled(false);

    m_btnConfirm->setText(tr("Redefinir senha"));
    adjustSize();
    m_newPassEdit->setFocus();
}

// ─── Estáticos ────────────────────────────────────────────────────────────────

QString SecurityQuestionsDialog::configPath()
{
    return AppLockDialog::configPath();
}

QString SecurityQuestionsDialog::hashAnswer(const QString& answer)
{
    const QByteArray salted = "CfdmNote::Recovery::" + answer.trimmed().toLower().toUtf8();
    return QCryptographicHash::hash(salted, QCryptographicHash::Sha256).toHex();
}

bool SecurityQuestionsDialog::hasRecoverySetup()
{
    QSettings s(configPath(), QSettings::IniFormat);
    return !s.value(KEY_A1).toString().isEmpty();
}

QList<int> SecurityQuestionsDialog::savedQuestionIndices()
{
    QSettings s(configPath(), QSettings::IniFormat);
    return {
        s.value(KEY_Q1, 0).toInt(),
        s.value(KEY_Q2, 1).toInt(),
        s.value(KEY_Q3, 2).toInt(),
    };
}

void SecurityQuestionsDialog::saveRecovery(const QList<int>& indices, const QStringList& answers)
{
    QSettings s(configPath(), QSettings::IniFormat);
    s.setValue(KEY_Q1, indices[0]);
    s.setValue(KEY_Q2, indices[1]);
    s.setValue(KEY_Q3, indices[2]);
    s.setValue(KEY_A1, hashAnswer(answers[0]));
    s.setValue(KEY_A2, hashAnswer(answers[1]));
    s.setValue(KEY_A3, hashAnswer(answers[2]));
    s.sync();
}

bool SecurityQuestionsDialog::verifyAnswers(const QStringList& answers, const QList<int>& /*indices*/)
{
    if (answers.size() < 3) return false;
    QSettings s(configPath(), QSettings::IniFormat);

    const QString a1 = s.value(KEY_A1).toString();
    const QString a2 = s.value(KEY_A2).toString();
    const QString a3 = s.value(KEY_A3).toString();

    if (a1.isEmpty() || a2.isEmpty() || a3.isEmpty()) return false;

    return hashAnswer(answers[0]) == a1
        && hashAnswer(answers[1]) == a2
        && hashAnswer(answers[2]) == a3;
}
