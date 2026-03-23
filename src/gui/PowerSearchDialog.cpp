#include "PowerSearchDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QDirIterator>
#include <QStorageInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QDateTime>
#include <QFileInfo>
#include <QMenu>
#include <QClipboard>
#include <QApplication>
#include <QMessageBox>
#include <QFrame>
#include <QShortcut>
#include <QKeySequence>
#include <QCursor>
#include <QDir>
#include <QIcon>
#include <QProcess>
#include <QScrollArea>
#include <QSettings>
#include <QStandardPaths>

// ─────────────────────────────────────────────────────────────────────────────
// SearchWorker
// ─────────────────────────────────────────────────────────────────────────────

void SearchWorker::setParams(const QString& dir, const QStringList& extensions,
                              const QString& nameFilter, bool recursive)
{
    m_dir        = dir;
    m_extensions = extensions;
    m_nameFilter = nameFilter.trimmed().toLower();
    m_recursive  = recursive;
    m_cancelled  = false;
}

void SearchWorker::start()
{
    const auto flags = m_recursive
        ? QDirIterator::Subdirectories | QDirIterator::FollowSymlinks
        : QDirIterator::NoIteratorFlags;

    QStringList nameFilters;
    for (const QString& ext : m_extensions)
        nameFilters << QString("*.%1").arg(ext);
    if (nameFilters.isEmpty())
        nameFilters << "*";

    QDirIterator it(m_dir, nameFilters, QDir::Files | QDir::Readable, flags);

    while (it.hasNext() && !m_cancelled) {
        const QString path = it.next();
        const QFileInfo fi(path);
        if (!m_nameFilter.isEmpty() &&
            !fi.fileName().toLower().contains(m_nameFilter))
            continue;
        emit resultFound(fi.fileName(), fi.absoluteFilePath(),
                         fi.size(), fi.lastModified());
    }
    emit finished();
}

// ─────────────────────────────────────────────────────────────────────────────
// Browser detection + openUrl
// ─────────────────────────────────────────────────────────────────────────────

/*static*/
QList<PowerSearchDialog::BrowserInfo> PowerSearchDialog::detectBrowsers()
{
    struct Candidate {
        QString name;
        QStringList execs;   // tenta em ordem; usa o primeiro encontrado
        QStringList args;    // modo privado
    };

    const QList<Candidate> known = {
        {"Brave",              {"brave-browser","brave"},
                               {"--incognito"}},
        {"LibreWolf",          {"librewolf"},
                               {"--private-window"}},
        {"Firefox",            {"firefox","firefox-esr"},
                               {"--private-window"}},
        {"Waterfox",           {"waterfox","waterfox-current"},
                               {"--private-window"}},
        {"Mullvad Browser",    {"mullvad-browser"},
                               {"--private-window"}},
        {"Vivaldi",            {"vivaldi","vivaldi-stable"},
                               {"--incognito"}},
        {"Ungoogled Chromium", {"ungoogled-chromium","chromium-browser","chromium"},
                               {"--incognito"}},
        {"Tor Browser",        {"tor-browser","torbrowser-launcher"},
                               {}},
        {"Google Chrome",      {"google-chrome","google-chrome-stable"},
                               {"--incognito"}},
        {"Microsoft Edge",     {"microsoft-edge","microsoft-edge-stable"},
                               {"--inprivate"}},
        {"Opera",              {"opera"},
                               {"--private"}},
    };

    QList<BrowserInfo> found;
    // "Padrão do sistema" sempre aparece primeiro
    found.append({tr("Padrão do sistema"), {}, {}});

    for (const auto& c : known) {
        for (const QString& exec : c.execs) {
            QProcess p;
            p.start("which", {exec});
            if (p.waitForFinished(500) && p.exitCode() == 0) {
                found.append({c.name, exec, c.args});
                break;
            }
        }
    }
    return found;
}

