#include "KdbxImportDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QApplication>
#include <QProgressDialog>

#include "core/Database.h"
#include "core/Group.h"
#include "core/Entry.h"
#include "keys/CompositeKey.h"
#include "keys/PasswordKey.h"
#include "keys/FileKey.h"
#include "gui/MainWindow.h"
#include "gui/DatabaseWidget.h"
#include "gui/DatabaseTabWidget.h"

// ─── API estática ─────────────────────────────────────────────────────────────

void KdbxImportDialog::runImport(QSharedPointer<Database> destDb,
                                  MainWindow* mainWindow,
                                  QWidget* parent)
{
    if (!destDb && mainWindow) {
        auto* tab = mainWindow->findChild<DatabaseTabWidget*>();
        if (tab && tab->currentDatabaseWidget())
            destDb = tab->currentDatabaseWidget()->database();
    }

    if (!destDb) {
        QMessageBox::warning(parent,
            tr("Importar .kdbx"),
            tr("Nenhum banco de dados aberto no CfdmNote.\n"
               "Abra ou crie um banco antes de importar."));
        return;
    }

    KdbxImportDialog dlg(destDb, mainWindow, parent);
    dlg.exec();
}

// ─── Construtor ───────────────────────────────────────────────────────────────

KdbxImportDialog::KdbxImportDialog(QSharedPointer<Database> destDb,
                                    MainWindow* mainWindow,
                                    QWidget* parent)
    : QDialog(parent)
    , m_destDb(destDb)
    , m_mainWindow(mainWindow)
{
    setWindowTitle(tr("Importar .kdbx Externo — CfdmNote"));
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setFixedWidth(480);
    setModal(true);
    setupUi();
}

// ─── UI ───────────────────────────────────────────────────────────────────────

