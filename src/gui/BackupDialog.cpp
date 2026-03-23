#include "BackupDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QStorageInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QMessageBox>
#include <QApplication>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFont>
#include <QDesktopServices>
#include <QUrl>

#include "core/Database.h"
#include "gui/MainWindow.h"
#include "gui/DatabaseWidget.h"
#include "gui/DatabaseTabWidget.h"
#include "gui/AppLockDialog.h"
#include "db/MainDatabaseManager.h"

// ─── Construtor ───────────────────────────────────────────────────────────────

BackupDialog::BackupDialog(QSharedPointer<Database> db,
                            const QString& dbPath,
                            QWidget* parent)
    : QDialog(parent)
    , m_db(db)
    , m_dbPath(dbPath)
{
    setWindowTitle(tr("Backup Externo — CfdmNote"));
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setMinimumWidth(520);
    setModal(true);
    setupUi();
}

// ─── UI ──────────────────────────────────────────────────────────────────────

void BackupDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(20, 16, 20, 16);

    // ── Banco de origem ───────────────────────────────────────────────────
    auto* srcGroup = new QGroupBox(tr("Banco de dados a salvar"));
    srcGroup->setStyleSheet("QGroupBox { font-weight: bold; font-size: 11px; }");
    auto* srcLayout = new QHBoxLayout(srcGroup);
    m_sourceLabel = new QLabel(
        m_dbPath.isEmpty() ? tr("(banco sem arquivo — não é possível fazer backup)")
                           : m_dbPath);
    m_sourceLabel->setStyleSheet("font-size: 11px; color: palette(text);");
    m_sourceLabel->setWordWrap(true);
    srcLayout->addWidget(QIcon::fromTheme("database").pixmap(16,16).isNull()
        ? new QLabel("🗄")
        : [&]{ auto* l = new QLabel(); l->setPixmap(QIcon::fromTheme("database").pixmap(16,16)); return l; }()
    );
    srcLayout->addWidget(m_sourceLabel, 1);
    mainLayout->addWidget(srcGroup);

    // ── Destino ───────────────────────────────────────────────────────────
    auto* destGroup = new QGroupBox(tr("Destino do backup"));
    destGroup->setStyleSheet("QGroupBox { font-weight: bold; font-size: 11px; }");
    auto* destLayout = new QGridLayout(destGroup);
    destLayout->setSpacing(6);

    // ComboBox de mídias removíveis
    destLayout->addWidget(new QLabel(tr("Mídia detectada:")), 0, 0);
    m_driveCombo = new QComboBox();
    m_driveCombo->setStyleSheet("font-size: 11px;");
    destLayout->addWidget(m_driveCombo, 0, 1, 1, 2);
    connect(m_driveCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BackupDialog::onDriveSelected);

    // Path manual
    destLayout->addWidget(new QLabel(tr("Pasta de destino:")), 1, 0);
    m_destEdit = new QLineEdit();
    m_destEdit->setStyleSheet("font-size: 11px;");
    m_destEdit->setPlaceholderText(tr("Selecione uma mídia acima ou digite o caminho"));
    connect(m_destEdit, &QLineEdit::textChanged, this, [this]() {
        m_fileNameLabel->setText(tr("Arquivo: ") + destFullPath());
    });
    destLayout->addWidget(m_destEdit, 1, 1);

    m_btnBrowse = new QPushButton(tr("Procurar..."));
    m_btnBrowse->setFixedHeight(28);
    connect(m_btnBrowse, &QPushButton::clicked, this, &BackupDialog::onBrowse);
    destLayout->addWidget(m_btnBrowse, 1, 2);

    // Nome do arquivo gerado
    m_fileNameLabel = new QLabel(tr("Arquivo: ") + destFullPath());
    m_fileNameLabel->setStyleSheet("font-size: 10px; color: gray;");
    destLayout->addWidget(m_fileNameLabel, 2, 0, 1, 3);

    mainLayout->addWidget(destGroup);

    // ── Nota descritiva ───────────────────────────────────────────────────
    auto* noteGroup = new QGroupBox(tr("Nota sobre este backup (opcional)"));
    noteGroup->setStyleSheet("QGroupBox { font-weight: bold; font-size: 11px; }");
    auto* noteLayout = new QHBoxLayout(noteGroup);
    m_noteEdit = new QLineEdit();
    m_noteEdit->setPlaceholderText(tr("Ex: USB Kingston preto, HD externo Seagate 1TB..."));
    m_noteEdit->setStyleSheet("font-size: 11px;");
    noteLayout->addWidget(m_noteEdit);
    mainLayout->addWidget(noteGroup);

    // ── Histórico ─────────────────────────────────────────────────────────
    auto* histGroup = new QGroupBox(tr("Histórico de backups (últimos %1)").arg(MAX_HISTORY));
    histGroup->setStyleSheet("QGroupBox { font-weight: bold; font-size: 11px; }");
    auto* histLayout = new QVBoxLayout(histGroup);

    m_historyList = new QListWidget();
    m_historyList->setFixedHeight(120);
    m_historyList->setAlternatingRowColors(true);
    m_historyList->setStyleSheet("font-size: 10px;");
    m_historyList->setToolTip(tr("Duplo clique para copiar o caminho"));
    connect(m_historyList, &QListWidget::itemDoubleClicked,
            this, &BackupDialog::onHistoryItemDoubleClicked);
    histLayout->addWidget(m_historyList);

    auto* histBtnRow = new QHBoxLayout();
    m_btnRemHistory = new QPushButton(QIcon::fromTheme("list-remove"), tr("Remover entrada"));
    m_btnRemHistory->setFixedHeight(26);
    m_btnRemHistory->setStyleSheet("font-size: 11px;");
    m_btnRemHistory->setEnabled(false);
    connect(m_btnRemHistory, &QPushButton::clicked, this, &BackupDialog::onRemoveHistory);
    connect(m_historyList, &QListWidget::currentRowChanged, [this](int row) {
        m_btnRemHistory->setEnabled(row >= 0);
    });
    histBtnRow->addWidget(m_btnRemHistory);
    histBtnRow->addStretch();
    histLayout->addLayout(histBtnRow);

    mainLayout->addWidget(histGroup);

    // ── Status + progresso ────────────────────────────────────────────────
    m_statusLabel = new QLabel();
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("font-size: 11px; color: gray;");
    mainLayout->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 0);  // indeterminado
    m_progressBar->setFixedHeight(6);
    m_progressBar->setVisible(false);
    m_progressBar->setStyleSheet("QProgressBar { border: none; border-radius: 3px; background: palette(mid); }"
                                  "QProgressBar::chunk { background: #2a5caa; border-radius: 3px; }");
    mainLayout->addWidget(m_progressBar);

    // ── Botões ────────────────────────────────────────────────────────────
    auto* btnRow = new QHBoxLayout();

    m_btnBackup = new QPushButton(tr("💾  Fazer Backup Agora"));
    m_btnBackup->setFixedHeight(38);
    m_btnBackup->setDefault(true);
    m_btnBackup->setStyleSheet(
        "QPushButton { background: #2a5caa; color: white; border-radius: 5px; "
        "font-weight: bold; font-size: 12px; }"
        "QPushButton:hover { background: #3a6cba; }"
        "QPushButton:pressed { background: #1a4c9a; }"
        "QPushButton:disabled { background: palette(mid); color: palette(dark); }"
    );
    m_btnBackup->setEnabled(!m_dbPath.isEmpty());
    connect(m_btnBackup, &QPushButton::clicked, this, &BackupDialog::onDoBackup);

    auto* btnClose = new QPushButton(tr("Fechar"));
    btnClose->setFixedHeight(38);
    btnClose->setStyleSheet("font-size: 12px;");
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);

    btnRow->addWidget(m_btnBackup, 1);
    btnRow->addWidget(btnClose);
    mainLayout->addLayout(btnRow);

    // Preenche dados iniciais
    refreshDrives();
    refreshHistory();
}

