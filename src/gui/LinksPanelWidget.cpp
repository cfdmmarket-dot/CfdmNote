#include "LinksPanelWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QScrollArea>
#include <QApplication>
#include <QStorageInfo>
#include <QMenu>

#include "core/Entry.h"
#include "core/Database.h"
#include "core/EntryAttributes.h"

const QString LinksPanelWidget::KEY_LOCAL     = "CfdmNote::LocalLinks";
const QString LinksPanelWidget::KEY_REMOVABLE = "CfdmNote::RemovableLinks";

// ─── Construtor ───────────────────────────────────────────────────────────────

LinksPanelWidget::LinksPanelWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    // Título
    auto* titleLabel = new QLabel(tr("<b>Links da Entrada</b>"));
    titleLabel->setStyleSheet("font-size: 12px; color: palette(text);");
    mainLayout->addWidget(titleLabel);

    // Seção Local Disk
    mainLayout->addWidget(
        buildSection(tr("Local Disk"), "🖴",
                     m_localList, m_btnAddLocal, m_btnRemLocal, m_btnOpenLocal)
    );
    {
        auto* menu = new QMenu(m_btnAddLocal);
        menu->addAction(QIcon::fromTheme("folder"), tr("Pasta..."),
                        this, [this]() { addPathToList(m_localList, KEY_LOCAL); });
        menu->addAction(QIcon::fromTheme("text-x-generic"), tr("Arquivo(s)..."),
                        this, [this]() { addFileToList(m_localList, KEY_LOCAL); });
        m_btnAddLocal->setMenu(menu);
    }
    connect(m_btnRemLocal,  &QPushButton::clicked, this, &LinksPanelWidget::onRemoveLocal);
    connect(m_btnOpenLocal, &QPushButton::clicked, this, &LinksPanelWidget::onOpenLocal);
    connect(m_localList, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem* item) { openPath(item->data(Qt::UserRole).toString()); });

    // Seção External / Removable Disk
    mainLayout->addWidget(
        buildSection(tr("External / Removable Disk"), "💿",
                     m_removableList, m_btnAddRemovable, m_btnRemRemovable, m_btnOpenRemovable)
    );
    {
        auto* menu = new QMenu(m_btnAddRemovable);
        menu->addAction(QIcon::fromTheme("folder"), tr("Pasta..."),
                        this, [this]() { addPathToList(m_removableList, KEY_REMOVABLE); });
        menu->addAction(QIcon::fromTheme("text-x-generic"), tr("Arquivo(s)..."),
                        this, [this]() { addFileToList(m_removableList, KEY_REMOVABLE); });
        m_btnAddRemovable->setMenu(menu);
    }
    connect(m_btnRemRemovable,  &QPushButton::clicked, this, &LinksPanelWidget::onRemoveRemovable);
    connect(m_btnOpenRemovable, &QPushButton::clicked, this, &LinksPanelWidget::onOpenRemovable);
    connect(m_removableList, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem* item) { openPath(item->data(Qt::UserRole).toString()); });

    mainLayout->addStretch();
}

// ─── Constrói seção ───────────────────────────────────────────────────────────

QWidget* LinksPanelWidget::buildSection(const QString& title,
                                         const QString& icon,
                                         QListWidget*& listOut,
                                         QPushButton*& btnAddOut,
                                         QPushButton*& btnRemOut,
                                         QPushButton*& btnOpenOut)
{
    auto* box = new QGroupBox(QString("%1  %2").arg(icon, title));
    box->setStyleSheet("QGroupBox { font-size: 11px; font-weight: bold; "
                       "border: 1px solid palette(mid); border-radius: 4px; "
                       "margin-top: 6px; padding-top: 4px; }"
                       "QGroupBox::title { subcontrol-origin: margin; left: 8px; }");

    auto* lay = new QVBoxLayout(box);
    lay->setSpacing(4);
    lay->setContentsMargins(6, 10, 6, 6);

    listOut = new QListWidget();
    listOut->setFixedHeight(110);
    listOut->setAlternatingRowColors(true);
    listOut->setStyleSheet("QListWidget { font-size: 11px; } "
                            "QListWidget::item { padding: 2px 4px; }");
    lay->addWidget(listOut);

    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(4);

    btnAddOut = new QPushButton(QIcon::fromTheme("list-add"), tr("Adicionar"));
    btnAddOut->setFixedHeight(26);
    btnAddOut->setStyleSheet("font-size: 11px;");

    btnRemOut = new QPushButton(QIcon::fromTheme("list-remove"), tr("Remover"));
    btnRemOut->setFixedHeight(26);
    btnRemOut->setStyleSheet("font-size: 11px;");
    btnRemOut->setEnabled(false);

    btnOpenOut = new QPushButton(QIcon::fromTheme("folder-open"), tr("Abrir"));
    btnOpenOut->setFixedHeight(26);
    btnOpenOut->setStyleSheet("font-size: 11px;");
    btnOpenOut->setEnabled(false);

    btnRow->addWidget(btnAddOut);
    btnRow->addWidget(btnRemOut);
    btnRow->addWidget(btnOpenOut);
    btnRow->addStretch();
    lay->addLayout(btnRow);

    // Habilita/desabilita botões conforme seleção
    connect(listOut, &QListWidget::currentRowChanged, [btnRemOut, btnOpenOut](int row) {
        btnRemOut->setEnabled(row >= 0);
        btnOpenOut->setEnabled(row >= 0);
    });

    return box;
}