void KdbxImportDialog::setupUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    layout->setContentsMargins(28, 24, 28, 24);

    // Título
    auto* title = new QLabel(tr("<b>📂 Importar banco .kdbx externo</b>"));
    title->setStyleSheet("font-size: 14px;");
    layout->addWidget(title);

    auto* desc = new QLabel(tr(
        "Selecione um arquivo <b>.kdbx</b> antigo.<br>"
        "Todos os grupos e entradas serão copiados para o banco atual,<br>"
        "preservando a hierarquia de grupos."));
    desc->setStyleSheet("color: #555; font-size: 11px;");
    desc->setWordWrap(true);
    layout->addWidget(desc);

    layout->addSpacing(4);

    // ── Arquivo .kdbx ────────────────────────────────────────────────────────
    auto* kdbxLabel = new QLabel(tr("Arquivo .kdbx:"));
    kdbxLabel->setStyleSheet("font-weight: bold;");
    layout->addWidget(kdbxLabel);

    auto* kdbxRow = new QHBoxLayout();
    m_kdbxEdit = new QLineEdit();
    m_kdbxEdit->setPlaceholderText(tr("Caminho do arquivo .kdbx..."));
    m_kdbxEdit->setFixedHeight(32);
    kdbxRow->addWidget(m_kdbxEdit);

    auto* btnBrowse = new QPushButton(tr("..."));
    btnBrowse->setFixedSize(36, 32);
    btnBrowse->setToolTip(tr("Selecionar arquivo .kdbx"));
    kdbxRow->addWidget(btnBrowse);
    layout->addLayout(kdbxRow);

    // ── Senha ────────────────────────────────────────────────────────────────
    auto* passLabel = new QLabel(tr("Senha (deixe vazio se o banco não tiver senha):"));
    passLabel->setStyleSheet("font-weight: bold;");
    layout->addWidget(passLabel);

    auto* passRow = new QHBoxLayout();
    m_passEdit = new QLineEdit();
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passEdit->setPlaceholderText(tr("Senha do banco (opcional)"));
    m_passEdit->setFixedHeight(32);
    passRow->addWidget(m_passEdit);

    m_btnToggle = new QPushButton(tr("👁"));
    m_btnToggle->setFixedSize(36, 32);
    m_btnToggle->setCheckable(true);
    m_btnToggle->setToolTip(tr("Mostrar/ocultar senha"));
    passRow->addWidget(m_btnToggle);
    layout->addLayout(passRow);

    // ── Arquivo chave (opcional) ─────────────────────────────────────────────
    auto* keyLabel = new QLabel(tr("Arquivo chave (opcional):"));
    keyLabel->setStyleSheet("font-weight: bold;");
    layout->addWidget(keyLabel);

    auto* keyRow = new QHBoxLayout();
    m_keyFileEdit = new QLineEdit();
    m_keyFileEdit->setPlaceholderText(tr("Arquivo .key / .keyx (se o banco usar)"));
    m_keyFileEdit->setFixedHeight(32);
    m_keyFileEdit->setReadOnly(true);
    keyRow->addWidget(m_keyFileEdit);

    auto* btnKey = new QPushButton(tr("..."));
    btnKey->setFixedSize(36, 32);
    btnKey->setToolTip(tr("Selecionar arquivo chave"));
    keyRow->addWidget(btnKey);

    m_btnKeyClear = new QPushButton(tr("✕"));
    m_btnKeyClear->setFixedSize(28, 32);
    m_btnKeyClear->setEnabled(false);
    m_btnKeyClear->setToolTip(tr("Remover arquivo chave"));
    keyRow->addWidget(m_btnKeyClear);
    layout->addLayout(keyRow);

    // ── Destino (info) ───────────────────────────────────────────────────────
    const QString destPath = QFileInfo(m_destDb->filePath()).fileName();
    auto* destLabel = new QLabel(tr("Destino: <b>%1</b>").arg(
        destPath.isEmpty() ? tr("banco atual") : destPath));
    destLabel->setStyleSheet("font-size: 11px; color: #2c5282; "
                             "background: #ebf8ff; padding: 6px 10px; "
                             "border-radius: 4px;");
    layout->addWidget(destLabel);

    // ── Erro / Status ────────────────────────────────────────────────────────
    m_errorLabel = new QLabel();
    m_errorLabel->setStyleSheet("color: #c53030; font-size: 11px;");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();
    layout->addWidget(m_errorLabel);

    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("color: #276749; font-size: 11px;");
    m_statusLabel->setWordWrap(true);
    m_statusLabel->hide();
    layout->addWidget(m_statusLabel);

    // ── Botões ───────────────────────────────────────────────────────────────
    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();

    auto* btnCancel = new QPushButton(tr("Cancelar"));
    btnCancel->setFixedHeight(34);
    btnRow->addWidget(btnCancel);

    m_btnImport = new QPushButton(tr("📥 Importar"));
    m_btnImport->setFixedHeight(34);
    m_btnImport->setDefault(true);
    m_btnImport->setStyleSheet(
        "QPushButton { background: #2c5282; color: white; font-weight: bold; "
        "border-radius: 4px; padding: 0 18px; }"
        "QPushButton:hover { background: #2a4365; }"
        "QPushButton:disabled { background: #a0aec0; }");
    btnRow->addWidget(m_btnImport);
    layout->addLayout(btnRow);

    // ── Conexões ─────────────────────────────────────────────────────────────
    connect(btnBrowse,  &QPushButton::clicked, this, &KdbxImportDialog::onBrowseKdbx);
    connect(btnKey,     &QPushButton::clicked, this, &KdbxImportDialog::onBrowseKeyFile);
    connect(m_btnKeyClear, &QPushButton::clicked, this, [this]() {
        m_keyFileEdit->clear();
        m_btnKeyClear->setEnabled(false);
    });
    connect(m_btnToggle, &QPushButton::toggled, this, &KdbxImportDialog::onTogglePassword);
    connect(m_btnImport, &QPushButton::clicked, this, &KdbxImportDialog::onImport);
    connect(btnCancel,   &QPushButton::clicked, this, &QDialog::reject);
}

// ─── Slots ────────────────────────────────────────────────────────────────────

void KdbxImportDialog::onBrowseKdbx()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        tr("Selecionar banco .kdbx"),
        QDir::homePath(),
        tr("Bancos KeePass (*.kdbx);;Todos os arquivos (*)")
    );
    if (!path.isEmpty())
        m_kdbxEdit->setText(path);
}

void KdbxImportDialog::onBrowseKeyFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        tr("Selecionar arquivo chave"),
        QDir::homePath(),
        tr("Arquivos chave (*.key *.keyx);;Todos os arquivos (*)")
    );
    if (!path.isEmpty()) {
        m_keyFileEdit->setText(path);
        m_btnKeyClear->setEnabled(true);
    }
}

void KdbxImportDialog::onTogglePassword()
{
    m_passEdit->setEchoMode(
        m_btnToggle->isChecked() ? QLineEdit::Normal : QLineEdit::Password);
}

void KdbxImportDialog::onImport()
{
    m_errorLabel->hide();
    m_statusLabel->hide();

    const QString kdbxPath = m_kdbxEdit->text().trimmed();
    if (kdbxPath.isEmpty()) {
        m_errorLabel->setText(tr("Selecione o arquivo .kdbx a importar."));
        m_errorLabel->show();
        return;
    }
    if (!QFileInfo::exists(kdbxPath)) {
        m_errorLabel->setText(tr("Arquivo não encontrado: %1").arg(kdbxPath));
        m_errorLabel->show();
        return;
    }

    m_btnImport->setEnabled(false);
    QApplication::processEvents();

    if (doImport()) {
        accept();
    } else {
        m_btnImport->setEnabled(true);
    }
}

