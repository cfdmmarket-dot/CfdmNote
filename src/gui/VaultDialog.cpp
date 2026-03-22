#include "VaultDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFile>

// Motor de banco criptografado do CfdmNote
#include "core/Database.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "format/Kdbx4Writer.h"
#include "crypto/kdf/Argon2Kdf.h"
#include "keys/CompositeKey.h"
#include "keys/PasswordKey.h"

VaultDialog::VaultDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Criar Cofre Criptografado"));
    setMinimumWidth(480);
    setModal(true);
    setupUi();
}

void VaultDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // ── Cabeçalho ─────────────────────────────────────────────────────────
    auto* headerLabel = new QLabel(
        tr("<b>Novo Cofre Criptografado — CfdmNote</b><br>"
           "<small>Cria um cofre .kdbx protegido por senha mestra (AES-256 + Argon2).<br>"
           "Compatível com outros gerenciadores de senha no formato .kdbx.</small>")
    );
    headerLabel->setWordWrap(true);
    mainLayout->addWidget(headerLabel);

    // ── Informações do cofre ───────────────────────────────────────────────
    auto* infoGroup = new QGroupBox(tr("Cofre"));
    auto* formLayout = new QFormLayout(infoGroup);

    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText(tr("ex: Trabalho, Pessoal, Finanças..."));
    formLayout->addRow(tr("Nome:"), m_nameEdit);

    auto* pathLayout = new QHBoxLayout();
    m_pathEdit = new QLineEdit();
    m_pathEdit->setPlaceholderText(tr("Caminho para salvar o .kdbx"));
    m_btnBrowse = new QPushButton(tr("..."));
    m_btnBrowse->setFixedWidth(32);
    connect(m_btnBrowse, &QPushButton::clicked, this, &VaultDialog::onBrowse);
    pathLayout->addWidget(m_pathEdit);
    pathLayout->addWidget(m_btnBrowse);
    formLayout->addRow(tr("Arquivo:"), pathLayout);

    m_encryptionCombo = new QComboBox();
    m_encryptionCombo->addItem(tr("AES-256 + Argon2d (recomendado)"), "argon2d");
    m_encryptionCombo->addItem(tr("AES-256 + Argon2id"),              "argon2id");
    m_encryptionCombo->addItem(tr("AES-256 + AES-KDF"),               "aes-kdf");
    formLayout->addRow(tr("Criptografia:"), m_encryptionCombo);

    mainLayout->addWidget(infoGroup);

    // ── Senha mestra ──────────────────────────────────────────────────────
    auto* passGroup = new QGroupBox(tr("Senha Mestra"));
    auto* passLayout = new QFormLayout(passGroup);

    auto* passRow = new QHBoxLayout();
    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(tr("Digite a senha mestra"));
    m_btnShowPass = new QPushButton(tr("👁"));
    m_btnShowPass->setFixedWidth(32);
    m_btnShowPass->setCheckable(true);
    connect(m_btnShowPass, &QPushButton::toggled, this, &VaultDialog::onPasswordToggle);
    passRow->addWidget(m_passwordEdit);
    passRow->addWidget(m_btnShowPass);
    passLayout->addRow(tr("Senha:"), passRow);

    m_confirmEdit = new QLineEdit();
    m_confirmEdit->setEchoMode(QLineEdit::Password);
    m_confirmEdit->setPlaceholderText(tr("Confirme a senha mestra"));
    passLayout->addRow(tr("Confirmar:"), m_confirmEdit);

    // Indicador de força da senha
    m_strengthBar = new QProgressBar();
    m_strengthBar->setRange(0, 100);
    m_strengthBar->setValue(0);
    m_strengthBar->setTextVisible(false);
    m_strengthBar->setFixedHeight(6);
    m_strengthLabel = new QLabel(tr("Senha não informada"));
    m_strengthLabel->setStyleSheet("color: gray; font-size: 11px;");
    passLayout->addRow(tr("Força:"), m_strengthBar);
    passLayout->addRow(QString(), m_strengthLabel);

    connect(m_passwordEdit, &QLineEdit::textChanged, this, &VaultDialog::validateForm);
    connect(m_confirmEdit,  &QLineEdit::textChanged, this, &VaultDialog::validateForm);
    connect(m_nameEdit,     &QLineEdit::textChanged, this, &VaultDialog::validateForm);
    connect(m_pathEdit,     &QLineEdit::textChanged, this, &VaultDialog::validateForm);

    mainLayout->addWidget(passGroup);

    // ── Status ────────────────────────────────────────────────────────────
    m_statusLabel = new QLabel();
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("color: red; font-size: 11px;");
    mainLayout->addWidget(m_statusLabel);

    // ── Botões ────────────────────────────────────────────────────────────
    auto* btnLayout = new QHBoxLayout();
    m_btnCancel = new QPushButton(tr("Cancelar"));
    m_btnCreate = new QPushButton(tr("Criar Cofre"));
    m_btnCreate->setDefault(true);
    m_btnCreate->setEnabled(false);
    m_btnCreate->setStyleSheet("QPushButton { font-weight: bold; }");

    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_btnCreate, &QPushButton::clicked, this, &VaultDialog::onCreate);

    btnLayout->addStretch();
    btnLayout->addWidget(m_btnCancel);
    btnLayout->addWidget(m_btnCreate);
    mainLayout->addLayout(btnLayout);
}

