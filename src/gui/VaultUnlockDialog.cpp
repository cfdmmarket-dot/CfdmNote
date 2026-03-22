#include "VaultUnlockDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QFont>
#include <QCoreApplication>

#include "core/Database.h"
#include "keys/CompositeKey.h"
#include "keys/PasswordKey.h"
#include "keys/FileKey.h"
#include "gui/MainWindow.h"
#include "gui/DatabaseWidget.h"
#include "gui/DatabaseTabWidget.h"

VaultUnlockDialog::VaultUnlockDialog(const QString& vaultPath,
                                      MainWindow* mainWindow,
                                      QWidget* parent)
    : QDialog(parent)
    , m_vaultPath(vaultPath)
    , m_mainWindow(mainWindow)
{
    setWindowTitle(tr("Abrir Cofre — CfdmNote"));
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint);
    setFixedWidth(400);
    setModal(true);
    setupUi();
}

void VaultUnlockDialog::setupUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(28, 24, 28, 24);

    // ── Versão (esquerda, fonte pequena) ─────────────────────────────────
    auto* versionLabel = new QLabel(
        QString("CfdmNote - v%1").arg(QCoreApplication::applicationVersion()));
    versionLabel->setStyleSheet("color: gray; font-size: 10px;");
    versionLabel->setAlignment(Qt::AlignLeft);
    layout->addWidget(versionLabel);

    layout->addSpacing(6);

    // ── Senha ─────────────────────────────────────────────────────────────
    auto* passLabel = new QLabel(tr("Senha:"));
    passLabel->setStyleSheet("font-size: 12px;");
    layout->addWidget(passLabel);

    auto* passRow = new QHBoxLayout();
    m_passEdit = new QLineEdit();
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passEdit->setPlaceholderText(tr("Senha do cofre (deixe vazio se não tiver)"));
    m_passEdit->setFixedHeight(34);
    connect(m_passEdit, &QLineEdit::returnPressed, this, &VaultUnlockDialog::onUnlock);

    m_btnToggle = new QPushButton("👁");
    m_btnToggle->setFixedSize(34, 34);
    m_btnToggle->setCheckable(true);
    connect(m_btnToggle, &QPushButton::toggled, this, &VaultUnlockDialog::onToggleVisibility);

    passRow->addWidget(m_passEdit);
    passRow->addWidget(m_btnToggle);
    layout->addLayout(passRow);

    layout->addSpacing(4);

    // ── Arquivo chave ─────────────────────────────────────────────────────
    auto* keyLabel = new QLabel(tr("Arquivo chave (opcional):"));
    keyLabel->setStyleSheet("font-size: 12px;");
    layout->addWidget(keyLabel);

    auto* keyRow = new QHBoxLayout();
    m_keyFileEdit = new QLineEdit();
    m_keyFileEdit->setPlaceholderText(tr("Nenhum arquivo chave selecionado"));
    m_keyFileEdit->setFixedHeight(34);
    m_keyFileEdit->setReadOnly(true);
    m_keyFileEdit->setStyleSheet("color: gray; font-size: 11px;");

    m_btnKeyBrowse = new QPushButton(tr("Procurar..."));
    m_btnKeyBrowse->setFixedHeight(34);
    connect(m_btnKeyBrowse, &QPushButton::clicked, this, &VaultUnlockDialog::onBrowseKeyFile);

    m_btnKeyClear = new QPushButton("✕");
    m_btnKeyClear->setFixedSize(34, 34);
    m_btnKeyClear->setToolTip(tr("Remover arquivo chave"));
    m_btnKeyClear->setEnabled(false);
    connect(m_btnKeyClear, &QPushButton::clicked, this, &VaultUnlockDialog::onClearKeyFile);

    keyRow->addWidget(m_keyFileEdit);
    keyRow->addWidget(m_btnKeyBrowse);
    keyRow->addWidget(m_btnKeyClear);
    layout->addLayout(keyRow);

    layout->addSpacing(6);

    // ── Erro ──────────────────────────────────────────────────────────────
    m_errorLabel = new QLabel();
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setStyleSheet("color: #e05050; font-size: 12px;");
    m_errorLabel->setWordWrap(true);
    layout->addWidget(m_errorLabel);

    // ── Botões ────────────────────────────────────────────────────────────
    m_btnUnlock = new QPushButton(tr("Abrir Cofre"));
    m_btnUnlock->setFixedHeight(38);
    m_btnUnlock->setDefault(true);
    m_btnUnlock->setStyleSheet(
        "QPushButton { background: #2a5caa; color: white; border-radius: 5px; font-weight: bold; }"
        "QPushButton:hover { background: #3a6cba; }"
        "QPushButton:pressed { background: #1a4c9a; }"
    );
    connect(m_btnUnlock, &QPushButton::clicked, this, &VaultUnlockDialog::onUnlock);
    layout->addWidget(m_btnUnlock);

    auto* btnCancel = new QPushButton(tr("Cancelar"));
    btnCancel->setFlat(true);
    btnCancel->setFixedHeight(28);
    btnCancel->setStyleSheet("color: gray;");
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    layout->addWidget(btnCancel, 0, Qt::AlignCenter);

    m_passEdit->setFocus();
}