// ─── Carga/limpeza ───────────────────────────────────────────────────────────

void LinksPanelWidget::clearLinks()
{
    m_entry = nullptr;
    m_db.reset();
    m_localList->clear();
    m_removableList->clear();
    m_btnAddLocal->setEnabled(false);
    m_btnAddRemovable->setEnabled(false);
}

void LinksPanelWidget::loadEntry(Entry* entry, const QSharedPointer<Database>& db)
{
    clearLinks();
    m_entry = entry;
    m_db    = db;

    m_btnAddLocal->setEnabled(entry != nullptr);
    m_btnAddRemovable->setEnabled(entry != nullptr);
    if (!entry) return;

    auto loadList = [&](QListWidget* list, const QString& key) {
        const QString raw = entry->attributes()->value(key);
        if (raw.isEmpty()) return;
        for (const QString& path : raw.split('\n', Qt::SkipEmptyParts)) {
            const QString trimmed = path.trimmed();
            if (trimmed.isEmpty()) continue;
            auto* item = new QListWidgetItem(trimmed);
            item->setData(Qt::UserRole, trimmed);
            list->addItem(item);
        }
        refreshItemIcons(list);
    };

    loadList(m_localList,     KEY_LOCAL);
    loadList(m_removableList, KEY_REMOVABLE);
}

// ─── Ícones de status (pasta existe / não montada) ────────────────────────────

// ─── Formata exibição: "Label (device)  →  /path" ───────────────────────────

static QString formatDriveInfo(const QString& path)
{
    const QStorageInfo storage(path);
    const QString device = QString::fromLocal8Bit(storage.device());
    const QString label  = storage.name().trimmed();
    const QString mount  = storage.rootPath();

    // Caminho relativo ao ponto de montagem (para mostrar só o subdiretório)
    QString subPath = path;
    if (!mount.isEmpty() && subPath.startsWith(mount))
        subPath = subPath.mid(mount.length());
    if (subPath.isEmpty()) subPath = "/";
    if (!subPath.startsWith("/")) subPath.prepend("/");

    // Linha 1: unidade
    QString driveStr;
    if (!label.isEmpty())
        driveStr = QString("%1  [%2]").arg(label, device);
    else if (!device.isEmpty())
        driveStr = device;
    else
        driveStr = mount.isEmpty() ? path : mount;

    // Linha 2: subdiretório dentro da unidade
    return QString("%1\n  └─ %2").arg(driveStr, subPath);
}

void LinksPanelWidget::refreshItemIcons(QListWidget* list)
{
    const QIcon iconFolder  = QIcon::fromTheme("folder");
    const QIcon iconFile    = QIcon::fromTheme("text-x-generic");
    const QIcon iconMissing = QIcon::fromTheme("dialog-warning");

    for (int i = 0; i < list->count(); ++i) {
        auto* item = list->item(i);
        const QString path = item->data(Qt::UserRole).toString();
        const QFileInfo fi(path);
        const bool exists = fi.exists();
        const bool isFile = exists && fi.isFile();

        if (exists) {
            if (isFile) {
                item->setText(fi.fileName() + "\n  └─ " + fi.dir().path());
                item->setIcon(iconFile);
            } else {
                item->setText(formatDriveInfo(path));
                item->setIcon(iconFolder);
            }
            item->setToolTip(tr("Clique duplo para abrir: %1").arg(path));
            item->setForeground(QApplication::palette().text());
        } else {
            item->setText(formatDriveInfo(path));
            item->setIcon(iconMissing);
            item->setToolTip(tr("⚠ Não encontrado (mídia desmontada?):\n%1").arg(path));
            item->setForeground(QBrush(QColor("#e05050")));
        }
    }
}