void VaultDialog::onBrowse()
{
    const QString name = m_nameEdit->text().trimmed();
    const QString defaultName = name.isEmpty() ? "cofre" : name.toLower().replace(' ', '_');

    QString path = QFileDialog::getSaveFileName(
        this,
        tr("Salvar cofre como"),
        QDir::homePath() + "/" + defaultName + ".kdbx",
        tr("Cofre CfdmNote (*.kdbx)")
    );
    if (!path.isEmpty()) {
        if (!path.endsWith(".kdbx")) path += ".kdbx";
        m_pathEdit->setText(path);
    }
}

void VaultDialog::validateForm()
{
    const QString pass    = m_passwordEdit->text();
    const QString confirm = m_confirmEdit->text();
    const QString name    = m_nameEdit->text().trimmed();
    const QString path    = m_pathEdit->text().trimmed();

    // Força da senha (estimativa simples)
    int strength = 0;
    if (pass.length() >= 8)  strength += 25;
    if (pass.length() >= 16) strength += 25;
    if (pass.contains(QRegularExpression("[A-Z]"))) strength += 15;
    if (pass.contains(QRegularExpression("[0-9]"))) strength += 15;
    if (pass.contains(QRegularExpression("[^a-zA-Z0-9]"))) strength += 20;

    m_strengthBar->setValue(strength);

    QString color, label;
    if (pass.isEmpty())      { color = "gray";   label = tr("Senha não informada"); }
    else if (strength < 30)  { color = "red";    label = tr("Muito fraca"); }
    else if (strength < 50)  { color = "orange"; label = tr("Fraca"); }
    else if (strength < 75)  { color = "gold";   label = tr("Moderada"); }
    else                     { color = "green";  label = tr("Forte"); }

    m_strengthBar->setStyleSheet(
        QString("QProgressBar::chunk { background: %1; border-radius: 3px; }").arg(color)
    );
    m_strengthLabel->setText(label);
    m_strengthLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(color));

    // Validação
    bool ok = !name.isEmpty() && !path.isEmpty() && !pass.isEmpty() && pass == confirm;
    m_btnCreate->setEnabled(ok);

    if (!pass.isEmpty() && pass != confirm)
        m_statusLabel->setText(tr("As senhas não coincidem."));
    else if (name.isEmpty())
        m_statusLabel->setText(tr("Informe um nome para o cofre."));
    else if (path.isEmpty())
        m_statusLabel->setText(tr("Escolha onde salvar o arquivo .kdbx."));
    else
        m_statusLabel->clear();
}

void VaultDialog::onPasswordToggle()
{
    const bool show = m_btnShowPass->isChecked();
    m_passwordEdit->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
    m_confirmEdit->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
}

void VaultDialog::onCreate()
{
    const QString name     = m_nameEdit->text().trimmed();
    const QString path     = m_pathEdit->text().trimmed();
    const QString password = m_passwordEdit->text();

    m_btnCreate->setEnabled(false);
    m_btnCreate->setText(tr("Criando..."));

    if (createVault(path, password)) {
        m_vaultPath = path;
        QMessageBox::information(this,
            tr("Cofre criado"),
            tr("<b>Cofre '%1' criado com sucesso!</b><br><br>"
               "Arquivo: %2<br><br>"
               "Guarde a senha mestra em local seguro.<br>"
               "<b>Não é possível recuperar o cofre sem ela.</b>")
               .arg(name, path)
        );
        accept();
    } else {
        m_statusLabel->setText(tr("Erro ao criar o cofre. Verifique o caminho e tente novamente."));
        m_btnCreate->setEnabled(true);
        m_btnCreate->setText(tr("Criar Cofre"));
    }
}

bool VaultDialog::createVault(const QString& filePath, const QString& password)
{
    // Cria cofre criptografado usando motor interno do CfdmNote
    auto db = QSharedPointer<Database>::create();

    // Grupo raiz com o nome do cofre
    db->rootGroup()->setName(m_nameEdit->text().trimmed());
    db->metadata()->setName(m_nameEdit->text().trimmed());

    // Chave composta: apenas senha por enquanto
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create(password));

    // Argon2d KDF (padrão de segurança do CfdmNote)
    auto kdf = QSharedPointer<Argon2Kdf>::create(Argon2Kdf::Type::Argon2d);
    kdf->setRounds(8);
    kdf->setMemory(65536);   // 64 MB
    kdf->setParallelism(2);

    db->setKey(key);
    db->setKdf(kdf);

    // Salva cofre criptografado (formato .kdbx v4)
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return false;

    Kdbx4Writer writer;
    const bool ok = writer.writeDatabase(&file, db.data());
    file.close();

    return ok;
}