// ─── Mídias removíveis ────────────────────────────────────────────────────────

void BackupDialog::refreshDrives()
{
    m_driveCombo->clear();
    m_driveCombo->addItem(tr("— Selecione uma mídia —"), QString());

    for (const QStorageInfo& vol : QStorageInfo::mountedVolumes()) {
        if (!vol.isValid() || !vol.isReady()) continue;
        if (vol.isRoot()) continue;  // ignora disco do sistema

        const QString mount  = vol.rootPath();
        const QString device = QString::fromLocal8Bit(vol.device());
        const QString label  = vol.name().trimmed();

        // Prioriza mídias em /media/ ou /run/media/ (removíveis típicas no Linux)
        const bool likelyRemovable = mount.startsWith("/media/")
                                  || mount.startsWith("/run/media/")
                                  || mount.startsWith("/mnt/");

        QString display;
        if (!label.isEmpty())
            display = QString("%1  [%2]  →  %3").arg(label, device, mount);
        else
            display = QString("%1  →  %2").arg(device, mount);

        if (likelyRemovable)
            display.prepend("💿  ");
        else
            display.prepend("🖴  ");

        m_driveCombo->addItem(display, mount);
    }

    if (m_driveCombo->count() == 1)
        m_driveCombo->addItem(tr("(nenhuma mídia externa detectada)"), QString());
}

void BackupDialog::onDriveSelected(int index)
{
    const QString mount = m_driveCombo->itemData(index).toString();
    if (!mount.isEmpty()) {
        m_destEdit->setText(mount);
        m_fileNameLabel->setText(tr("Arquivo: ") + destFullPath());
    }
}

