#include "AttachmentsPanelWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidgetItem>
#include <QFileDialog>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QMenu>
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QMimeType>

#include "core/Entry.h"
#include "core/Database.h"
#include "core/EntryAttachments.h"
#include "core/EntryAttributes.h"

// ─── Chave de áudio gravado ────────────────────────────────────────────────────
const QString AttachmentsPanelWidget::KEY_AUDIO = "CfdmNote::AudioAttachments";

// ─── Roles nos items da lista ─────────────────────────────────────────────────
static const int ROLE_PATH   = Qt::UserRole + 1;  // caminho no disco (ou "")
static const int ROLE_SOURCE = Qt::UserRole + 2;  // "binary" | "local" | "audio" | "removable"
static const int ROLE_KEY    = Qt::UserRole + 3;  // chave no binary attachments

// ─── Detecta ícone por extensão ───────────────────────────────────────────────
static QString iconFor(const QString& name)
{
    const QString ext = QFileInfo(name).suffix().toLower();
    if (ext == "wav" || ext == "mp3" || ext == "ogg" || ext == "flac" || ext == "m4a")
        return "🎙";
    if (ext == "txt" || ext == "md")
        return "📝";
    if (ext == "pdf")
        return "📄";
    if (ext == "doc" || ext == "docx" || ext == "odt" || ext == "rtf")
        return "📃";
    if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "webp")
        return "🖼";
    if (ext == "zip" || ext == "tar" || ext == "gz" || ext == "7z" || ext == "rar")
        return "🗜";
    if (ext == "xlsx" || ext == "xls" || ext == "ods" || ext == "csv")
        return "📊";
    if (ext == "pptx" || ext == "ppt" || ext == "odp")
        return "📑";
    return "📎";
}

// ─── Constructor ──────────────────────────────────────────────────────────────
AttachmentsPanelWidget::AttachmentsPanelWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(4);

    // Cabeçalho
    auto* hdr = new QLabel(tr(
        "<small style='color:#718096;'>"
        "Todos os arquivos vinculados a esta entrada.</small>"));
    hdr->setTextFormat(Qt::RichText);
    root->addWidget(hdr);

    // Lista
    m_list = new QListWidget();
    m_list->setAlternatingRowColors(true);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    m_list->setMinimumHeight(80);
    connect(m_list, &QListWidget::itemDoubleClicked,
            this, &AttachmentsPanelWidget::onItemDoubleClicked);
    connect(m_list, &QListWidget::customContextMenuRequested,
            this, &AttachmentsPanelWidget::showContextMenu);
    root->addWidget(m_list, 1);

    // Label "sem anexos"
    m_emptyLbl = new QLabel(tr(
        "<div style='text-align:center; color:#a0aec0; padding:20px;'>"
        "📎 Nenhum anexo nesta entrada.</div>"));
    m_emptyLbl->setTextFormat(Qt::RichText);
    m_emptyLbl->setAlignment(Qt::AlignCenter);
    m_emptyLbl->hide();
    root->addWidget(m_emptyLbl);

    // Botões
    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(4);

    m_btnOpen = new QPushButton(tr("Abrir"));
    m_btnOpen->setEnabled(false);
    connect(m_btnOpen, &QPushButton::clicked, this, &AttachmentsPanelWidget::onOpen);
    btnRow->addWidget(m_btnOpen);

    m_btnSave = new QPushButton(tr("Salvar..."));
    m_btnSave->setEnabled(false);
    m_btnSave->setToolTip(tr("Salvar binário do banco em disco"));
    connect(m_btnSave, &QPushButton::clicked, this, &AttachmentsPanelWidget::onSaveToDesk);
    btnRow->addWidget(m_btnSave);

    m_btnRemove = new QPushButton(tr("Remover"));
    m_btnRemove->setEnabled(false);
    connect(m_btnRemove, &QPushButton::clicked, this, &AttachmentsPanelWidget::onRemove);
    btnRow->addWidget(m_btnRemove);

    btnRow->addStretch();

    m_btnAdd = new QPushButton(tr("+ Adicionar arquivo..."));
    m_btnAdd->setEnabled(false);
    connect(m_btnAdd, &QPushButton::clicked, this, &AttachmentsPanelWidget::onAddFile);
    btnRow->addWidget(m_btnAdd);

    root->addLayout(btnRow);

    // Habilita botões ao selecionar item
    connect(m_list, &QListWidget::currentItemChanged,
            this, [this](QListWidgetItem* cur, QListWidgetItem*) {
        const bool hasItem = (cur != nullptr);
        const QString src  = hasItem ? cur->data(ROLE_SOURCE).toString() : QString();
        m_btnOpen->setEnabled(hasItem && src != "binary");
        m_btnSave->setEnabled(hasItem && src == "binary");
        m_btnRemove->setEnabled(hasItem);
    });
}