// ─── Salvamento ───────────────────────────────────────────────────────────────

void LinksPanelWidget::saveLinks()
{
    if (!m_entry || !m_db) return;

    auto listToStr = [](QListWidget* list) -> QString {
        QStringList paths;
        for (int i = 0; i < list->count(); ++i)
            paths << list->item(i)->data(Qt::UserRole).toString();
        return paths.join('\n');
    };

    m_entry->attributes()->set(KEY_LOCAL,     listToStr(m_localList),     true);
    m_entry->attributes()->set(KEY_REMOVABLE, listToStr(m_removableList), true);

    QString err;
    m_db->save(Database::Atomic, {}, &err);
}

// ─── Abrir pasta ──────────────────────────────────────────────────────────────

void LinksPanelWidget::openPath(const QString& path)
{
    if (!QFileInfo::exists(path)) {
        QMessageBox::warning(this, tr("Caminho não encontrado"),
            tr("O caminho não existe ou a mídia não está montada:\n%1").arg(path));
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

// ─── Adicionar (genérico) ─────────────────────────────────────────────────────

void LinksPanelWidget::addPathToList(QListWidget* list, const QString& /*typeKey*/)
{
    if (!m_entry) {
        QMessageBox::information(this, tr("Links"), tr("Selecione uma entrada primeiro."));
        return;
    }

    const QString path = QFileDialog::getExistingDirectory(
        this,
        tr("Selecionar pasta"),
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    if (path.isEmpty()) return;

    // Evita duplicatas
    for (int i = 0; i < list->count(); ++i)
        if (list->item(i)->data(Qt::UserRole).toString() == path) return;

    auto* item = new QListWidgetItem(path);
    item->setData(Qt::UserRole, path);
    list->addItem(item);
    refreshItemIcons(list);
    saveLinks();
}

// ─── Adicionar arquivo(s) ─────────────────────────────────────────────────────

void LinksPanelWidget::addFileToList(QListWidget* list, const QString& /*typeKey*/)
{
    if (!m_entry) {
        QMessageBox::information(this, tr("Links"), tr("Selecione uma entrada primeiro."));
        return;
    }

    const QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Selecionar arquivo(s)"),
        QDir::homePath(),
        tr("Todos os arquivos (*)")
    );
    if (files.isEmpty()) return;

    for (const QString& path : files) {
        if (path.isEmpty()) continue;
        bool dup = false;
        for (int i = 0; i < list->count(); ++i)
            if (list->item(i)->data(Qt::UserRole).toString() == path) { dup = true; break; }
        if (dup) continue;

        auto* item = new QListWidgetItem(path);
        item->setData(Qt::UserRole, path);
        list->addItem(item);
    }
    refreshItemIcons(list);
    saveLinks();
}

// ─── Slots Local ─────────────────────────────────────────────────────────────

void LinksPanelWidget::onAddLocal()     { addPathToList(m_localList, KEY_LOCAL); }

void LinksPanelWidget::onRemoveLocal()
{
    auto* item = m_localList->currentItem();
    if (!item) return;
    delete m_localList->takeItem(m_localList->row(item));
    saveLinks();
}

void LinksPanelWidget::onOpenLocal()
{
    auto* item = m_localList->currentItem();
    if (item) openPath(item->data(Qt::UserRole).toString());
}

// ─── Slots Removable ─────────────────────────────────────────────────────────

void LinksPanelWidget::onAddRemovable()  { addPathToList(m_removableList, KEY_REMOVABLE); }

void LinksPanelWidget::onRemoveRemovable()
{
    auto* item = m_removableList->currentItem();
    if (!item) return;
    delete m_removableList->takeItem(m_removableList->row(item));
    saveLinks();
}

void LinksPanelWidget::onOpenRemovable()
{
    auto* item = m_removableList->currentItem();
    if (item) openPath(item->data(Qt::UserRole).toString());
}