// ─── Nome do arquivo / path completo ─────────────────────────────────────────

QString BackupDialog::suggestedFileName() const
{
    const QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm");
    return QString("CfdmNote_backup_%1.kdbx").arg(ts);
}

QString BackupDialog::destFullPath() const
{
    const QString dir = m_destEdit ? m_destEdit->text().trimmed() : QString();
    if (dir.isEmpty()) return suggestedFileName();
    return QDir(dir).absoluteFilePath(suggestedFileName());
}

// ─── Procurar pasta ───────────────────────────────────────────────────────────

void BackupDialog::onBrowse()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Selecionar pasta de destino do backup"),
        m_destEdit->text().isEmpty() ? QDir::homePath() : m_destEdit->text(),
        QFileDialog::ShowDirsOnly
    );
    if (!dir.isEmpty()) {
        m_destEdit->setText(dir);
        m_fileNameLabel->setText(tr("Arquivo: ") + destFullPath());
    }
}

// ─── Fazer backup ─────────────────────────────────────────────────────────────

void BackupDialog::onDoBackup()
{
    const QString destPath = destFullPath();
    const QString destDir  = QFileInfo(destPath).absolutePath();

    if (m_dbPath.isEmpty()) {
        QMessageBox::warning(this, tr("Backup"), tr("O banco de dados não tem um arquivo associado."));
        return;
    }
    if (!QFile::exists(m_dbPath)) {
        QMessageBox::warning(this, tr("Backup"),
            tr("Arquivo de origem não encontrado:\n%1").arg(m_dbPath));
        return;
    }
    if (m_destEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Backup"), tr("Selecione um destino para o backup."));
        m_destEdit->setFocus();
        return;
    }
    if (!QDir(destDir).exists()) {
        QMessageBox::warning(this, tr("Backup"),
            tr("A pasta de destino não existe ou a mídia não está montada:\n%1").arg(destDir));
        return;
    }
    if (QFile::exists(destPath)) {
        const int r = QMessageBox::question(this, tr("Backup"),
            tr("O arquivo já existe:\n%1\n\nSobrescrever?").arg(destPath),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (r != QMessageBox::Yes) return;
        QFile::remove(destPath);
    }

    // ── Progresso visual ──────────────────────────────────────────────────
    m_btnBackup->setEnabled(false);
    m_progressBar->setVisible(true);
    m_statusLabel->setText(tr("Copiando..."));
    QApplication::processEvents();

    // ── Cópia ─────────────────────────────────────────────────────────────
    const bool copied = QFile::copy(m_dbPath, destPath);

    m_progressBar->setVisible(false);

    if (!copied) {
        m_statusLabel->setText(tr("❌ Erro ao copiar o arquivo."));
        m_statusLabel->setStyleSheet("font-size: 11px; color: #e05050;");
        m_btnBackup->setEnabled(true);
        QMessageBox::critical(this, tr("Erro no backup"),
            tr("Não foi possível copiar para:\n%1\n\n"
               "Verifique se a mídia está montada e tem espaço livre.").arg(destPath));
        return;
    }

    // ── Verificação de integridade (tamanho) ──────────────────────────────
    const qint64 srcSize  = QFileInfo(m_dbPath).size();
    const qint64 destSize = QFileInfo(destPath).size();
    if (srcSize != destSize) {
        m_statusLabel->setText(tr("⚠ Backup concluído mas tamanhos divergem. Verifique o arquivo."));
        m_statusLabel->setStyleSheet("font-size: 11px; color: #e08030;");
    } else {
        m_statusLabel->setText(tr("✅ Backup salvo com sucesso!"));
        m_statusLabel->setStyleSheet("font-size: 11px; color: #30a030;");
    }

    // ── Salva no histórico ────────────────────────────────────────────────
    const QString note = m_noteEdit->text().trimmed();
    saveToHistory(destPath, note);
    refreshHistory();

    m_btnBackup->setEnabled(true);

    QMessageBox::information(this, tr("Backup concluído"),
        tr("<b>Backup salvo com sucesso!</b><br><br>"
           "<b>Destino:</b> %1<br>"
           "<b>Tamanho:</b> %2 KB<br>"
           "%3")
        .arg(destPath)
        .arg(destSize / 1024)
        .arg(note.isEmpty() ? QString() : tr("<b>Nota:</b> %1").arg(note))
    );
}

// ─── Histórico ────────────────────────────────────────────────────────────────

static QString historyConfigPath()
{
    return AppLockDialog::configPath();
}

void BackupDialog::saveToHistory(const QString& destPath, const QString& note)
{
    QSettings s(historyConfigPath(), QSettings::IniFormat);

    // Lê entradas existentes
    const int size = s.beginReadArray("backup/history");
    QList<QStringList> entries;
    for (int i = 0; i < size && i < MAX_HISTORY - 1; ++i) {
        s.setArrayIndex(i);
        entries << QStringList{
            s.value("date").toString(),
            s.value("path").toString(),
            s.value("note").toString()
        };
    }
    s.endArray();

    // Adiciona nova entrada no início
    entries.prepend(QStringList{
        QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm"),
        destPath,
        note
    });
    if (entries.size() > MAX_HISTORY) entries.removeLast();

    // Salva
    s.beginWriteArray("backup/history");
    for (int i = 0; i < entries.size(); ++i) {
        s.setArrayIndex(i);
        s.setValue("date", entries[i][0]);
        s.setValue("path", entries[i][1]);
        s.setValue("note", entries[i][2]);
    }
    s.endArray();
    s.sync();
}

void BackupDialog::refreshHistory()
{
    m_historyList->clear();

    QSettings s(historyConfigPath(), QSettings::IniFormat);
    const int size = s.beginReadArray("backup/history");

    for (int i = 0; i < size; ++i) {
        s.setArrayIndex(i);
        const QString date = s.value("date").toString();
        const QString path = s.value("path").toString();
        const QString note = s.value("note").toString();

        const bool exists = QFileInfo::exists(path);
        const QString statusIcon = exists ? "✅" : "⚠";

        QString display = QString("%1  %2  →  %3").arg(statusIcon, date, path);
        if (!note.isEmpty()) display += QString("  [%1]").arg(note);

        auto* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, path);
        item->setData(Qt::UserRole + 1, note);
        item->setForeground(exists ? QApplication::palette().text()
                                   : QBrush(QColor("#e05050")));
        item->setToolTip(exists
            ? tr("Backup encontrado: %1").arg(path)
            : tr("⚠ Arquivo não encontrado (mídia desmontada?):\n%1").arg(path));
        m_historyList->addItem(item);
    }
    s.endArray();

    if (m_historyList->count() == 0) {
        auto* placeholder = new QListWidgetItem(tr("Nenhum backup realizado ainda."));
        placeholder->setForeground(QBrush(Qt::gray));
        placeholder->setFlags(Qt::NoItemFlags);
        m_historyList->addItem(placeholder);
    }
}