// ─── Import ──────────────────────────────────────────────────────────────────

bool KdbxImportDialog::doImport()
{
    const QString kdbxPath   = m_kdbxEdit->text().trimmed();
    const QString password   = m_passEdit->text();
    const QString keyFilePath = m_keyFileEdit->text().trimmed();

    // Monta CompositeKey
    auto key = QSharedPointer<CompositeKey>::create();

    if (!password.isEmpty()) {
        key->addKey(QSharedPointer<PasswordKey>::create(password));
    } else {
        // Banco sem senha: adiciona PasswordKey vazia para isInitialized() = true
        key->addKey(QSharedPointer<PasswordKey>::create(QString("")));
    }

    if (!keyFilePath.isEmpty()) {
        auto fileKey = QSharedPointer<FileKey>::create();
        QString keyErr;
        if (!fileKey->load(keyFilePath, &keyErr)) {
            m_errorLabel->setText(tr("Erro no arquivo chave: %1").arg(keyErr));
            m_errorLabel->show();
            return false;
        }
        key->addKey(fileKey);
    }

    // Abre banco de origem
    auto srcDb = QSharedPointer<Database>::create(kdbxPath);
    QString openError;
    if (!srcDb->open(key, &openError)) {
        m_errorLabel->setText(
            tr("Não foi possível abrir o banco.\n"
               "Verifique a senha e o arquivo chave.\n\n"
               "Detalhe: %1").arg(openError));
        m_errorLabel->show();
        return false;
    }

    Group* srcRoot = srcDb->rootGroup();
    Group* dstRoot = m_destDb->rootGroup();

    if (!srcRoot || !dstRoot) {
        m_errorLabel->setText(tr("Erro interno: grupos raiz inválidos."));
        m_errorLabel->show();
        return false;
    }

    // Mostra progresso
    const QString srcName = QFileInfo(kdbxPath).fileName();
    auto* progress = new QProgressDialog(
        tr("Importando de %1...").arg(srcName),
        QString(), 0, 0, this);
    progress->setWindowTitle(tr("Importando .kdbx — CfdmNote"));
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setValue(0);
    QApplication::processEvents();

    // Importa grupos e entradas recursivamente
    int totalCount = 0;
    importGroupRecursive(srcRoot, dstRoot, totalCount);

    // Salva destino
    progress->setLabelText(tr("Salvando banco de dados..."));
    QApplication::processEvents();

    QString saveError;
    const bool saved = m_destDb->save(Database::Atomic, QString(), &saveError);
    progress->hide();
    progress->deleteLater();

    // Atualiza visual da janela
    if (m_mainWindow) {
        auto* tab = m_mainWindow->findChild<DatabaseTabWidget*>();
        if (tab && tab->currentDatabaseWidget())
            tab->currentDatabaseWidget()->update();
    }

    if (!saved) {
        QMessageBox::warning(this,
            tr("Importação concluída com aviso"),
            tr("<b>%1</b> grupos e entradas importados de:<br>%2<br><br>"
               "Atenção: não foi possível salvar automaticamente.<br>"
               "Erro: %3<br>Use Arquivo → Salvar para preservar.")
            .arg(totalCount).arg(srcName).arg(saveError));
    } else {
        QMessageBox::information(this,
            tr("Importação concluída"),
            tr("Importação concluída com sucesso!<br><br>"
               "<b>%1</b> itens (grupos + entradas) importados de:<br>%2")
            .arg(totalCount).arg(srcName));
    }

    return true;
}

// ─── Recursão de grupos ───────────────────────────────────────────────────────

void KdbxImportDialog::importGroupRecursive(Group* src, Group* dest, int& count)
{
    // Importa entradas do grupo atual
    for (Entry* e : src->entries()) {
        if (!e) continue;

        auto* newEntry = e->clone(Entry::CloneNewUuid);
        newEntry->setGroup(dest);
        ++count;
    }

    // Importa subgrupos recursivamente
    for (Group* child : src->children()) {
        if (!child) continue;

        // Não importa a Lixeira do banco de origem
        const Group* recycleBin = src->database()->metadata()->recycleBin();
        if (recycleBin && child->uuid() == recycleBin->uuid())
            continue;

        // Cria grupo correspondente no destino
        auto* newGroup = new Group();
        newGroup->setUuid(QUuid::createUuid());
        newGroup->setName(child->name());
        newGroup->setNotes(child->notes());
        newGroup->setIcon(child->iconNumber());
        newGroup->setParent(dest);
        ++count;

        importGroupRecursive(child, newGroup, count);
    }
}