// ─── Slots ────────────────────────────────────────────────────────────────────

void VaultUnlockDialog::onUnlock()
{
    const QString password   = m_passEdit->text();
    const QString keyFilePath = m_keyFileEdit->text();

    // Pelo menos um dos dois deve estar preenchido
    if (password.isEmpty() && keyFilePath.isEmpty()) {
        m_errorLabel->setText(tr("Informe a senha ou selecione um arquivo chave."));
        return;
    }

    auto key = QSharedPointer<CompositeKey>::create();

    if (!password.isEmpty()) {
        key->addKey(QSharedPointer<PasswordKey>::create(password));
    }

    if (!keyFilePath.isEmpty()) {
        auto fileKey = QSharedPointer<FileKey>::create();
        QString keyError;
        if (!fileKey->load(keyFilePath, &keyError)) {
            m_errorLabel->setText(tr("Erro ao carregar arquivo chave: %1").arg(keyError));
            return;
        }
        key->addKey(fileKey);
    }

    auto db = QSharedPointer<Database>::create(m_vaultPath);
    QString error;
    if (!db->open(key, &error)) {
        m_errorLabel->setText(tr("Não foi possível abrir o cofre. Verifique a senha e o arquivo chave."));
        m_passEdit->clear();
        m_passEdit->setFocus();
        return;
    }

    if (injectDatabase(db)) {
        accept();
    } else {
        m_errorLabel->setText(tr("Erro ao carregar cofre na janela principal."));
    }
}

void VaultUnlockDialog::onToggleVisibility()
{
    m_passEdit->setEchoMode(m_btnToggle->isChecked() ? QLineEdit::Normal : QLineEdit::Password);
}

void VaultUnlockDialog::onBrowseKeyFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        tr("Selecionar Arquivo Chave"),
        QDir::homePath(),
        tr("Arquivos chave (*.key *.keyx);;Todos os arquivos (*)")
    );
    if (!path.isEmpty()) {
        m_keyFileEdit->setText(path);
        m_keyFileEdit->setStyleSheet("font-size: 11px;");
        m_btnKeyClear->setEnabled(true);
    }
}

void VaultUnlockDialog::onClearKeyFile()
{
    m_keyFileEdit->clear();
    m_keyFileEdit->setStyleSheet("color: gray; font-size: 11px;");
    m_btnKeyClear->setEnabled(false);
}

// ─── Injeção no DatabaseTabWidget ────────────────────────────────────────────

bool VaultUnlockDialog::injectDatabase(QSharedPointer<Database> db)
{
    if (!m_mainWindow) return false;
    auto* tabWidget = m_mainWindow->findChild<DatabaseTabWidget*>();
    if (!tabWidget) return false;
    auto* dbWidget = new DatabaseWidget(db, tabWidget);
    tabWidget->addDatabaseTab(dbWidget);
    return true;
}