void BackupDialog::onHistoryItemDoubleClicked(QListWidgetItem* item)
{
    const QString path = item->data(Qt::UserRole).toString();
    if (path.isEmpty()) return;
    // Abre a pasta que contém o backup
    const QString dir = QFileInfo(path).absolutePath();
    if (QFileInfo::exists(dir)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    } else {
        QMessageBox::information(this, tr("Backup"),
            tr("A pasta não está acessível:\n%1").arg(dir));
    }
}

void BackupDialog::onRemoveHistory()
{
    const int row = m_historyList->currentRow();
    if (row < 0) return;

    const int r = QMessageBox::question(this, tr("Remover do histórico"),
        tr("Remover esta entrada do histórico?\n(O arquivo de backup NÃO será apagado.)"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (r != QMessageBox::Yes) return;

    // Lê todas, remove a selecionada, regrava
    QSettings s(historyConfigPath(), QSettings::IniFormat);
    const int size = s.beginReadArray("backup/history");
    QList<QStringList> entries;
    for (int i = 0; i < size; ++i) {
        s.setArrayIndex(i);
        entries << QStringList{
            s.value("date").toString(),
            s.value("path").toString(),
            s.value("note").toString()
        };
    }
    s.endArray();

    if (row < entries.size()) entries.removeAt(row);

    s.remove("backup/history");
    s.beginWriteArray("backup/history");
    for (int i = 0; i < entries.size(); ++i) {
        s.setArrayIndex(i);
        s.setValue("date", entries[i][0]);
        s.setValue("path", entries[i][1]);
        s.setValue("note", entries[i][2]);
    }
    s.endArray();
    s.sync();

    refreshHistory();
}

// ─── API pública estática ─────────────────────────────────────────────────────

void BackupDialog::runBackup(MainWindow* mainWindow, QWidget* parent)
{
    if (!mainWindow) return;

    // Descobre banco ativo
    QSharedPointer<Database> db;
    QString dbPath;

    auto* tabWidget = mainWindow->findChild<DatabaseTabWidget*>();
    if (tabWidget && tabWidget->currentDatabaseWidget()) {
        db     = tabWidget->currentDatabaseWidget()->database();
        dbPath = db ? db->filePath() : QString();
    }

    // Fallback: banco principal
    if (dbPath.isEmpty())
        dbPath = MainDatabaseManager::defaultDatabasePath();

    BackupDialog dlg(db, dbPath, parent ? parent : mainWindow);
    dlg.exec();
}