void PowerSearchDialog::openUrl(const QString& urlStr)
{
    if (!m_browserCombo) {
        QDesktopServices::openUrl(QUrl(urlStr));
        return;
    }

    const QVariant data = m_browserCombo->currentData();
    if (!data.isValid() || data.toString().isEmpty()) {
        QDesktopServices::openUrl(QUrl(urlStr));
        return;
    }

    // data = "exe|arg1|arg2"
    const QStringList parts = data.toString().split('|');
    const QString     exe   = parts.value(0);
    QStringList       args  = parts.mid(1);
    if (!args.isEmpty() && args.last().isEmpty())
        args.removeLast();
    args << urlStr;

    if (!QProcess::startDetached(exe, args)) {
        // Fallback para o padrão se o browser falhar
        QDesktopServices::openUrl(QUrl(urlStr));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// PowerSearchDialog — construtor / destrutor
// ─────────────────────────────────────────────────────────────────────────────

PowerSearchDialog::PowerSearchDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Pesquisa Poderosa"));
    setMinimumSize(860, 600);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);

    // ── Seletor de navegador (barra global no topo) ───────────────────────────
    auto* browserRow = new QHBoxLayout();
    browserRow->addWidget(new QLabel(tr("🌐 Abrir links com:")));
    m_browserCombo = new QComboBox();
    m_browserCombo->setToolTip(
        tr("Escolha o navegador para abrir todas as buscas e links.\n"
           "Navegadores com modo privado já são iniciados nesse modo."));
    m_browserCombo->setMinimumWidth(200);

    const auto browsers = detectBrowsers();
    for (const auto& b : browsers) {
        // data = "exe|arg1|arg2" ou vazio para padrão
        QString data;
        if (!b.exe.isEmpty()) {
            data = b.exe;
            for (const QString& a : b.args)
                data += "|" + a;
        }
        m_browserCombo->addItem(b.name, data);
    }

    // Restaura última seleção
    QSettings cfg;
    const QString saved = cfg.value("powersearch/browser").toString();
    if (!saved.isEmpty()) {
        for (int i = 0; i < m_browserCombo->count(); ++i) {
            if (m_browserCombo->itemData(i).toString() == saved) {
                m_browserCombo->setCurrentIndex(i);
                break;
            }
        }
    }
    connect(m_browserCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
                QSettings cfg2;
                cfg2.setValue("powersearch/browser",
                              m_browserCombo->currentData().toString());
            });

    browserRow->addWidget(m_browserCombo);
    browserRow->addStretch();

    // Badge: quantos browsers privados foram encontrados
    const int privCount = browsers.size() - 1;  // -1 pelo "Padrão"
    if (privCount > 0) {
        auto* badge = new QLabel(
            tr("✅ %1 navegador(es) privado(s) detectado(s)").arg(privCount));
        badge->setStyleSheet("font-size: 11px; color: #276749;");
        browserRow->addWidget(badge);
    } else {
        auto* badge = new QLabel(tr("⚠ Nenhum navegador privado detectado"));
        badge->setStyleSheet("font-size: 11px; color: #c05621;");
        browserRow->addWidget(badge);
    }
    layout->addLayout(browserRow);

    auto* tabs = new QTabWidget();
    setupLocalTab(tabs);
    setupWebTab(tabs);
    setupLinkCheckTab(tabs);
    setupTrendingTab(tabs);
    layout->addWidget(tabs);

    auto* btnClose = new QPushButton(tr("Fechar"));
    btnClose->setFixedHeight(28);
    auto* bottomRow = new QHBoxLayout();
    bottomRow->addStretch();
    bottomRow->addWidget(btnClose);
    layout->addLayout(bottomRow);

    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    connect(new QShortcut(QKeySequence(Qt::Key_Escape), this),
            &QShortcut::activated, this, &QDialog::accept);
}

