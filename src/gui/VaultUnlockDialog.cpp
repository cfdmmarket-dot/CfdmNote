#include "VaultUnlockDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QFont>

#include "core/Database.h"
#include "keys/CompositeKey.h"
#include "keys/PasswordKey.h"
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
    setFixedWidth(380);
    setModal(true);
    setupUi();
}

void VaultUnlockDialog::setupUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    layout->setContentsMargins(28, 24, 28, 24);

    // ── Título ────────────────────────────────────────────────────────────
    auto* titleLabel = new QLabel(tr("<b>Abrir Cofre Criptografado</b>"));
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont f = titleLabel->font();
    f.setPointSize(13);
    titleLabel->setFont(f);
    layout->addWidget(titleLabel);

    // ── Arquivo ───────────────────────────────────────────────────────────
    const QString fileName = QFileInfo(m_vaultPath).fileName();
    m_pathLabel = new QLabel(QString("<span style='color:gray; font-size:11px;'>%1</span>")
                              .arg(fileName));
    m_pathLabel->setAlignment(Qt::AlignCenter);
    m_pathLabel->setWordWrap(true);
    layout->addWidget(m_pathLabel);

    layout->addSpacing(6);

    // ── Campo senha ───────────────────────────────────────────────────────
    auto* passRow = new QHBoxLayout();

    m_passEdit = new QLineEdit();
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passEdit->setPlaceholderText(tr("Senha do cofre"));
    m_passEdit->setFixedHeight(36);
    connect(m_passEdit, &QLineEdit::returnPressed, this, &VaultUnlockDialog::onUnlock);

    m_btnToggle = new QPushButton("👁");
    m_btnToggle->setFixedSize(36, 36);
    m_btnToggle->setCheckable(true);
    connect(m_btnToggle, &QPushButton::toggled, this, &VaultUnlockDialog::onToggleVisibility);

    passRow->addWidget(m_passEdit);
    passRow->addWidget(m_btnToggle);
    layout->addLayout(passRow);

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
    btnCancel->setFixedHeight(30);
    btnCancel->setFlat(true);
    btnCancel->setStyleSheet("color: gray;");
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    layout->addWidget(btnCancel, 0, Qt::AlignCenter);

    m_passEdit->setFocus();
}

void VaultUnlockDialog::onUnlock()
{
    const QString password = m_passEdit->text();

    auto key = QSharedPointer<CompositeKey>::create();
    if (!password.isEmpty()) {
        auto passKey = QSharedPointer<PasswordKey>::create(password);
        key->addKey(passKey);
    }

    auto db = QSharedPointer<Database>::create(m_vaultPath);
    QString error;
    if (!db->open(key, &error)) {
        m_errorLabel->setText(tr("Senha incorreta ou cofre inválido."));
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

bool VaultUnlockDialog::injectDatabase(QSharedPointer<Database> db)
{
    if (!m_mainWindow) return false;

    auto* tabWidget = m_mainWindow->findChild<DatabaseTabWidget*>();
    if (!tabWidget) return false;

    auto* dbWidget = new DatabaseWidget(db, tabWidget);
    tabWidget->addDatabaseTab(dbWidget);
    return true;
}
