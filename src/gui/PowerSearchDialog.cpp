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
#include <QHash>
#include <algorithm>

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

    // Tooltip escuro e rico para todo o diálogo
    setStyleSheet(
        "QToolTip {"
        "  background-color: #1a202c;"
        "  color: #e2e8f0;"
        "  border: 1px solid #4a5568;"
        "  border-radius: 6px;"
        "  padding: 8px 10px;"
        "  font-size: 12px;"
        "}");

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

    struct Engine { QString label; QString key; QString tip; };
    const QList<Engine> engines = {
        {tr("Google"),     "google",
         tr("<b>Google</b><br>Busca geral em todos os sites indexados.<br><i>Melhor para: notícias, produtos, sites.</i>")},
        {tr("GitHub"),     "github",
         tr("<b>GitHub</b><br>Repositórios e projetos de código-fonte.<br><i>Melhor para: bibliotecas, ferramentas, exemplos.</i>")},
        {tr("YouTube"),    "youtube",
         tr("<b>YouTube</b><br>Vídeos, tutoriais e transmissões ao vivo.<br><i>Melhor para: tutoriais, reviews, entretenimento.</i>")},
        {tr("Wikipedia"),  "wikipedia",
         tr("<b>Wikipedia</b><br>Enciclopédia livre em português.<br><i>Melhor para: definições, histórico, conceitos.</i>")},
        {tr("DuckDuckGo"), "duckduckgo",
         tr("<b>DuckDuckGo</b><br>Busca privada sem rastreamento.<br><i>Melhor para: privacidade, resultados neutros.</i>")},
    };
    auto* engRow = new QHBoxLayout();
    for (const auto& eng : engines) {
        auto* btn = new QPushButton(eng.label);
        btn->setFixedHeight(28);
        btn->setToolTip(eng.tip);
        const QString k = eng.key;
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
    // ── Helper: página com scroll + grupos de botões ──────────────────────────
    auto makePage = [this](const QList<std::tuple<QString,
                                                   QList<QPair<QString,QString>>,
                                                   QString>>& groups) -> QWidget*
    {
        auto* scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        auto* page   = new QWidget();
        auto* layout = new QVBoxLayout(page);
        layout->setSpacing(8);
        layout->setContentsMargins(6, 6, 6, 6);

        // Mapa de descrições para sites conhecidos
        static const QHash<QString,QString> siteDesc = {
            {"youtube.com",         tr("Maior plataforma de vídeos do mundo")},
            {"tiktok.com",          tr("Vídeos curtos em alta no mundo todo")},
            {"instagram.com",       tr("Fotos, Reels e Stories em destaque")},
            {"twitter.com",         tr("Tendências e debates em tempo real")},
            {"x.com",               tr("Tendências e debates em tempo real")},
            {"reddit.com",          tr("Fóruns e discussões da comunidade")},
            {"github.com",          tr("Repositórios de código mais populares")},
            {"trends.google.com",   tr("O que o mundo está buscando agora")},
            {"trends.google.com.br",tr("O que o Brasil está buscando agora")},
            {"news.google.com",     tr("Agregador de notícias do Google")},
            {"g1.globo.com",        tr("Portal de notícias da Rede Globo")},
            {"uol.com.br",          tr("Um dos maiores portais do Brasil")},
            {"hotmart.com",         tr("Maior plataforma de infoprodutos BR")},
            {"amazon.com",          tr("Maior e-commerce do mundo")},
            {"amazon.com.br",       tr("E-commerce mais acessado do Brasil")},
            {"mercadolivre.com.br", tr("Maior marketplace da América Latina")},
            {"bloomberg.com",       tr("Referência mundial em finanças e mercados")},
            {"coinmarketcap.com",   tr("Preços e rankings de criptomoedas")},
            {"producthunt.com",     tr("Produtos e startups lançados hoje")},
            {"explodingtopics.com", tr("Tendências emergentes antes de explodirem")},
            {"answerthepublic.com", tr("O que as pessoas perguntam sobre qualquer tema")},
            {"clickbank.com",       tr("Maior rede de afiliados digital do mundo")},
            {"publico.pt",          tr("Jornal de referência de Portugal")},
            {"observador.pt",       tr("Notícias e análises de Portugal")},
            {"aljazeera.com",       tr("Canal de notícias internacionais — Médio Oriente")},
            {"bbc.com",             tr("BBC: notícias mundiais de referência")},
            {"reuters.com",         tr("Agência de notícias global de referência")},
            {"ft.com",              tr("Financial Times: economia e finanças globais")},
            {"tradingeconomics.com",tr("Dados económicos de 196 países")},
            {"ourworldindata.org",  tr("Dados e gráficos sobre o mundo em desenvolvimento")},
            {"nationalgeographic.com",tr("Ciência, natureza e exploração geográfica")},
            {"ted.com",             tr("Palestras inspiradoras de especialistas mundiais")},
            {"dribbble.com",        tr("Design gráfico e UI em destaque")},
            {"bilibili.com",        tr("Principal plataforma de vídeo da China")},
            {"webtoons.com",        tr("Manhwas e webtoons coreanos populares")},
        };

        for (const auto& [title, items, color] : groups) {
            // Detecta se algum item tem 🔥 para destacar o grupo
            const bool hasHot = std::any_of(items.begin(), items.end(),
                [](const QPair<QString,QString>& p){ return p.first.startsWith("🔥"); });

            auto* grp = new QGroupBox(title);
            grp->setStyleSheet(
                QString("QGroupBox{font-weight:bold;color:%1;border:%2 solid %1;"
                        "border-radius:6px;margin-top:8px;padding-top:4px}"
                        "QGroupBox::title{subcontrol-origin:margin;left:8px;padding:0 4px}")
                .arg(color, hasHot ? "2px" : "1px"));
            auto* grid = new QGridLayout(grp);
            grid->setSpacing(5);
            int col = 0, row = 0;
            for (const auto& [rawLabel, url] : items) {
                const bool isHot = rawLabel.startsWith("🔥");
                // Remove prefixo "🔥 " do texto exibido
                const QString label = isHot
                    ? rawLabel.mid(rawLabel.indexOf(' ') + 1)
                    : rawLabel;

                // Tooltip rico: nome + domínio + descrição do site
                const QString domain = QUrl(url).host().remove("www.");
                const QString desc   = siteDesc.value(domain,
                    siteDesc.value(QUrl(url).host(),
                    tr("Clique para abrir no navegador selecionado")));
                const QString tip =
                    QString("<b>%1</b><br>"
                            "<span style='color:#a0aec0;font-size:10px'>%2</span><br><br>"
                            "%3<br><br>"
                            "<i style='color:#68d391'>🖱️ Abre no navegador escolhido</i>")
                    .arg(label.toHtmlEscaped(), domain, desc);

                auto* btn = new QPushButton(isHot ? "🔥 " + label : label);
                btn->setFixedHeight(28);
                btn->setToolTip(tip);

                if (isHot) {
                    btn->setStyleSheet(
                        "QPushButton{"
                        "  border: 2px solid #f6ad55;"
                        "  border-radius: 4px;"
                        "  color: #c05621;"
                        "  padding: 0 8px;"
                        "  font-size: 11px;"
                        "  font-weight: bold;"
                        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
                        "    stop:0 #fffaf0, stop:1 #fef3c7);}"
                        "QPushButton:hover{"
                        "  background: #f6ad55;"
                        "  color: white;"
                        "  border-color: #ed8936;}");
                } else {
                    btn->setStyleSheet(
                        QString("QPushButton{border:1px solid %1;border-radius:4px;"
                                "color:%1;padding:0 8px;font-size:11px}"
                                "QPushButton:hover{background:%1;color:white}").arg(color));
                }
                const QString u = url;
                connect(btn, &QPushButton::clicked, this, [this,u](){ openUrl(u); });
                grid->addWidget(btn, row, col);
                if (++col >= 3) { col = 0; ++row; }
            }
            layout->addWidget(grp);
        }
        layout->addStretch();
        scroll->setWidget(page);
        return scroll;
    };

    // ── Sub-abas regionais ────────────────────────────────────────────────────
    auto* regional = new QTabWidget();
    regional->setTabPosition(QTabWidget::North);

    // ════ 🇧🇷 BRASIL ════════════════════════════════════════════════════════
    regional->addTab(makePage({
        {tr("📺 Vídeo & Entretenimento"), {
            {"🔥 YouTube Trending BR",  "https://www.youtube.com/feed/trending?gl=BR"},
            {"YouTube Música BR",      "https://www.youtube.com/feed/trending?gl=BR&bp=4gINGgt5dG1hX2NoYXJ0cw%3D%3D"},
            {"YouTube Gaming BR",      "https://www.youtube.com/feed/trending?gl=BR&bp=4gIcGhpnYW1pbmdfY29ycHVzX21vc3RfcG9wdWxhcg%3D%3D"},
            {"TikTok Trending",        "https://www.tiktok.com/trending"},
            {"Kwai BR",                "https://www.kwai.com/"},
            {"Globoplay Destaques",    "https://globoplay.globo.com/"},
        }, "#green"},
        {tr("📰 Notícias BR"), {
            {"Google News BR",         "https://news.google.com/topstories?hl=pt-BR&gl=BR&ceid=BR:pt-419"},
            {"🔥 Google Trends BR",      "https://trends.google.com.br/trending?geo=BR"},
            {"UOL Notícias",           "https://noticias.uol.com.br/"},
            {"G1 Globo",               "https://g1.globo.com/"},
            {"R7 Notícias",            "https://www.r7.com/"},
            {"Folha de S.Paulo",       "https://www.folha.uol.com.br/"},
        }, "#d69e2e"},
        {tr("🛒 Mercado & Afiliados BR"), {
            {"🔥 Mercado Livre + Vendidos","https://www.mercadolivre.com.br/mais-vendidos/"},
            {"🔥 Hotmart Marketplace",  "https://hotmart.com/pt-BR/marketplace"},
            {"Monetizze Marketplace",  "https://monetizze.com.br/marketplace/"},
            {"Eduzz Marketplace",      "https://eduzz.com/marketplace"},
            {"Amazon BR Best Sellers", "https://www.amazon.com.br/gp/bestsellers/"},
            {"Shopee BR Trending",     "https://shopee.com.br/"},
        }, "#dd6b20"},
        {tr("💬 Redes Sociais BR"), {
            {"Instagram Explore",      "https://www.instagram.com/explore/"},
            {"Twitter/X BR Trending",  "https://twitter.com/explore/tabs/trending"},
            {"Facebook BR",            "https://www.facebook.com/"},
            {"Pinterest BR",           "https://www.pinterest.com.br/"},
            {"LinkedIn BR",            "https://www.linkedin.com/feed/"},
            {"Reddit r/brasil",        "https://www.reddit.com/r/brasil/hot/"},
        }, "#3182ce"},
    }), tr("🇧🇷 Brasil"));

    // ════ 🇵🇹 PORTUGAL ══════════════════════════════════════════════════════
    regional->addTab(makePage({
        {tr("📰 Notícias PT"), {
            {"Google News Portugal",   "https://news.google.com/topstories?hl=pt-PT&gl=PT&ceid=PT:pt"},
            {"Google Trends Portugal", "https://trends.google.pt/trending?geo=PT"},
            {"Público",                "https://www.publico.pt/"},
            {"Observador",             "https://observador.pt/"},
            {"Expresso",               "https://expresso.pt/"},
            {"RTP Notícias",           "https://www.rtp.pt/noticias/"},
        }, "#d69e2e"},
        {tr("📺 Vídeo & Entretenimento PT"), {
            {"YouTube Trending PT",    "https://www.youtube.com/feed/trending?gl=PT"},
            {"RTP Play",               "https://www.rtp.pt/play/"},
            {"SIC Online",             "https://sicnoticias.pt/"},
            {"TVI24",                  "https://tvi24.iol.pt/"},
            {"CMTV",                   "https://www.cmtv.pt/"},
            {"TikTok PT",              "https://www.tiktok.com/trending"},
        }, "#e53e3e"},
        {tr("🛒 Comércio & Afiliados PT"), {
            {"Amazon.es (PT usa muito)","https://www.amazon.es/gp/bestsellers/"},
            {"FNAC Portugal",          "https://www.fnac.pt/"},
            {"Worten Promoções",       "https://www.worten.pt/"},
            {"OLX Portugal",           "https://www.olx.pt/"},
            {"Digistore24 PT",         "https://www.digistore24.com/"},
            {"Continente Online",      "https://www.continente.pt/"},
        }, "#38a169"},
        {tr("💬 Redes & Comunidade PT"), {
            {"Instagram PT",           "https://www.instagram.com/explore/"},
            {"Twitter/X PT",           "https://twitter.com/explore/tabs/trending"},
            {"LinkedIn PT",            "https://www.linkedin.com/feed/"},
            {"Reddit r/portugal",      "https://www.reddit.com/r/portugal/hot/"},
            {"SAPO Notícias",          "https://noticias.sapo.pt/"},
            {"Jornal de Negócios",     "https://www.jornaldenegocios.pt/"},
        }, "#805ad5"},
    }), tr("🇵🇹 Portugal"));

    // ════ 🇺🇸 USA & GLOBAL ══════════════════════════════════════════════════
    regional->addTab(makePage({
        {tr("📰 News USA"), {
            {"Google News USA",        "https://news.google.com/topstories?hl=en-US&gl=US&ceid=US:en"},
            {"Google Trends USA",      "https://trends.google.com/trending?geo=US"},
            {"Reddit r/all Hot",       "https://www.reddit.com/r/all/hot/"},
            {"Hacker News",            "https://news.ycombinator.com/"},
            {"Reuters",                "https://www.reuters.com/"},
            {"AP News",                "https://apnews.com/"},
        }, "#d69e2e"},
        {tr("💻 Tech & Dev"), {
            {"🔥 GitHub Trending",       "https://github.com/trending"},
            {"🔥 Product Hunt",         "https://www.producthunt.com/"},
            {"DEV.to",                 "https://dev.to/"},
            {"The Verge",              "https://www.theverge.com/"},
            {"TechCrunch",             "https://techcrunch.com/"},
            {"Wired",                  "https://www.wired.com/"},
        }, "#38a169"},
        {tr("🛒 Comércio USA"), {
            {"Amazon USA Best Sellers","https://www.amazon.com/gp/bestsellers/"},
            {"eBay Trending",          "https://www.ebay.com/trending"},
            {"Etsy Trending",          "https://www.etsy.com/"},
            {"ClickBank Marketplace",  "https://www.clickbank.com/"},
            {"JVZoo Marketplace",      "https://www.jvzoo.com/"},
            {"WarriorPlus",            "https://warriorplus.com/"},
        }, "#dd6b20"},
        {tr("🎯 Criadores & Tendências"), {
            {"🔥 Exploding Topics",      "https://explodingtopics.com/"},
            {"🔥 AnswerThePublic",      "https://answerthepublic.com/"},
            {"BuzzSumo",               "https://buzzsumo.com/"},
            {"SparkToro Trending",     "https://sparktoro.com/trending"},
            {"Substack Trending",      "https://substack.com/discover"},
            {"Medium Trending",        "https://medium.com/explore"},
        }, "#805ad5"},
        {tr("📺 Entretenimento USA"), {
            {"YouTube Trending USA",   "https://www.youtube.com/feed/trending?gl=US"},
            {"Netflix Top USA",        "https://www.netflix.com/"},
            {"Twitch Trending",        "https://www.twitch.tv/directory?sort=VIEWER_COUNT"},
            {"Spotify Charts USA",     "https://charts.spotify.com/charts/overview/global"},
            {"Billboard Hot 100",      "https://www.billboard.com/charts/hot-100/"},
            {"IMDb Most Popular",      "https://www.imdb.com/chart/moviemeter/"},
        }, "#e53e3e"},
    }), tr("🇺🇸 USA & Global"));

    // ════ 🌏 ÁSIA ════════════════════════════════════════════════════════════
    regional->addTab(makePage({
        {tr("🇯🇵 Japão"), {
            {"Yahoo Japan Trends",     "https://trends.yahoo.co.jp/"},
            {"NicoNico Trending",      "https://www.nicovideo.jp/"},
            {"Rakuten Mais Vendidos",  "https://www.rakuten.co.jp/"},
            {"Amazon Japan Best",      "https://www.amazon.co.jp/gp/bestsellers/"},
            {"LINE TODAY JP",          "https://today.line.me/jp"},
            {"Twitter/X JP Trends",    "https://twitter.com/explore/tabs/trending"},
        }, "#e53e3e"},
        {tr("🇨🇳 China / Mercado Chinês"), {
            {"Bilibili Trending",      "https://www.bilibili.com/"},
            {"Weibo Hot",              "https://weibo.com/hot/"},
            {"Taobao/Tmall Trending",  "https://www.taobao.com/"},
            {"JD.com Best Sellers",    "https://www.jd.com/"},
            {"Alibaba Global",         "https://www.alibaba.com/"},
            {"AliExpress Trending",    "https://www.aliexpress.com/"},
        }, "#dd6b20"},
        {tr("🇰🇷 Coreia do Sul"), {
            {"Naver Trending",         "https://www.naver.com/"},
            {"Kakao Trending",         "https://www.kakao.com/"},
            {"Coupang Best Sellers",   "https://www.coupang.com/"},
            {"Webtoon Popular",        "https://www.webtoons.com/en/dailySchedule"},
            {"YouTube Trending KR",    "https://www.youtube.com/feed/trending?gl=KR"},
            {"Melon Music Chart",      "https://www.melon.com/chart/index.htm"},
        }, "#3182ce"},
        {tr("🇮🇳 India"), {
            {"Google Trends India",    "https://trends.google.co.in/trending?geo=IN"},
            {"Hotstar Trending",       "https://www.hotstar.com/in"},
            {"Flipkart Best Sellers",  "https://www.flipkart.com/"},
            {"Amazon India Best",      "https://www.amazon.in/gp/bestsellers/"},
            {"Times of India",         "https://timesofindia.indiatimes.com/"},
            {"ShareChat Trending",     "https://sharechat.com/"},
        }, "#38a169"},
        {tr("🌏 Sudeste Asiático"), {
            {"Lazada SEA",             "https://www.lazada.com/"},
            {"Shopee SEA",             "https://shopee.com/"},
            {"Grab",                   "https://www.grab.com/"},
            {"Tokopedia (Indonesia)",  "https://www.tokopedia.com/"},
            {"Straits Times (SG)",     "https://www.straitstimes.com/"},
            {"Channel NewsAsia",       "https://www.channelnewsasia.com/"},
        }, "#805ad5"},
    }), tr("🌏 Ásia"));

    // ════ 🌍 EUROPA ══════════════════════════════════════════════════════════
    regional->addTab(makePage({
        {tr("📰 Notícias Europa"), {
            {"Euronews",               "https://www.euronews.com/"},
            {"BBC News Europe",        "https://www.bbc.com/news/world/europe"},
            {"Deutsche Welle (DW)",    "https://www.dw.com/en/top-stories/s-9097"},
            {"Le Monde",               "https://www.lemonde.fr/"},
            {"El País",                "https://elpais.com/"},
            {"Der Spiegel International","https://www.spiegel.de/international/"},
        }, "#3182ce"},
        {tr("📺 Vídeo & Entretenimento EU"), {
            {"YouTube Trending UK",    "https://www.youtube.com/feed/trending?gl=GB"},
            {"YouTube Trending DE",    "https://www.youtube.com/feed/trending?gl=DE"},
            {"YouTube Trending FR",    "https://www.youtube.com/feed/trending?gl=FR"},
            {"ARD Mediathek (DE)",     "https://www.ardmediathek.de/"},
            {"France.tv",              "https://www.france.tv/"},
            {"La Repubblica (IT)",     "https://www.repubblica.it/"},
        }, "#e53e3e"},
        {tr("🛒 Comércio & Mercado EU"), {
            {"Amazon UK Best Sellers", "https://www.amazon.co.uk/gp/bestsellers/"},
            {"Amazon DE Best Sellers", "https://www.amazon.de/gp/bestsellers/"},
            {"Amazon FR Best Sellers", "https://www.amazon.fr/gp/bestsellers/"},
            {"Zalando Trending",       "https://www.zalando.com/"},
            {"Booking.com Trending",   "https://www.booking.com/"},
            {"Idealo.de Preços",       "https://www.idealo.de/"},
        }, "#38a169"},
        {tr("🏛️ Política & Economia EU"), {
            {"Politico Europe",        "https://www.politico.eu/"},
            {"EUobserver",             "https://euobserver.com/"},
            {"Financial Times EU",     "https://www.ft.com/"},
            {"Statista Europe",        "https://www.statista.com/markets/403/topic/467/european-union/"},
            {"Eurostat Dados",         "https://ec.europa.eu/eurostat/"},
            {"Trading Economics EU",   "https://tradingeconomics.com/european-union/indicators"},
        }, "#805ad5"},
    }), tr("🌍 Europa"));

    // ════ 🌎 AMÉRICA LATINA ══════════════════════════════════════════════════
    regional->addTab(makePage({
        {tr("📰 Notícias Am. Latina"), {
            {"Google News LATAM",      "https://news.google.com/topstories?hl=es-419&gl=MX&ceid=MX:es-419"},
            {"Infobae",                "https://www.infobae.com/"},
            {"BBC Mundo",              "https://www.bbc.com/mundo"},
            {"El Universal México",    "https://www.eluniversal.com.mx/"},
            {"Clarín Argentina",       "https://www.clarin.com/"},
            {"El Comercio Peru",       "https://elcomercio.pe/"},
        }, "#d69e2e"},
        {tr("🛒 Comércio LATAM"), {
            {"Mercado Livre MX",       "https://www.mercadolibre.com.mx/"},
            {"Mercado Livre CO",       "https://www.mercadolibre.com.co/"},
            {"Mercado Livre AR",       "https://www.mercadolibre.com.ar/"},
            {"Amazon MX Best Sellers", "https://www.amazon.com.mx/gp/bestsellers/"},
            {"Falabella Tendencias",   "https://www.falabella.com/falabella-cl/"},
            {"Linio Marketplace",      "https://www.linio.com/"},
        }, "#dd6b20"},
        {tr("📺 Vídeo LATAM"), {
            {"YouTube Trending MX",    "https://www.youtube.com/feed/trending?gl=MX"},
            {"YouTube Trending AR",    "https://www.youtube.com/feed/trending?gl=AR"},
            {"YouTube Trending CO",    "https://www.youtube.com/feed/trending?gl=CO"},
            {"TikTok LATAM",           "https://www.tiktok.com/trending"},
            {"Netflix LATAM",          "https://www.netflix.com/mx/"},
            {"Globo Internacional",    "https://www.globo.com/"},
        }, "#e53e3e"},
        {tr("💬 Redes Sociais LATAM"), {
            {"Twitter/X MX Trending",  "https://twitter.com/explore/tabs/trending"},
            {"Instagram LATAM",        "https://www.instagram.com/explore/"},
            {"Reddit r/argentina",     "https://www.reddit.com/r/argentina/hot/"},
            {"Reddit r/mexico",        "https://www.reddit.com/r/mexico/hot/"},
            {"Reddit r/colombia",      "https://www.reddit.com/r/colombia/hot/"},
            {"Reddit r/chile",         "https://www.reddit.com/r/chile/hot/"},
        }, "#3182ce"},
    }), tr("🌎 América Latina"));

    // ════ 🌍 ÁFRICA ══════════════════════════════════════════════════════════
    regional->addTab(makePage({
        {tr("📰 Notícias África"), {
            {"Al Jazeera Africa",      "https://www.aljazeera.com/africa/"},
            {"BBC Africa",             "https://www.bbc.com/news/world/africa"},
            {"Africa News",            "https://www.africanews.com/"},
            {"The Africa Report",      "https://www.theafricareport.com/"},
            {"Daily Nation Kenya",     "https://nation.africa/kenya"},
            {"Mail & Guardian (ZA)",   "https://mg.co.za/"},
        }, "#d69e2e"},
        {tr("🇿🇦 África do Sul"), {
            {"News24",                 "https://www.news24.com/"},
            {"TimesLive ZA",           "https://www.timeslive.co.za/"},
            {"Google Trends ZA",       "https://trends.google.co.za/trending?geo=ZA"},
            {"Takealot Best Sellers",  "https://www.takealot.com/"},
            {"YouTube Trending ZA",    "https://www.youtube.com/feed/trending?gl=ZA"},
            {"OLX South Africa",       "https://www.olx.co.za/"},
        }, "#38a169"},
        {tr("🇳🇬 Nigéria"), {
            {"Vanguard Nigeria",       "https://www.vanguardngr.com/"},
            {"Punch Nigeria",          "https://punchng.com/"},
            {"Google Trends NG",       "https://trends.google.com.ng/trending?geo=NG"},
            {"Jumia Nigeria",          "https://www.jumia.com.ng/"},
            {"Konga Nigeria",          "https://www.konga.com/"},
            {"YouTube Trending NG",    "https://www.youtube.com/feed/trending?gl=NG"},
        }, "#e53e3e"},
        {tr("🇪🇬 Egito & Norte de África"), {
            {"Al-Ahram Arabic",        "https://www.ahram.org.eg/"},
            {"Al Jazeera Arabic",      "https://www.aljazeera.net/"},
            {"Google Trends EG",       "https://trends.google.com/trending?geo=EG"},
            {"OLX Egypt",              "https://www.olx.com.eg/"},
            {"Souq/Amazon.ae",         "https://www.amazon.ae/gp/bestsellers/"},
            {"YouTube Trending EG",    "https://www.youtube.com/feed/trending?gl=EG"},
        }, "#805ad5"},
        {tr("💰 Fintech & Mobile África"), {
            {"M-Pesa (Safaricom)",     "https://www.safaricom.co.ke/personal/m-pesa"},
            {"Flutterwave",            "https://flutterwave.com/"},
            {"Paystack",               "https://paystack.com/"},
            {"Jumia Marketplace",      "https://www.jumia.com/"},
            {"AfricanStartups",        "https://africanstartups.com/"},
            {"Disrupt Africa",         "https://disruptafrica.com/"},
        }, "#3182ce"},
    }), tr("🌍 África"));

    // ════ 🌊 OCEANIA ════════════════════════════════════════════════════════
    regional->addTab(makePage({
        {tr("📰 Notícias Oceania"), {
            {"ABC Australia News",     "https://www.abc.net.au/news/"},
            {"The Guardian Australia", "https://www.theguardian.com/australia-news"},
            {"NZ Herald",              "https://www.nzherald.co.nz/"},
            {"Stuff.co.nz",            "https://www.stuff.co.nz/"},
            {"Sydney Morning Herald",  "https://www.smh.com.au/"},
            {"Google News AU",         "https://news.google.com/topstories?hl=en-AU&gl=AU&ceid=AU:en"},
        }, "#3182ce"},
        {tr("🛒 Comércio Oceania"), {
            {"Amazon AU Best Sellers", "https://www.amazon.com.au/gp/bestsellers/"},
            {"eBay Australia",         "https://www.ebay.com.au/"},
            {"TradeMe NZ",             "https://www.trademe.co.nz/"},
            {"Catch.com.au",           "https://www.catch.com.au/"},
            {"Kmart Australia",        "https://www.kmart.com.au/"},
            {"JB Hi-Fi Trending",      "https://www.jbhifi.com.au/"},
        }, "#38a169"},
        {tr("📺 Vídeo & Cultura AU"), {
            {"YouTube Trending AU",    "https://www.youtube.com/feed/trending?gl=AU"},
            {"9Now Australia",         "https://www.9now.com.au/"},
            {"Stan Streaming",         "https://www.stan.com.au/"},
            {"ABC iview",              "https://iview.abc.net.au/"},
            {"Google Trends AU",       "https://trends.google.com.au/trending?geo=AU"},
            {"Reddit r/australia",     "https://www.reddit.com/r/australia/hot/"},
        }, "#d69e2e"},
    }), tr("🌊 Oceania"));

    // ════ 💰 FINANÇAS & MERCADOS GLOBAIS ════════════════════════════════════
    regional->addTab(makePage({
        {tr("📈 Mercados Financeiros"), {
            {"🔥 Bloomberg Markets",    "https://www.bloomberg.com/markets"},
            {"Reuters Finance",        "https://www.reuters.com/finance/"},
            {"Yahoo Finance",          "https://finance.yahoo.com/"},
            {"Investing.com",          "https://www.investing.com/"},
            {"MarketWatch",            "https://www.marketwatch.com/"},
            {"Financial Times",        "https://www.ft.com/"},
        }, "#38a169"},
        {tr("₿ Criptomoedas"), {
            {"🔥 CoinMarketCap",         "https://coinmarketcap.com/"},
            {"CoinGecko",              "https://www.coingecko.com/"},
            {"CryptoCompare",          "https://www.cryptocompare.com/"},
            {"Binance Markets",        "https://www.binance.com/en/markets/overview"},
            {"TradingView Crypto",     "https://www.tradingview.com/markets/cryptocurrencies/prices-all/"},
            {"Messari Crypto",         "https://messari.io/"},
        }, "#d69e2e"},
        {tr("🏦 Economia Global"), {
            {"Trading Economics",      "https://tradingeconomics.com/"},
            {"World Bank Data",        "https://data.worldbank.org/"},
            {"IMF Data",               "https://www.imf.org/en/Data"},
            {"OECD Data",              "https://data.oecd.org/"},
            {"Statista",               "https://www.statista.com/"},
            {"Our World in Data",      "https://ourworldindata.org/"},
        }, "#3182ce"},
        {tr("📊 Bolsas de Valores"), {
            {"NYSE Markets",           "https://www.nyse.com/markets"},
            {"NASDAQ Trends",          "https://www.nasdaq.com/"},
            {"B3 (Bovespa)",           "https://www.b3.com.br/pt_br/market-data-e-indices/"},
            {"Euronext Markets",       "https://www.euronext.com/en/markets"},
            {"Tokyo Stock Exchange",   "https://www.jpx.co.jp/english/"},
            {"Macrotrends",            "https://www.macrotrends.net/"},
        }, "#805ad5"},
        {tr("💹 Forex & Commodities"), {
            {"XE Currency Converter",  "https://www.xe.com/"},
            {"Forex Factory",          "https://www.forexfactory.com/"},
            {"Gold Price Live",        "https://www.goldprice.org/"},
            {"Oil Price Monitor",      "https://oilprice.com/"},
            {"FRED Economic Data",     "https://fred.stlouisfed.org/"},
            {"Numbeo Cost of Living",  "https://www.numbeo.com/cost-of-living/"},
        }, "#dd6b20"},
    }), tr("💰 Finanças"));

    // ════ 🌐 POLÍTICA & GEOPOLÍTICA GLOBAL ══════════════════════════════════
    regional->addTab(makePage({
        {tr("🌐 Geopolítica & Relações Internacionais"), {
            {"Council on Foreign Relations","https://www.cfr.org/"},
            {"Foreign Policy",         "https://foreignpolicy.com/"},
            {"The Diplomat (Ásia)",    "https://thediplomat.com/"},
            {"Al Jazeera World",       "https://www.aljazeera.com/"},
            {"Reuters World News",     "https://www.reuters.com/world/"},
            {"Associated Press Global","https://apnews.com/hub/world-news"},
        }, "#e53e3e"},
        {tr("🏛️ Política USA"), {
            {"Politico USA",           "https://www.politico.com/"},
            {"The Hill",               "https://thehill.com/"},
            {"RealClearPolitics",      "https://www.realclearpolitics.com/"},
            {"FiveThirtyEight",        "https://fivethirtyeight.com/"},
            {"Congress.gov",           "https://www.congress.gov/"},
            {"NPR Politics",           "https://www.npr.org/sections/politics/"},
        }, "#3182ce"},
        {tr("🇧🇷🇵🇹 Política Lusófona"), {
            {"Agência Brasil",         "https://agenciabrasil.ebc.com.br/"},
            {"Câmara dos Deputados",   "https://www.camara.leg.br/"},
            {"Senado Federal BR",      "https://www.senado.leg.br/"},
            {"Assembleia Portugal",    "https://www.parlamento.pt/"},
            {"Observador PT Política", "https://observador.pt/politica/"},
            {"Jornal Público BR",      "https://www.publico.pt/"},
        }, "#38a169"},
        {tr("📊 Think Tanks & Análises"), {
            {"Chatham House",          "https://www.chathamhouse.org/"},
            {"Brookings Institution",  "https://www.brookings.edu/"},
            {"CSIS (Security)",        "https://www.csis.org/"},
            {"World Economic Forum",   "https://www.weforum.org/"},
            {"Transparency International","https://www.transparency.org/"},
            {"Freedom House",          "https://freedomhouse.org/"},
        }, "#805ad5"},
    }), tr("🌐 Política"));

    // ════ 🌏 CULTURA, CIÊNCIA & CONHECIMENTO ════════════════════════════════
    regional->addTab(makePage({
        {tr("🔬 Ciência & Tecnologia"), {
            {"Nature News",            "https://www.nature.com/news"},
            {"Science Daily",          "https://www.sciencedaily.com/"},
            {"MIT Technology Review",  "https://www.technologyreview.com/"},
            {"New Scientist",          "https://www.newscientist.com/"},
            {"ArXiv Preprints",        "https://arxiv.org/"},
            {"Google Scholar",         "https://scholar.google.com/"},
        }, "#38a169"},
        {tr("🌍 Cultura & Sociedade"), {
            {"National Geographic",    "https://www.nationalgeographic.com/"},
            {"Culture Trip",           "https://theculturetrip.com/"},
            {"Smithsonian Magazine",   "https://www.smithsonianmag.com/"},
            {"Wikipedia PT",           "https://pt.wikipedia.org/wiki/Especial:PáginaAleatória"},
            {"Wikipedia EN",           "https://en.wikipedia.org/wiki/Special:Random"},
            {"Wikipedia ES",           "https://es.wikipedia.org/wiki/Especial:Aleatoria"},
        }, "#d69e2e"},
        {tr("🗺️ Geografia & Mapas"), {
            {"Google Maps Explore",    "https://www.google.com/maps/"},
            {"OpenStreetMap",          "https://www.openstreetmap.org/"},
            {"Geograph World",         "https://www.geograph.org.uk/"},
            {"Atlas Obscura",          "https://www.atlasobscura.com/"},
            {"Worldometers",           "https://www.worldometers.info/"},
            {"GaWC World Cities",      "https://www.lboro.ac.uk/microsites/geography/gawc/"},
        }, "#3182ce"},
        {tr("📚 Aprendizado Global"), {
            {"Khan Academy",           "https://www.khanacademy.org/"},
            {"TED Talks",              "https://www.ted.com/talks"},
            {"Coursera Popular",       "https://www.coursera.org/"},
            {"edX Courses",            "https://www.edx.org/"},
            {"Project Gutenberg",      "https://www.gutenberg.org/"},
            {"Internet Archive",       "https://archive.org/"},
        }, "#805ad5"},
    }), tr("🌏 Cultura & Ciência"));

    // ════ 💰 PRODUTOS DIGITAIS & AFILIADOS ══════════════════════════════════
    regional->addTab(makePage({
        {tr("📦 Plataformas BR/PT"), {
            {"Hotmart Marketplace",    "https://hotmart.com/pt-BR/marketplace"},
            {"Monetizze Marketplace",  "https://monetizze.com.br/marketplace/"},
            {"Eduzz Marketplace",      "https://eduzz.com/marketplace"},
            {"Braip Afiliados",        "https://braip.com/"},
            {"Digistore24",            "https://www.digistore24.com/"},
            {"PerfectPay",             "https://perfectpay.com.br/"},
        }, "#dd6b20"},
        {tr("📦 Plataformas USA/Global"), {
            {"ClickBank Marketplace",  "https://www.clickbank.com/"},
            {"JVZoo Marketplace",      "https://www.jvzoo.com/"},
            {"WarriorPlus",            "https://warriorplus.com/"},
            {"ShareASale",             "https://www.shareasale.com/"},
            {"Commission Junction",    "https://www.cj.com/"},
            {"Impact Radius",          "https://impact.com/"},
        }, "#38a169"},
        {tr("🎓 Cursos & Conteúdo"), {
            {"Udemy Best Sellers",     "https://www.udemy.com/courses/search/?sort=most_reviewed"},
            {"Coursera Popular",       "https://www.coursera.org/"},
            {"Skillshare Trending",    "https://www.skillshare.com/en/browse"},
            {"Teachable Marketplace",  "https://marketplace.teachable.com/"},
            {"Gumroad Trending",       "https://discover.gumroad.com/"},
            {"Patreon Explore",        "https://www.patreon.com/explore"},
        }, "#805ad5"},
        {tr("🛍️ Criação de Produtos"), {
            {"Canva Templates",        "https://www.canva.com/templates/"},
            {"Creative Market",        "https://creativemarket.com/"},
            {"Etsy Digital Downloads", "https://www.etsy.com/market/digital_downloads"},
            {"Ko-fi Creators",         "https://ko-fi.com/explore"},
            {"Notion Templates",       "https://www.notion.so/templates"},
            {"Fiverr Gig Trends",      "https://www.fiverr.com/gigs"},
        }, "#3182ce"},
    }), tr("💰 Produtos Digitais"));

    // ════ 🎯 CRIADORES DE CONTEÚDO ══════════════════════════════════════════
    regional->addTab(makePage({
        {tr("🔍 Pesquisa de Mercado"), {
            {"AnswerThePublic",        "https://answerthepublic.com/"},
            {"Exploding Topics",       "https://explodingtopics.com/"},
            {"Google Trends Global",   "https://trends.google.com/trends/explore"},
            {"SparkToro",              "https://sparktoro.com/trending"},
            {"Semrush Free",           "https://www.semrush.com/analytics/keywordoverview/"},
            {"Ubersuggest Free",       "https://neilpatel.com/ubersuggest/"},
        }, "#38a169"},
        {tr("✍️ Escrita & Publicação"), {
            {"Substack Discover",      "https://substack.com/discover"},
            {"Medium Trending",        "https://medium.com/explore"},
            {"Notion Templates",       "https://www.notion.so/templates"},
            {"Ghost Themes",           "https://ghost.org/themes/"},
            {"Hashnode Trending",      "https://hashnode.com/"},
            {"Write.as",               "https://write.as/discover"},
        }, "#805ad5"},
        {tr("🎨 Design & Visual"), {
            {"Dribbble Trending",      "https://dribbble.com/shots/popular"},
            {"Behance Trending",       "https://www.behance.net/"},
            {"Pinterest Trends",       "https://www.pinterest.com/ideas/"},
            {"Unsplash Editorial",     "https://unsplash.com/editorial"},
            {"Pexels Trending",        "https://www.pexels.com/"},
            {"Freepik Trending",       "https://www.freepik.com/popular"},
        }, "#e53e3e"},
        {tr("📊 Analytics & SEO"), {
            {"Google Search Console",  "https://search.google.com/search-console/"},
            {"Google Analytics",       "https://analytics.google.com/"},
            {"Ahrefs Free Tools",      "https://ahrefs.com/free-seo-tools"},
            {"Moz Free Tools",         "https://moz.com/free-seo-tools"},
            {"SimilarWeb",             "https://www.similarweb.com/"},
            {"SocialBlade",            "https://socialblade.com/"},
        }, "#d69e2e"},
    }), tr("🎯 Criadores"));

    // Adiciona a aba regional como a 4ª aba principal
    auto* wrapper = new QWidget();
    auto* wl = new QVBoxLayout(wrapper);
    wl->setContentsMargins(0, 4, 0, 0);
    wl->addWidget(regional);
    tabs->addTab(wrapper, QIcon::fromTheme("go-up"), tr("📈 Em Alta"));
}