// ─── API pública ───────────────────────────────────────────────────────────────
void AttachmentsPanelWidget::setEntry(Entry* entry, const QSharedPointer<Database>& db)
{
    m_entry = entry;
    m_db    = db;
    refresh();
}

void AttachmentsPanelWidget::clear()
{
    m_entry = nullptr;
    m_db.reset();
    m_list->clear();
    m_emptyLbl->hide();
    m_list->show();
    m_btnOpen->setEnabled(false);
    m_btnSave->setEnabled(false);
    m_btnRemove->setEnabled(false);
    m_btnAdd->setEnabled(false);
}

// ─── Refresh ──────────────────────────────────────────────────────────────────
void AttachmentsPanelWidget::refresh()
{
    m_list->clear();
    m_btnAdd->setEnabled(m_entry != nullptr);

    if (!m_entry) {
        m_emptyLbl->hide();
        return;
    }

    // 1. KeePassXC binary attachments
    if (m_entry->attachments()) {
        for (const QString& key : m_entry->attachments()->keys()) {
            const int bytes = m_entry->attachments()->value(key).size();
            const QString label = QString("%1  <small style='color:#718096;'>(%2 KB — banco)</small>")
                .arg(key).arg(bytes / 1024 + 1);
            addRow(iconFor(key), label, tr("Banco"), QString(), "binary");
            m_list->item(m_list->count() - 1)->setData(ROLE_KEY, key);
        }
    }

    // 2. Arquivos locais (docs importados, links locais)
    const QString localRaw = m_entry->attributes()->value("CfdmNote::LocalLinks");
    if (!localRaw.isEmpty()) {
        for (const QString& path : localRaw.split('\n', Qt::SkipEmptyParts)) {
            const QFileInfo fi(path);
            const bool exists = fi.exists();
            const QString label = QString("%1  <small style='color:%2;'>%3</small>")
                .arg(fi.fileName())
                .arg(exists ? "#718096" : "#e53e3e")
                .arg(exists ? path : path + tr(" ⚠ não encontrado"));
            addRow(iconFor(path), label, tr("Local"), path, "local");
        }
    }

    // 3. Áudio gravado
    const QString audioRaw = m_entry->attributes()->value(KEY_AUDIO);
    if (!audioRaw.isEmpty()) {
        for (const QString& path : audioRaw.split('\n', Qt::SkipEmptyParts)) {
            const QFileInfo fi(path);
            const bool exists = fi.exists();
            const QString label = QString("%1  <small style='color:%2;'>%3</small>")
                .arg(fi.fileName())
                .arg(exists ? "#718096" : "#e53e3e")
                .arg(exists ? fi.lastModified().toString("dd/MM/yyyy HH:mm")
                             : tr("⚠ arquivo não encontrado"));
            addRow("🎙", label, tr("Áudio"), path, "audio");
        }
    }

    // 4. Mídia removível
    const QString removableRaw = m_entry->attributes()->value("CfdmNote::RemovableLinks");
    if (!removableRaw.isEmpty()) {
        for (const QString& path : removableRaw.split('\n', Qt::SkipEmptyParts)) {
            const QFileInfo fi(path);
            const QString label = QString("%1  <small style='color:#718096;'>%2</small>")
                .arg(fi.fileName()).arg(path);
            addRow(iconFor(path), label, tr("Removível"), path, "removable");
        }
    }

    // Mostrar label vazio se não há itens
    const bool hasItems = (m_list->count() > 0);
    m_list->setVisible(hasItems);
    m_emptyLbl->setVisible(!hasItems);
}

// ─── Adiciona linha na lista ───────────────────────────────────────────────────
void AttachmentsPanelWidget::addRow(const QString& icon, const QString& label,
                                    const QString& type, const QString& path,
                                    const QString& source)
{
    auto* item = new QListWidgetItem();
    QString plainLabel = label;
    plainLabel.remove(QRegExp("<[^>]*>"));
    item->setText(icon + "  " + plainLabel);
    item->setToolTip(path.isEmpty() ? label : path);
    item->setData(ROLE_PATH, path);
    item->setData(ROLE_SOURCE, source);
    item->setData(Qt::StatusTipRole, type);

    // Badge de tipo como foreground hint
    if (type == tr("Áudio"))
        item->setForeground(QColor("#6d28d9"));
    else if (type == tr("Banco"))
        item->setForeground(QColor("#1d4ed8"));

    m_list->addItem(item);
}

// ─── Slots ────────────────────────────────────────────────────────────────────
void AttachmentsPanelWidget::onItemDoubleClicked(QListWidgetItem* item)
{
    openItem(item);
}

void AttachmentsPanelWidget::onOpen()
{
    openItem(m_list->currentItem());
}

