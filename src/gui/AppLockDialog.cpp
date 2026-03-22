#include "AppLockDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QSettings>
#include <QPixmap>
#include <QFile>
#include <QKeyEvent>
#include <QCoreApplication>

static const QString SETTINGS_KEY = "app/passwordHash";

AppLockDialog::AppLockDialog(QWidget* parent)
    : QDialog(parent)
{
    m_firstRun = isFirstRun();
    setWindowTitle("CfdmNote");
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint);
    setFixedWidth(360);
    setModal(true);
    setupUi(m_firstRun);
}

void AppLockDialog::setupUi(bool firstRun)
{
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(14);
    layout->setContentsMargins(32, 28, 32, 28);

    // ── Logo ──────────────────────────────────────────────────────────────
    m_logoLabel = new QLabel();
    m_logoLabel->setAlignment(Qt::AlignCenter);
    const QString iconPath = QStandardPaths::locate(
        QStandardPaths::AppDataLocation, "icons/cfdmnote_128.png");
    // Tenta caminho relativo ao binário
    const QString altPath = QCoreApplication::applicationDirPath()
                            + "/../resources/icons/cfdmnote_128.png";
    QPixmap logo;
    if (!iconPath.isEmpty())
        logo.load(iconPath);
    else if (QFile::exists(altPath))
        logo.load(altPath);
    if (!logo.isNull())
        m_logoLabel->setPixmap(logo.scaled(72, 72, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout->addWidget(m_logoLabel);

    // ── Título ────────────────────────────────────────────────────────────
    m_titleLabel = new QLabel("CfdmNote");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    layout->addWidget(m_titleLabel);

    m_subtitleLabel = new QLabel(firstRun
        ? tr("Crie uma senha para proteger o CfdmNote")
        : tr("Digite sua senha para continuar"));
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    m_subtitleLabel->setStyleSheet("color: gray; font-size: 12px;");
    m_subtitleLabel->setWordWrap(true);
    layout->addWidget(m_subtitleLabel);

    layout->addSpacing(4);

    // ── Campo senha ───────────────────────────────────────────────────────
    auto* passRow = new QHBoxLayout();
    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(firstRun ? tr("Nova senha") : tr("Senha"));
    m_passwordEdit->setFixedHeight(36);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &AppLockDialog::onUnlock);

    m_btnToggle = new QPushButton("👁");
    m_btnToggle->setFixedSize(36, 36);
    m_btnToggle->setCheckable(true);
    connect(m_btnToggle, &QPushButton::toggled, this, &AppLockDialog::onToggleVisibility);

    passRow->addWidget(m_passwordEdit);
    passRow->addWidget(m_btnToggle);
    layout->addLayout(passRow);

    // ── Confirmar senha (só na primeira vez) ──────────────────────────────
    if (firstRun) {
        m_confirmEdit = new QLineEdit();
        m_confirmEdit->setEchoMode(QLineEdit::Password);
        m_confirmEdit->setPlaceholderText(tr("Confirme a senha"));
        m_confirmEdit->setFixedHeight(36);
        connect(m_confirmEdit, &QLineEdit::returnPressed, this, &AppLockDialog::onUnlock);
        layout->addWidget(m_confirmEdit);
    }

    // ── Erro ──────────────────────────────────────────────────────────────
    m_errorLabel = new QLabel();
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setStyleSheet("color: #e05050; font-size: 12px;");
    m_errorLabel->setWordWrap(true);
    layout->addWidget(m_errorLabel);

    // ── Botão ─────────────────────────────────────────────────────────────
    m_btnUnlock = new QPushButton(firstRun ? tr("Criar senha e entrar") : tr("Desbloquear"));
    m_btnUnlock->setFixedHeight(38);
    m_btnUnlock->setDefault(true);
    m_btnUnlock->setStyleSheet(
        "QPushButton { background: #2a5caa; color: white; border-radius: 5px; font-weight: bold; }"
        "QPushButton:hover { background: #3a6cba; }"
        "QPushButton:pressed { background: #1a4c9a; }"
    );
    connect(m_btnUnlock, &QPushButton::clicked, this, &AppLockDialog::onUnlock);
    layout->addWidget(m_btnUnlock);

    m_passwordEdit->setFocus();
}

void AppLockDialog::onUnlock()
{
    const QString password = m_passwordEdit->text();

    if (password.isEmpty()) {
        m_errorLabel->setText(tr("Digite uma senha."));
        return;
    }

    if (m_firstRun) {
        // Criando senha pela primeira vez
        if (m_confirmEdit && m_confirmEdit->text() != password) {
            m_errorLabel->setText(tr("As senhas não coincidem."));
            m_confirmEdit->clear();
            m_confirmEdit->setFocus();
            return;
        }
        if (password.length() < 4) {
            m_errorLabel->setText(tr("Senha muito curta (mínimo 4 caracteres)."));
            return;
        }
        savePassword(password);
        accept();
    } else {
        // Verificando senha existente
        if (checkPassword(password)) {
            accept();
        } else {
            m_errorLabel->setText(tr("Senha incorreta. Tente novamente."));
            m_passwordEdit->clear();
            m_passwordEdit->setFocus();
        }
    }
}

void AppLockDialog::onToggleVisibility()
{
    const bool show = m_btnToggle->isChecked();
    m_passwordEdit->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
    if (m_confirmEdit)
        m_confirmEdit->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
}

// ─── Estáticos ───────────────────────────────────────────────────────────────

QString AppLockDialog::configPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + "/cfdmnote.conf";
}

bool AppLockDialog::isFirstRun()
{
    QSettings s(configPath(), QSettings::IniFormat);
    return s.value(SETTINGS_KEY).toString().isEmpty();
}

QString AppLockDialog::hashPassword(const QString& password)
{
    // SHA-256 com salt fixo do app
    const QByteArray salted = "CfdmNote::AppLock::" + password.toUtf8();
    return QCryptographicHash::hash(salted, QCryptographicHash::Sha256).toHex();
}

bool AppLockDialog::checkPassword(const QString& password)
{
    QSettings s(configPath(), QSettings::IniFormat);
    const QString stored = s.value(SETTINGS_KEY).toString();
    return !stored.isEmpty() && stored == hashPassword(password);
}

void AppLockDialog::savePassword(const QString& password)
{
    QSettings s(configPath(), QSettings::IniFormat);
    s.setValue(SETTINGS_KEY, hashPassword(password));
    s.sync();
}
