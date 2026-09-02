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
#include <QSplitter>
#include <QSizePolicy>
#include <QApplication>
#include <QStorageInfo>
#include <QMenu>
#include <QClipboard>
#include <QProcess>
#include <QMimeDatabase>
#include <QMimeType>
#include <QDialog>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QLabel>
#include <QSettings>
#include <QStandardPaths>

#include "core/Entry.h"
#include "core/Database.h"
#include "core/EntryAttributes.h"

const QString LinksPanelWidget::KEY_LOCAL     = "CfdmNote::LocalLinks";
const QString LinksPanelWidget::KEY_REMOVABLE = "CfdmNote::RemovableLinks";

// ─── Construtor ───────────────────────────────────────────────────────────────

LinksPanelWidget::LinksPanelWidget(QWidget* parent)
    : QWidget(parent)
{
    // Layout raiz: sem margens, ocupa tudo
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // QSplitter vertical — divide espaço entre as duas seções
    auto* splitter = new QSplitter(Qt::Vertical, this);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(6);

    // ── Seção Local Disk ───────────────────────────────────────────────────
    auto* localSection = buildSection(tr("Local Disk"), "🖴",
                                      m_localList, m_btnAddLocal,
                                      m_btnRemLocal, m_btnOpenLocal);
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
    setupContextMenu(m_localList);
    splitter->addWidget(localSection);

    // ── Seção External / Removable Disk ───────────────────────────────────
    auto* removableSection = buildSection(tr("External / Removable Disk"), "💿",
                                          m_removableList, m_btnAddRemovable,
                                          m_btnRemRemovable, m_btnOpenRemovable);
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
    setupContextMenu(m_removableList);
    splitter->addWidget(removableSection);

    // Distribui espaço igual entre as duas seções
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    rootLayout->addWidget(splitter);
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
    box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* lay = new QVBoxLayout(box);
    lay->setSpacing(4);
    lay->setContentsMargins(6, 14, 6, 6);

    listOut = new QListWidget();
    listOut->setMinimumHeight(60);
    listOut->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    listOut->setAlternatingRowColors(true);
    listOut->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    listOut->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    listOut->setStyleSheet("QListWidget { font-size: 11px; } "
                            "QListWidget::item { padding: 2px 4px; }");
    lay->addWidget(listOut, 1);

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

static QIcon iconForPath(const QString& path)
{
    const QFileInfo fi(path);
    if (!fi.exists()) return QIcon::fromTheme("dialog-warning");
    if (fi.isDir())  return QIcon::fromTheme("folder");

    // Tenta ícone pelo MIME type (ex.: application/pdf → "application-pdf")
    static QMimeDatabase mimeDb;
    const QMimeType mime = mimeDb.mimeTypeForFile(path, QMimeDatabase::MatchExtension);
    QIcon icon = QIcon::fromTheme(mime.iconName());
    if (!icon.isNull()) return icon;

    // Fallback: ícone genérico do MIME (ex.: "text-x-generic", "image-x-generic")
    icon = QIcon::fromTheme(mime.genericIconName());
    if (!icon.isNull()) return icon;

    return QIcon::fromTheme("text-x-generic");
}

void LinksPanelWidget::refreshItemIcons(QListWidget* list)
{
    const QIcon iconFolder  = QIcon::fromTheme("folder");
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
                item->setIcon(iconForPath(path));
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

// ─── Adiciona link externo (chamado ao importar documento) ───────────────────

void LinksPanelWidget::addExternalLink(const QString& filePath)
{
    if (!m_entry || filePath.isEmpty()) return;

    // Evita duplicata
    for (int i = 0; i < m_localList->count(); ++i) {
        if (m_localList->item(i)->data(Qt::UserRole).toString() == filePath)
            return;
    }

    auto* item = new QListWidgetItem(filePath);
    item->setData(Qt::UserRole, filePath);
    item->setToolTip(filePath);
    m_localList->addItem(item);
    refreshItemIcons(m_localList);
    saveLinks();
}

// ─── Sugestão de apps por extensão ────────────────────────────────────────────

QStringList LinksPanelWidget::suggestAppsForPath(const QString& path)
{
    const QFileInfo fi(path);
    if (!fi.isFile()) return {};

    const QString ext = fi.suffix().toLower();

    // Mapeia extensão → lista de executáveis candidatos
    static const QMap<QString, QStringList> extMap = {
        { "pdf",  {"evince","okular","xreader","zathura","libreoffice"} },
        { "doc",  {"libreoffice","wps","abiword"} },
        { "docx", {"libreoffice","wps","abiword"} },
        { "odt",  {"libreoffice"} },
        { "xls",  {"libreoffice","gnumeric"} },
        { "xlsx", {"libreoffice","gnumeric"} },
        { "ods",  {"libreoffice"} },
        { "ppt",  {"libreoffice"} },
        { "pptx", {"libreoffice"} },
        { "txt",  {"gedit","kate","mousepad","pluma","xed","nano","vim"} },
        { "md",   {"gedit","kate","mousepad","marktext","ghostwriter"} },
        { "py",   {"gedit","kate","spyder","thonny","idle"} },
        { "sh",   {"gedit","kate","bash"} },
        { "mp4",  {"vlc","mpv","totem","celluloid","smplayer"} },
        { "mkv",  {"vlc","mpv","totem","celluloid","smplayer"} },
        { "avi",  {"vlc","mpv","totem","smplayer"} },
        { "mov",  {"vlc","mpv"} },
        { "webm", {"vlc","mpv","totem"} },
        { "mp3",  {"vlc","mpv","rhythmbox","clementine","audacious"} },
        { "ogg",  {"vlc","mpv","rhythmbox","audacious"} },
        { "flac", {"vlc","mpv","rhythmbox","audacious"} },
        { "wav",  {"vlc","mpv","audacity"} },
        { "jpg",  {"eog","shotwell","gimp","gwenview","gthumb"} },
        { "jpeg", {"eog","shotwell","gimp","gwenview","gthumb"} },
        { "png",  {"eog","shotwell","gimp","gwenview","gthumb"} },
        { "gif",  {"eog","gimp","firefox"} },
        { "svg",  {"inkscape","eog","gimp"} },
        { "zip",  {"file-roller","ark","nautilus"} },
        { "tar",  {"file-roller","ark"} },
        { "gz",   {"file-roller","ark"} },
        { "7z",   {"file-roller","ark"} },
        { "rar",  {"file-roller","ark"} },
    };

    QStringList found;
    const QStringList candidates = extMap.value(ext);
    for (const QString& app : candidates) {
        if (!QStandardPaths::findExecutable(app).isEmpty())
            found << app;
    }

    // Tenta via xdg-mime para detectar o padrão do sistema
    QProcess p;
    QMimeDatabase mdb;
    const QString mime = mdb.mimeTypeForFile(path).name();
    p.start("xdg-mime", {"query", "default", mime});
    if (p.waitForFinished(1000)) {
        QString desktop = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
        // remove .desktop, ex: "vlc.desktop" → "vlc"
        if (desktop.endsWith(".desktop"))
            desktop.chop(8);
        if (!desktop.isEmpty() && !found.contains(desktop))
            found.prepend(desktop); // padrão do sistema vai primeiro
    }

    return found;
}

// ─── Menu de contexto ─────────────────────────────────────────────────────────

void LinksPanelWidget::setupContextMenu(QListWidget* list)
{
    list->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(list, &QListWidget::customContextMenuRequested, this,
            [this, list](const QPoint& pos) {
        auto* item = list->itemAt(pos);
        if (!item) return;
        const QString path = item->data(Qt::UserRole).toString();
        const QFileInfo fi(path);

        QMenu menu(this);
        menu.setStyleSheet("QMenu { font-size: 11px; }");

        auto* actOpen = menu.addAction(QIcon::fromTheme("document-open"), tr("📂  Abrir"));
        connect(actOpen, &QAction::triggered, this, [this, path]() { openPath(path); });

        if (fi.isFile()) {
            auto* actFolder = menu.addAction(QIcon::fromTheme("folder"), tr("📁  Abrir pasta que contém"));
            connect(actFolder, &QAction::triggered, this, [this, path]() { openContainingFolder(path); });
        }

        menu.addSeparator();

        auto* actCopy = menu.addAction(QIcon::fromTheme("edit-copy"), tr("⎘  Copiar caminho"));
        connect(actCopy, &QAction::triggered, this, [path]() {
            QApplication::clipboard()->setText(path);
        });

        menu.addSeparator();

        // Submenu "Abrir com"
        auto* subWith = menu.addMenu(QIcon::fromTheme("system-run"), tr("🖥  Abrir com..."));
        const QStringList apps = suggestAppsForPath(path);
        for (const QString& app : apps) {
            auto* act = subWith->addAction(app);
            connect(act, &QAction::triggered, this, [this, path, app]() {
                openWithApp(path, app);
            });
        }
        if (!apps.isEmpty()) subWith->addSeparator();
        auto* actCustom = subWith->addAction(tr("Outro aplicativo..."));
        connect(actCustom, &QAction::triggered, this, [this, path]() { openWith(path); });

        menu.exec(list->viewport()->mapToGlobal(pos));
    });
}

// ─── Abrir com app específico ──────────────────────────────────────────────────

void LinksPanelWidget::openWithApp(const QString& path, const QString& app)
{
    if (!QProcess::startDetached(app, {path})) {
        QMessageBox::warning(this, tr("Erro"),
            tr("Não foi possível abrir '%1' com '%2'.").arg(path, app));
    }
}

void LinksPanelWidget::openWith(const QString& path)
{
    QSettings s("CfdmNote", "CfdmNote");
    const QFileInfo fi(path);
    const QString extKey = "openwith/" + fi.suffix().toLower();
    const QString lastApp = s.value(extKey).toString();

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Abrir com..."));
    dlg.resize(360, 200);
    auto* lay = new QVBoxLayout(&dlg);

    lay->addWidget(new QLabel(tr("<b>Arquivo:</b> %1").arg(fi.fileName())));

    // Sugestões
    const QStringList apps = suggestAppsForPath(path);
    auto* listApps = new QListWidget();
    listApps->setFixedHeight(90);
    listApps->setStyleSheet("font-size:11px;");
    for (const QString& a : apps)
        listApps->addItem(a);
    if (!apps.isEmpty()) lay->addWidget(new QLabel(tr("Apps detectados:")));
    if (!apps.isEmpty()) lay->addWidget(listApps);

    lay->addWidget(new QLabel(tr("Ou digite o comando:")));
    auto* lineEdit = new QLineEdit(lastApp);
    lineEdit->setPlaceholderText("ex: vlc, gimp, code ...");
    lay->addWidget(lineEdit);

    // Clique em item da lista preenche o campo
    connect(listApps, &QListWidget::itemClicked, this, [lineEdit](QListWidgetItem* item) {
        lineEdit->setText(item->text());
    });

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    lay->addWidget(btns);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    const QString cmd = lineEdit->text().trimmed();
    if (cmd.isEmpty()) return;

    s.setValue(extKey, cmd);
    openWithApp(path, cmd);
}

// ─── Abrir pasta que contém ────────────────────────────────────────────────────

void LinksPanelWidget::openContainingFolder(const QString& path)
{
    const QFileInfo fi(path);
    const QString dir = fi.isFile() ? fi.dir().absolutePath() : path;
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
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