PowerSearchDialog::~PowerSearchDialog()
{
    if (m_worker) m_worker->cancel();
    if (m_thread && m_thread->isRunning()) {
        m_thread->quit();
        m_thread->wait(3000);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Aba 1: Arquivos Locais
// ─────────────────────────────────────────────────────────────────────────────

void PowerSearchDialog::setupLocalTab(QTabWidget* tabs)
{
    auto* page   = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(6);

    // ── Grade de filtros ──────────────────────────────────────────────────────
    auto* grid = new QGridLayout();
    grid->setSpacing(6);

    grid->addWidget(new QLabel(tr("Nome / padrão:")), 0, 0, Qt::AlignRight);
    m_nameFilter = new QLineEdit();
    m_nameFilter->setPlaceholderText(tr("Ex.: relatorio   (vazio = todos os arquivos)"));
    grid->addWidget(m_nameFilter, 0, 1);

    grid->addWidget(new QLabel(tr("Tipo:")), 0, 2, Qt::AlignRight);
    m_typeCombo = new QComboBox();
    m_typeCombo->addItem(tr("Todos (*)"));
    m_typeCombo->addItem(tr("Documentos  — txt, doc, docx, pdf, odt, md"));
    m_typeCombo->addItem(tr("Imagens  — jpg, png, gif, bmp, svg, webp"));
    m_typeCombo->addItem(tr("Áudio / Vídeo  — mp3, mp4, avi, mkv, wav"));
    m_typeCombo->addItem(tr("Código-Fonte  — cpp, h, py, js, sh, java"));
    m_typeCombo->addItem(tr("Compactados  — zip, tar, gz, 7z, rar"));
    m_typeCombo->addItem(tr("Personalizado..."));
    grid->addWidget(m_typeCombo, 0, 3);

    grid->addWidget(new QLabel(tr("Diretório:")), 1, 0, Qt::AlignRight);
    m_dirEdit = new QLineEdit();
    m_dirEdit->setPlaceholderText(tr("Selecione um diretório ou mídia..."));
    grid->addWidget(m_dirEdit, 1, 1);

    auto* btnBrowse = new QPushButton(tr("Pasta..."));
    btnBrowse->setFixedWidth(72);
    grid->addWidget(btnBrowse, 1, 2);

    auto* btnMedia = new QPushButton(tr("Mídias ▼"));
    btnMedia->setFixedWidth(90);
    grid->addWidget(btnMedia, 1, 3);

    layout->addLayout(grid);

    // ── Linha de extensão personalizada (oculta) ──────────────────────────────
    m_customRow = new QWidget();
    auto* customLayout = new QHBoxLayout(m_customRow);
    customLayout->setContentsMargins(0, 0, 0, 0);
    customLayout->addWidget(new QLabel(tr("Extensões (separadas por vírgula):")));
    m_customExt = new QLineEdit();
    m_customExt->setPlaceholderText(tr("Ex.: pdf, docx, xlsx, epub"));
    customLayout->addWidget(m_customExt);
    m_customRow->setVisible(false);
    layout->addWidget(m_customRow);

    // ── Opções + botões ───────────────────────────────────────────────────────
    auto* optRow = new QHBoxLayout();
    m_recursive = new QCheckBox(tr("Incluir subpastas"));
    m_recursive->setChecked(true);
    optRow->addWidget(m_recursive);
    optRow->addStretch();
    m_btnSearch = new QPushButton(QIcon::fromTheme("system-search"), tr("  Buscar"));
    m_btnSearch->setFixedHeight(30);
    m_btnCancel = new QPushButton(tr("Cancelar"));
    m_btnCancel->setFixedHeight(30);
    m_btnCancel->setEnabled(false);
    optRow->addWidget(m_btnSearch);
    optRow->addWidget(m_btnCancel);
    layout->addLayout(optRow);

    // ── Separador ─────────────────────────────────────────────────────────────
    auto* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    // ── Tabela de resultados ──────────────────────────────────────────────────
    m_results = new QTreeWidget();
    m_results->setColumnCount(4);
    m_results->setHeaderLabels({tr("Nome"), tr("Caminho"), tr("Tamanho"), tr("Modificado")});
    m_results->setAlternatingRowColors(true);
    m_results->setRootIsDecorated(false);
    m_results->setSortingEnabled(true);
    m_results->setContextMenuPolicy(Qt::CustomContextMenu);
    m_results->header()->setStretchLastSection(false);
    m_results->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_results->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_results->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_results->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    layout->addWidget(m_results, 1);

    // ── Rodapé ────────────────────────────────────────────────────────────────
    auto* footRow = new QHBoxLayout();
    m_statusLocal = new QLabel(tr("Aguardando busca..."));
    m_statusLocal->setStyleSheet("font-size: 11px; color: #4a5568;");
    footRow->addWidget(m_statusLocal);
    footRow->addStretch();
    auto* btnCopy = new QPushButton(tr("Copiar caminho"));
    btnCopy->setFixedHeight(26);
    auto* btnOpen = new QPushButton(tr("Abrir arquivo"));
    btnOpen->setFixedHeight(26);
    footRow->addWidget(btnCopy);
    footRow->addWidget(btnOpen);
    layout->addLayout(footRow);

    tabs->addTab(page, QIcon::fromTheme("system-search"), tr("Arquivos Locais"));

    // ── Conexões ──────────────────────────────────────────────────────────────
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PowerSearchDialog::onTypeChanged);
    connect(btnBrowse, &QPushButton::clicked, this, &PowerSearchDialog::onBrowseDir);
    connect(btnMedia,  &QPushButton::clicked, this, &PowerSearchDialog::onDetectMedia);
    connect(m_btnSearch, &QPushButton::clicked, this, &PowerSearchDialog::onSearch);
    connect(m_btnCancel, &QPushButton::clicked, this, &PowerSearchDialog::onCancelSearch);
    connect(m_results, &QTreeWidget::itemDoubleClicked,
            this, &PowerSearchDialog::onItemDoubleClicked);
    connect(btnCopy, &QPushButton::clicked, this, &PowerSearchDialog::onCopyPath);
    connect(btnOpen, &QPushButton::clicked, this, [this]() {
        onItemDoubleClicked(m_results->currentItem(), 0);
    });
    connect(m_results, &QTreeWidget::customContextMenuRequested,
            this, [this](const QPoint& pos) {
                if (!m_results->itemAt(pos)) return;
                QMenu menu;
                menu.addAction(tr("Abrir arquivo"), this, [this]() {
                    onItemDoubleClicked(m_results->currentItem(), 0);
                });
                menu.addAction(tr("Abrir pasta"), this, [this]() {
                    if (auto* item = m_results->currentItem()) {
                        const QString path = item->data(0, Qt::UserRole).toString();
                        QDesktopServices::openUrl(
                            QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
                    }
                });
                menu.addSeparator();
                menu.addAction(tr("Copiar caminho"), this,
                               &PowerSearchDialog::onCopyPath);
                menu.exec(m_results->viewport()->mapToGlobal(pos));
            });
}

// ─────────────────────────────────────────────────────────────────────────────
// Aba 2: Busca na Web + Google Dorking
// ─────────────────────────────────────────────────────────────────────────────

void PowerSearchDialog::setupWebTab(QTabWidget* tabs)
{
    auto* page   = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(8);

    // ── Busca rápida ──────────────────────────────────────────────────────────
    auto* grpQuick = new QGroupBox(tr("Busca Rápida"));
    auto* qLayout  = new QVBoxLayout(grpQuick);

    auto* qRow = new QHBoxLayout();
    qRow->addWidget(new QLabel(tr("Termo:")));
    m_webQuery = new QLineEdit();
    m_webQuery->setPlaceholderText(tr("O que deseja buscar?"));
    qRow->addWidget(m_webQuery, 1);
    qLayout->addLayout(qRow);

    const QList<QPair<QString,QString>> engines = {
        {tr("Google"),     "google"},
        {tr("GitHub"),     "github"},
        {tr("YouTube"),    "youtube"},
        {tr("Wikipedia"),  "wikipedia"},
        {tr("DuckDuckGo"), "duckduckgo"},
    };
    auto* engRow = new QHBoxLayout();
    for (const auto& [label, key] : engines) {
        auto* btn = new QPushButton(label);
        btn->setFixedHeight(28);
        const QString k = key;
        connect(btn, &QPushButton::clicked, this, [this, k]() { onWebSearch(k); });
        engRow->addWidget(btn);
    }
    engRow->addStretch();
    qLayout->addLayout(engRow);
    layout->addWidget(grpQuick);

    // ── Google Dorking ────────────────────────────────────────────────────────
    auto* grpDork = new QGroupBox(tr("Google Dorking (busca avançada)"));
    auto* dLayout = new QVBoxLayout(grpDork);

    auto* dGrid = new QGridLayout();
    dGrid->setSpacing(6);

    auto addDorkField = [&](int row, int col, const QString& label,
                            QLineEdit*& field, const QString& hint) {
        dGrid->addWidget(new QLabel(label), row, col * 2, Qt::AlignRight);
        field = new QLineEdit();
        field->setPlaceholderText(hint);
        dGrid->addWidget(field, row, col * 2 + 1);
    };

    addDorkField(0, 0, "site:",     m_dorkSite,     "ex.: github.com");
    addDorkField(0, 1, "filetype:", m_dorkFiletype, "pdf, docx...");
    addDorkField(1, 0, "intitle:",  m_dorkIntitle,  "palavra no título");
    addDorkField(1, 1, "inurl:",    m_dorkInurl,    "palavra na URL");
    addDorkField(2, 0, tr("Frase exata:"), m_dorkExact, "\"entre aspas\"");
    addDorkField(2, 1, tr("Termos livres:"), m_dorkTerm, tr("palavras adicionais"));

    dLayout->addLayout(dGrid);

    m_dorkPreview = new QLabel(tr("(preencha os campos para ver a query)"));
    m_dorkPreview->setWordWrap(true);
    m_dorkPreview->setStyleSheet(
        "font-family: monospace; font-size: 11px; color: #2c5282;"
        "background: #ebf8ff; border: 1px solid #bee3f8;"
        "padding: 6px 8px; border-radius: 4px;");
    dLayout->addWidget(m_dorkPreview);

    auto* dorkRow = new QHBoxLayout();
    dorkRow->addStretch();
    auto* btnDork = new QPushButton(QIcon::fromTheme("system-search"),
                                     tr("  Buscar com Dork"));
    btnDork->setFixedHeight(30);
    dorkRow->addWidget(btnDork);
    dLayout->addLayout(dorkRow);

    layout->addWidget(grpDork);
    layout->addStretch();

    tabs->addTab(page, QIcon::fromTheme("internet-web-browser"), tr("Busca na Web"));

    // ── Conexões dork ─────────────────────────────────────────────────────────
    auto updatePreview = [this]() {
        const QString q = buildDorkQuery();
        if (q.isEmpty())
            m_dorkPreview->setText(tr("(preencha os campos para ver a query)"));
        else
            m_dorkPreview->setText(tr("Query: <b>%1</b>").arg(q.toHtmlEscaped()));
    };

    for (QLineEdit* f : {m_dorkSite, m_dorkFiletype, m_dorkIntitle,
                         m_dorkInurl, m_dorkExact, m_dorkTerm})
        connect(f, &QLineEdit::textChanged, this, updatePreview);

    connect(btnDork, &QPushButton::clicked, this, [this]() {
        const QString q = buildDorkQuery();
        if (q.isEmpty()) {
            QMessageBox::warning(this, tr("Aviso"),
                tr("Preencha pelo menos um campo para buscar."));
            return;
        }
        const QString enc = QString::fromUtf8(QUrl::toPercentEncoding(q));
        openUrl("https://www.google.com/search?q=" + enc);
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Aba 3: Verificar Link
// ─────────────────────────────────────────────────────────────────────────────

void PowerSearchDialog::setupLinkCheckTab(QTabWidget* tabs)
{
    auto* page   = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(8);

    auto* warn = new QLabel(
        tr("<b>⚠ Atenção:</b> a URL é enviada para serviços externos de terceiros.<br>"
           "Não use com URLs que contenham senhas ou dados confidenciais."));
    warn->setWordWrap(true);
    warn->setStyleSheet(
        "background: #fffbeb; border: 1px solid #f6e05e;"
        "color: #744210; padding: 8px 10px; border-radius: 4px;");
    layout->addWidget(warn);

    auto* urlRow = new QHBoxLayout();
    urlRow->addWidget(new QLabel(tr("URL a verificar:")));
    m_urlEdit = new QLineEdit();
    m_urlEdit->setPlaceholderText("https://...");
    urlRow->addWidget(m_urlEdit, 1);
    layout->addLayout(urlRow);

    auto* grp = new QGroupBox(tr("Serviços de verificação"));
    auto* sLayout = new QVBoxLayout(grp);
    sLayout->setSpacing(10);

    struct Service { QString label; QString key; QString color; QString desc; };
    const QList<Service> services = {
        {"VirusTotal",          "virustotal",  "#e53e3e",
         tr("Copia a URL e abre o VirusTotal (cole no campo de busca)")},
        {"Google Safe Browsing","googlesafe",  "#3182ce",
         tr("Relatório de transparência do Google para a URL")},
        {"URLVoid",             "urlvoid",     "#38a169",
         tr("Reputação do domínio em múltiplos serviços de blacklist")},
    };

    for (const auto& svc : services) {
        auto* row = new QHBoxLayout();
        auto* btn = new QPushButton(svc.label);
        btn->setFixedHeight(32);
        btn->setMinimumWidth(180);
        btn->setStyleSheet(
            QString("font-weight: bold; background: %1; color: white;"
                    "border-radius: 4px; padding: 0 12px;").arg(svc.color));
        row->addWidget(btn);
        auto* desc = new QLabel(svc.desc);
        desc->setStyleSheet("font-size: 11px; color: #4a5568;");
        row->addWidget(desc);
        row->addStretch();
        sLayout->addLayout(row);

        const QString k = svc.key;
        connect(btn, &QPushButton::clicked, this,
                [this, k]() { onCheckUrl(k); });
    }

    layout->addWidget(grp);

    m_urlStatus = new QLabel();
    m_urlStatus->setStyleSheet("font-size: 11px; color: #2c5282; padding: 4px 0;");
    layout->addWidget(m_urlStatus);
    layout->addStretch();

    tabs->addTab(page, QIcon::fromTheme("security-high"), tr("Verificar Link"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

QStringList PowerSearchDialog::extensionsForCurrentType() const
{
    switch (m_typeCombo->currentIndex()) {
        case 0: return {};
        case 1: return {"txt","doc","docx","pdf","odt","rtf","md"};
        case 2: return {"jpg","jpeg","png","gif","bmp","svg","webp","tiff"};
        case 3: return {"mp3","mp4","avi","mkv","wav","ogg","flac","mov","wmv"};
        case 4: return {"cpp","h","hpp","py","js","ts","sh","java","cs","go","rs","rb"};
        case 5: return {"zip","tar","gz","bz2","7z","rar","xz"};
        default: {
            QStringList exts;
            for (const QString& e : m_customExt->text().split(','))
                if (!e.trimmed().isEmpty())
                    exts << e.trimmed().toLower();
            return exts;
        }
    }
}

QString PowerSearchDialog::buildDorkQuery() const
{
    QStringList parts;
    if (!m_dorkSite->text().trimmed().isEmpty())
        parts << "site:" + m_dorkSite->text().trimmed();
    if (!m_dorkFiletype->text().trimmed().isEmpty())
        parts << "filetype:" + m_dorkFiletype->text().trimmed();
    if (!m_dorkIntitle->text().trimmed().isEmpty())
        parts << "intitle:\"" + m_dorkIntitle->text().trimmed() + "\"";
    if (!m_dorkInurl->text().trimmed().isEmpty())
        parts << "inurl:" + m_dorkInurl->text().trimmed();
    if (!m_dorkExact->text().trimmed().isEmpty())
        parts << "\"" + m_dorkExact->text().trimmed() + "\"";
    if (!m_dorkTerm->text().trimmed().isEmpty())
        parts << m_dorkTerm->text().trimmed();
    return parts.join(" ");
}

// ─────────────────────────────────────────────────────────────────────────────
// Slots — Arquivos Locais
// ─────────────────────────────────────────────────────────────────────────────

void PowerSearchDialog::onBrowseDir()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Selecionar diretório"),
        m_dirEdit->text().isEmpty() ? QDir::homePath() : m_dirEdit->text());
    if (!dir.isEmpty())
        m_dirEdit->setText(dir);
}

void PowerSearchDialog::onDetectMedia()
{
    auto* menu = new QMenu(this);
    menu->addAction(QIcon::fromTheme("user-home"), tr("Pasta Home"), this,
                    [this]() { m_dirEdit->setText(QDir::homePath()); });
    menu->addAction(QIcon::fromTheme("folder"), tr("Raiz (/)"), this,
                    [this]() { m_dirEdit->setText("/"); });
    menu->addSeparator();

    bool found = false;
    for (const QStorageInfo& vol : QStorageInfo::mountedVolumes()) {
        if (!vol.isValid() || vol.isReadOnly()) continue;
        const QString dev = vol.device();
        const bool isRemovable =
            dev.startsWith("/dev/sd") || dev.startsWith("/dev/nvme") ||
            dev.startsWith("/dev/mmcblk");
        const bool isMountedMedia =
            vol.rootPath().startsWith("/media") ||
            vol.rootPath().startsWith("/run/media");
        if (!isRemovable && !isMountedMedia) continue;
        if (vol.rootPath() == "/") continue;

        const QString label = vol.name().isEmpty() ? dev : vol.name();
        const QString path  = vol.rootPath();
        menu->addAction(QIcon::fromTheme("drive-removable-media"),
                        QString("%1 (%2)").arg(label, path),
                        this, [this, path]() { m_dirEdit->setText(path); });
        found = true;
    }

    if (!found)
        menu->addAction(tr("(nenhuma mídia removível detectada)"))->setEnabled(false);

    menu->exec(QCursor::pos());
}

void PowerSearchDialog::onTypeChanged(int index)
{
    m_customRow->setVisible(index == m_typeCombo->count() - 1);
}

void PowerSearchDialog::onSearch()
{
    const QString dir = m_dirEdit->text().trimmed();
    if (dir.isEmpty()) {
        QMessageBox::warning(this, tr("Aviso"),
            tr("Selecione um diretório primeiro."));
        return;
    }
    if (!QDir(dir).exists()) {
        QMessageBox::warning(this, tr("Aviso"),
            tr("O diretório '%1' não existe.").arg(dir));
        return;
    }

    m_results->clear();
    m_foundCount = 0;
    m_statusLocal->setText(tr("Buscando..."));
    m_btnSearch->setEnabled(false);
    m_btnCancel->setEnabled(true);

    // Para busca anterior, se houver
    if (m_thread && m_thread->isRunning()) {
        if (m_worker) m_worker->cancel();
        m_thread->quit();
        m_thread->wait(3000);
    }

    m_worker = new SearchWorker();
    m_worker->setParams(dir, extensionsForCurrentType(),
                        m_nameFilter->text(), m_recursive->isChecked());

    m_thread = new QThread();
    m_worker->moveToThread(m_thread);

    connect(m_thread, &QThread::started,
            m_worker, &SearchWorker::start);
    connect(m_worker, &SearchWorker::resultFound,
            this,     &PowerSearchDialog::onResultFound,
            Qt::QueuedConnection);
    connect(m_worker, &SearchWorker::finished,
            this,     &PowerSearchDialog::onSearchFinished,
            Qt::QueuedConnection);
    connect(m_worker, &SearchWorker::finished,
            m_thread, &QThread::quit);
    connect(m_thread, &QThread::finished,
            m_worker, &QObject::deleteLater);
    connect(m_thread, &QThread::finished,
            m_thread, &QObject::deleteLater);
    connect(m_thread, &QThread::finished, this, [this]() {
        m_thread = nullptr;
        m_worker = nullptr;
    });

    m_thread->start();
}

void PowerSearchDialog::onCancelSearch()
{
    if (m_worker) m_worker->cancel();
    m_btnCancel->setEnabled(false);
    m_statusLocal->setText(tr("Cancelado — %1 arquivo(s) encontrado(s).").arg(m_foundCount));
}

void PowerSearchDialog::onResultFound(const QString& name, const QString& path,
                                       qint64 size, const QDateTime& modified)
{
    ++m_foundCount;

    auto* item = new QTreeWidgetItem(m_results);
    item->setText(0, name);
    item->setText(1, path);
    item->setData(0, Qt::UserRole, path);

    if (size < 1024)
        item->setText(2, QString("%1 B").arg(size));
    else if (size < 1024 * 1024)
        item->setText(2, QString("%1 KB").arg(size / 1024.0, 0, 'f', 1));
    else
        item->setText(2, QString("%1 MB").arg(size / (1024.0 * 1024), 0, 'f', 2));

    item->setText(3, modified.toString("dd/MM/yyyy HH:mm"));

    static const QMap<QString,QString> iconMap = {
        {"pdf","application-pdf"}, {"txt","text-x-generic"},
        {"doc","x-office-document"}, {"docx","x-office-document"},
        {"odt","x-office-document"}, {"md","text-x-generic"},
        {"jpg","image-x-generic"}, {"jpeg","image-x-generic"},
        {"png","image-x-generic"}, {"gif","image-x-generic"},
        {"mp3","audio-x-generic"}, {"mp4","video-x-generic"},
        {"avi","video-x-generic"}, {"mkv","video-x-generic"},
        {"zip","package-x-generic"}, {"gz","package-x-generic"},
        {"cpp","text-x-csrc"}, {"h","text-x-chdr"},
        {"py","text-x-python"}, {"sh","text-x-script"},
    };
    const QString ext = QFileInfo(name).suffix().toLower();
    item->setIcon(0, QIcon::fromTheme(iconMap.value(ext, "text-x-generic")));

    if (m_foundCount % 10 == 0)
        m_statusLocal->setText(tr("Buscando... %1 encontrado(s)").arg(m_foundCount));
}

void PowerSearchDialog::onSearchFinished()
{
    m_btnSearch->setEnabled(true);
    m_btnCancel->setEnabled(false);
    m_statusLocal->setText(
        m_foundCount == 0
            ? tr("Nenhum arquivo encontrado.")
            : tr("%1 arquivo(s) encontrado(s).").arg(m_foundCount));
    m_results->resizeColumnToContents(0);
}

void PowerSearchDialog::onItemDoubleClicked(QTreeWidgetItem* item, int)
{
    if (!item) return;
    const QString path = item->data(0, Qt::UserRole).toString();
    if (!path.isEmpty())
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void PowerSearchDialog::onCopyPath()
{
    auto* item = m_results->currentItem();
    if (!item) return;
    const QString path = item->data(0, Qt::UserRole).toString();
    if (!path.isEmpty())
        QApplication::clipboard()->setText(path);
}

// ─────────────────────────────────────────────────────────────────────────────
// Slots — Busca na Web
// ─────────────────────────────────────────────────────────────────────────────

void PowerSearchDialog::onWebSearch(const QString& engine)
{
    const QString q = m_webQuery->text().trimmed();
    if (q.isEmpty()) {
        QMessageBox::warning(this, tr("Aviso"), tr("Digite um termo de busca."));
        return;
    }
    const QString enc = QString::fromUtf8(QUrl::toPercentEncoding(q));

    QString urlStr;
    if      (engine == "google")     urlStr = "https://www.google.com/search?q=" + enc;
    else if (engine == "github")     urlStr = "https://github.com/search?q=" + enc;
    else if (engine == "youtube")    urlStr = "https://www.youtube.com/results?search_query=" + enc;
    else if (engine == "wikipedia")  urlStr = "https://pt.wikipedia.org/wiki/Special:Search?search=" + enc;
    else if (engine == "duckduckgo") urlStr = "https://duckduckgo.com/?q=" + enc;

    if (!urlStr.isEmpty())
        openUrl(urlStr);
}

// ─────────────────────────────────────────────────────────────────────────────
// Slots — Verificar Link
// ─────────────────────────────────────────────────────────────────────────────

void PowerSearchDialog::onCheckUrl(const QString& service)
{
    const QString url = m_urlEdit->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::warning(this, tr("Aviso"),
            tr("Digite uma URL para verificar."));
        return;
    }

    if (service == "virustotal") {
        QApplication::clipboard()->setText(url);
        m_urlStatus->setText(
            tr("✅ URL copiada. Cole no campo de busca do VirusTotal."));
        openUrl("https://www.virustotal.com/gui/home/url");
    }
    else if (service == "googlesafe") {
        const QString enc = QString::fromUtf8(QUrl::toPercentEncoding(url));
        openUrl("https://transparencyreport.google.com/safe-browsing/search?url=" + enc);
        m_urlStatus->setText(tr("Abrindo Google Safe Browsing..."));
    }
    else if (service == "urlvoid") {
        const QString fullUrl = url.startsWith("http") ? url : "https://" + url;
        const QString domain  = QUrl(fullUrl).host();
        if (domain.isEmpty()) {
            QMessageBox::warning(this, tr("Aviso"),
                tr("Não foi possível extrair o domínio da URL."));
            return;
        }
        openUrl("https://www.urlvoid.com/scan/" + domain + "/");
        m_urlStatus->setText(tr("Verificando domínio: %1").arg(domain));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Aba 4: Em Alta / Trending
// ─────────────────────────────────────────────────────────────────────────────

void PowerSearchDialog::setupTrendingTab(QTabWidget* tabs)
{
    // Scroll area para caber tudo sem redimensionar o diálogo
    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* page   = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(10);
    layout->setContentsMargins(6, 6, 6, 6);

    // ── Helper: cria grupo com grade de botões ────────────────────────────────
    auto makeGroup = [&](const QString& title,
                          const QList<QPair<QString,QString>>& items,
                          const QString& color) -> QGroupBox*
    {
        auto* grp = new QGroupBox(title);
        grp->setStyleSheet(
            QString("QGroupBox { font-weight: bold; color: %1;"
                    " border: 1px solid %1; border-radius: 6px;"
                    " margin-top: 8px; padding-top: 6px; }"
                    "QGroupBox::title { subcontrol-origin: margin;"
                    " left: 8px; padding: 0 4px; }").arg(color));
        auto* flow = new QGridLayout(grp);
        flow->setSpacing(6);

        int col = 0, row = 0;
        for (const auto& [label, url] : items) {
            auto* btn = new QPushButton(label);
            btn->setFixedHeight(30);
            btn->setStyleSheet(
                QString("QPushButton { border: 1px solid %1; border-radius: 5px;"
                        " color: %1; padding: 0 10px; }"
                        "QPushButton:hover { background: %1; color: white; }").arg(color));
            const QString u = url;
            connect(btn, &QPushButton::clicked, this, [this, u]() { openUrl(u); });
            flow->addWidget(btn, row, col);
            ++col;
            if (col >= 3) { col = 0; ++row; }
        }
        return grp;
    };

    // ── Vídeo & Conteúdo ──────────────────────────────────────────────────────
    layout->addWidget(makeGroup(tr("▶  Vídeo & Conteúdo"), {
        {tr("YouTube Trending BR"),   "https://www.youtube.com/feed/trending?gl=BR"},
        {tr("YouTube Música BR"),     "https://www.youtube.com/feed/trending?gl=BR&bp=4gINGgt5dG1hX2NoYXJ0cw%3D%3D"},
        {tr("YouTube Gaming"),        "https://www.youtube.com/feed/trending?gl=BR&bp=4gIcGhpnYW1pbmdfY29ycHVzX21vc3RfcG9wdWxhcg%3D%3D"},
        {tr("TikTok Trending"),       "https://www.tiktok.com/trending"},
        {tr("Kwai Trending"),         "https://www.kwai.com/"},
        {tr("Vimeo Staff Picks"),     "https://vimeo.com/channels/staffpicks"},
    }, "#e53e3e"));

    // ── Redes Sociais ─────────────────────────────────────────────────────────
    layout->addWidget(makeGroup(tr("👥  Redes Sociais"), {
        {tr("X / Twitter Trending"),  "https://twitter.com/explore/tabs/trending"},
        {tr("Instagram Explore"),     "https://www.instagram.com/explore/"},
        {tr("Facebook Trending"),     "https://www.facebook.com/"},
        {tr("Pinterest Trending"),    "https://www.pinterest.com/"},
        {tr("LinkedIn Notícias"),     "https://www.linkedin.com/news/"},
        {tr("Mastodon Federated"),    "https://mastodon.social/explore"},
    }, "#3182ce"));

    // ── Tech & Dev ────────────────────────────────────────────────────────────
    layout->addWidget(makeGroup(tr("💻  Tech & Dev"), {
        {tr("GitHub Trending"),       "https://github.com/trending"},
        {tr("Hacker News"),           "https://news.ycombinator.com/"},
        {tr("Product Hunt"),          "https://www.producthunt.com/"},
        {tr("DEV.to"),                "https://dev.to/"},
        {tr("Lobste.rs"),             "https://lobste.rs/"},
        {tr("Reddit r/programming"),  "https://www.reddit.com/r/programming/hot/"},
    }, "#38a169"));

    // ── Notícias & Tendências ─────────────────────────────────────────────────
    layout->addWidget(makeGroup(tr("📰  Notícias & Tendências"), {
        {tr("Google Trends BR"),      "https://trends.google.com.br/trending?geo=BR"},
        {tr("Google News BR"),        "https://news.google.com/topstories?hl=pt-BR&gl=BR&ceid=BR:pt-419"},
        {tr("Reddit r/all Hot"),      "https://www.reddit.com/r/all/hot/"},
        {tr("Reddit r/worldnews"),    "https://www.reddit.com/r/worldnews/hot/"},
        {tr("Feedly Trending"),       "https://feedly.com/i/discover"},
        {tr("Flipboard Trending"),    "https://flipboard.com/"},
    }, "#d69e2e"));

    // ── Para Criadores de Conteúdo ────────────────────────────────────────────
    layout->addWidget(makeGroup(tr("🎯  Para Criadores de Conteúdo"), {
        {tr("AnswerThePublic"),        "https://answerthepublic.com/"},
        {tr("Google Trends Comparar"),"https://trends.google.com.br/trends/explore?geo=BR"},
        {tr("BuzzSumo (free)"),        "https://buzzsumo.com/"},
        {tr("Exploding Topics"),       "https://explodingtopics.com/"},
        {tr("SparkToro Trending"),     "https://sparktoro.com/trending"},
        {tr("Semrush Keyword Free"),   "https://www.semrush.com/analytics/keywordoverview/"},
    }, "#805ad5"));

    // ── Compras & Mercado ─────────────────────────────────────────────────────
    layout->addWidget(makeGroup(tr("🛒  Compras & Mercado"), {
        {tr("Amazon Best Sellers BR"), "https://www.amazon.com.br/gp/bestsellers/"},
        {tr("Mercado Livre Mais Vend."),"https://www.mercadolivre.com.br/mais-vendidos/"},
        {tr("Hotmart Top Produtos"),   "https://hotmart.com/pt-BR/marketplace"},
        {tr("Monetizze Top"),          "https://monetizze.com.br/marketplace/"},
        {tr("Eduzz Marketplace"),      "https://eduzz.com/marketplace"},
        {tr("Shopee Trending BR"),     "https://shopee.com.br/"},
    }, "#dd6b20"));

    layout->addStretch();
    scroll->setWidget(page);
    tabs->addTab(scroll, QIcon::fromTheme("go-up"), tr("📈 Em Alta"));
}