void AttachmentsPanelWidget::openItem(QListWidgetItem* item)
{
    if (!item) return;
    const QString src  = item->data(ROLE_SOURCE).toString();
    const QString path = item->data(ROLE_PATH).toString();

    if (src == "binary") {
        // Salva temp e abre
        const QString key     = item->data(ROLE_KEY).toString();
        const QByteArray data = m_entry->attachments()->value(key);
        const QString tmp     = QDir::tempPath() + "/" + key;
        QFile f(tmp);
        if (f.open(QIODevice::WriteOnly)) { f.write(data); f.close(); }
        QDesktopServices::openUrl(QUrl::fromLocalFile(tmp));
    } else if (!path.isEmpty()) {
        const QString ext = QFileInfo(path).suffix().toLower();
        if (ext == "wav" || ext == "mp3" || ext == "ogg" || ext == "flac") {
            // Reproduz com aplay / paplay
            QProcess::startDetached("aplay", {path});
        } else {
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
    }
}

void AttachmentsPanelWidget::onSaveToDesk()
{
    auto* item = m_list->currentItem();
    if (!item || item->data(ROLE_SOURCE).toString() != "binary") return;
    const QString key = item->data(ROLE_KEY).toString();
    const QByteArray data = m_entry->attachments()->value(key);

    const QString dst = QFileDialog::getSaveFileName(
        this, tr("Salvar anexo"), QDir::homePath() + "/" + key);
    if (dst.isEmpty()) return;
    QFile f(dst);
    if (f.open(QIODevice::WriteOnly)) { f.write(data); f.close(); }
}

void AttachmentsPanelWidget::onRemove()
{
    auto* item = m_list->currentItem();
    if (!item) return;
    removeItem(item);
}

void AttachmentsPanelWidget::removeItem(QListWidgetItem* item)
{
    if (!item || !m_entry) return;
    const QString src  = item->data(ROLE_SOURCE).toString();
    const QString path = item->data(ROLE_PATH).toString();

    if (src == "binary") {
        const QString key = item->data(ROLE_KEY).toString();
        m_entry->attachments()->remove(key);
    } else if (src == "local") {
        QString raw = m_entry->attributes()->value("CfdmNote::LocalLinks");
        QStringList lines = raw.split('\n', Qt::SkipEmptyParts);
        lines.removeAll(path);
        m_entry->attributes()->set("CfdmNote::LocalLinks", lines.join('\n'), false);
    } else if (src == "audio") {
        QString raw = m_entry->attributes()->value(KEY_AUDIO);
        QStringList lines = raw.split('\n', Qt::SkipEmptyParts);
        lines.removeAll(path);
        m_entry->attributes()->set(KEY_AUDIO, lines.join('\n'), false);
    } else if (src == "removable") {
        QString raw = m_entry->attributes()->value("CfdmNote::RemovableLinks");
        QStringList lines = raw.split('\n', Qt::SkipEmptyParts);
        lines.removeAll(path);
        m_entry->attributes()->set("CfdmNote::RemovableLinks", lines.join('\n'), false);
    }

    if (m_db) { QString err; m_db->save(Database::Atomic, {}, &err); }
    refresh();
}

void AttachmentsPanelWidget::onAddFile()
{
    if (!m_entry) return;
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Adicionar arquivo como link"), QDir::homePath());
    if (path.isEmpty()) return;

    // Adiciona a CfdmNote::LocalLinks
    QString raw = m_entry->attributes()->value("CfdmNote::LocalLinks");
    QStringList lines = raw.split('\n', Qt::SkipEmptyParts);
    if (!lines.contains(path)) {
        lines << path;
        m_entry->attributes()->set("CfdmNote::LocalLinks", lines.join('\n'), false);
        if (m_db) { QString err; m_db->save(Database::Atomic, {}, &err); }
    }
    refresh();
}

void AttachmentsPanelWidget::showContextMenu(const QPoint& pos)
{
    auto* item = m_list->itemAt(pos);
    if (!item) return;
    const QString src = item->data(ROLE_SOURCE).toString();

    QMenu menu(this);
    if (src == "binary") {
        menu.addAction(tr("💾 Salvar em disco..."), this, &AttachmentsPanelWidget::onSaveToDesk);
    } else {
        menu.addAction(tr("📂 Abrir"), this, &AttachmentsPanelWidget::onOpen);
        const QString path = item->data(ROLE_PATH).toString();
        if (!path.isEmpty()) {
            menu.addAction(tr("📁 Abrir pasta"), this, [path]() {
                QDesktopServices::openUrl(
                    QUrl::fromLocalFile(QFileInfo(path).dir().absolutePath()));
            });
        }
    }
    menu.addSeparator();
    menu.addAction(tr("🗑 Remover da entrada"), this, [this, item]() { removeItem(item); });
    menu.exec(m_list->viewport()->mapToGlobal(pos));
}
