#include "AppearanceDialog.h"
#include "ThemeManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QColorDialog>
#include <QFrame>
#include <QFont>

// ─── Helpers ─────────────────────────────────────────────────────────────────
static QString swatchStyle(const QColor& c) {
    // Escolhe texto preto ou branco conforme luminosidade
    const int luma = c.red()*299 + c.green()*587 + c.blue()*114;
    const QString fg = (luma > 128000) ? "#000" : "#fff";
    return QStringLiteral(
        "QPushButton { background:%1; color:%2; border:2px solid #555;"
        " border-radius:5px; min-width:88px; min-height:26px; font-weight:bold; }"
        "QPushButton:hover { border-color:#aaa; }")
        .arg(c.name(), fg);
}

static QString weightToQss(int w) {
    // converte QFont::Weight para string CSS font-weight
    if (w >= 87) return "900";  // Black
    if (w >= 75) return "700";  // Bold
    if (w >= 63) return "600";  // DemiBold
    if (w >= 57) return "500";  // Medium
    return "400";               // Normal
}

// ─── QFont::Weight por índice de combobox ─────────────────────────────────
static const QList<int> kWeights = {
    QFont::Normal, QFont::Medium, QFont::DemiBold, QFont::Bold, QFont::Black
};
static const QStringList kWeightLabels = {
    "Normal (400)", "Médio (500)", "Seminegrito (600)", "Negrito (700)", "Extranegrito (900)"
};

// ─── Construtor ──────────────────────────────────────────────────────────────
AppearanceDialog::AppearanceDialog(QApplication* app, QWidget* parent)
    : QDialog(parent), m_app(app)
{
    setWindowTitle(tr("🎨 Personalizar Aparência"));
    setMinimumWidth(480);
    loadSaved();
    buildUI();
}

void AppearanceDialog::loadSaved()
{
    QSettings cfg;
    if (cfg.value("cfdmnote/theme").toString() != "custom") return;
    m_bg      = QColor(cfg.value("cfdmnote/custom/bg",      m_bg.name()).toString());
    m_toolbar = QColor(cfg.value("cfdmnote/custom/toolbar",  m_toolbar.name()).toString());
    m_text    = QColor(cfg.value("cfdmnote/custom/text",     m_text.name()).toString());
    m_accent  = QColor(cfg.value("cfdmnote/custom/accent",   m_accent.name()).toString());
}

void AppearanceDialog::buildUI()
{
    auto* root = new QVBoxLayout(this);
    root->setSpacing(8);

    auto* title = new QLabel(tr("<b>Personalizar Aparência do CfdmNote</b>"));
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);

    auto* tabs = new QTabWidget();

    // ════════════════════════════════════════════════════════════════════════
    // ABA 1 — CORES
    // ════════════════════════════════════════════════════════════════════════
    auto* tabCores = new QWidget();
    auto* coresLay = new QVBoxLayout(tabCores);

    auto* grpCores = new QGroupBox(tr("Paleta de Cores"));
    auto* grd = new QFormLayout(grpCores);
    grd->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    auto makeBtn = [&](QColor& col, const char* slot) -> QPushButton* {
        auto* b = new QPushButton();
        b->setStyleSheet(swatchStyle(col));
        connect(b, SIGNAL(clicked()), this, slot);
        return b;
    };
    m_btnBg      = makeBtn(m_bg,      SLOT(onPickBg()));
    m_btnToolbar = makeBtn(m_toolbar, SLOT(onPickToolbar()));
    m_btnText    = makeBtn(m_text,    SLOT(onPickText()));
    m_btnAccent  = makeBtn(m_accent,  SLOT(onPickAccent()));

    grd->addRow(tr("Fundo principal:"),   m_btnBg);
    grd->addRow(tr("Toolbar / barra:"),   m_btnToolbar);
    grd->addRow(tr("Texto:"),             m_btnText);
    grd->addRow(tr("Destaque / links:"),  m_btnAccent);
    coresLay->addWidget(grpCores);

    auto* grpFonte = new QGroupBox(tr("Fonte Base"));
    auto* fontLay = new QHBoxLayout(grpFonte);
    fontLay->addWidget(new QLabel(tr("Família:")));
    m_fontCombo = new QFontComboBox();
    QSettings cfg;
    m_fontCombo->setCurrentFont(
        QFont(cfg.value("cfdmnote/custom/fontFamily", "Segoe UI").toString()));
    fontLay->addWidget(m_fontCombo, 1);
    fontLay->addWidget(new QLabel(tr("Tamanho base:")));
    m_fontSize = new QSpinBox();
    m_fontSize->setRange(8, 20); m_fontSize->setSuffix("px");
    m_fontSize->setValue(cfg.value("cfdmnote/custom/fontSize", 12).toInt());
    fontLay->addWidget(m_fontSize);
    coresLay->addWidget(grpFonte);

    // Preview
    auto* grpPrev = new QGroupBox(tr("Pré-visualização"));
    auto* pvLay = new QVBoxLayout(grpPrev);
    m_preview = new QLabel();
    m_preview->setTextFormat(Qt::RichText);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setMinimumHeight(52);
    m_preview->setFrameShape(QFrame::StyledPanel);
    pvLay->addWidget(m_preview);
    coresLay->addWidget(grpPrev);
    coresLay->addStretch();
    updatePreviewLabel();

    tabs->addTab(tabCores, tr("🎨 Cores"));

    // ════════════════════════════════════════════════════════════════════════
    // ABA 2 — TEXTO E TIPOGRAFIA
    // ════════════════════════════════════════════════════════════════════════
    auto* tabTexto = new QWidget();
    auto* textoLay = new QVBoxLayout(tabTexto);

    // ── Tamanhos por área ─────────────────────────────────────────────────
    auto* grpSize = new QGroupBox(tr("Tamanho de Fonte por Área"));
    auto* fmSize = new QFormLayout(grpSize);

    auto makeSpin = [&](const QString& key, int def) -> QSpinBox* {
        auto* s = new QSpinBox(); s->setRange(7, 22); s->setSuffix("px");
        s->setValue(cfg.value(key, def).toInt());
        return s;
    };
    m_menuSize   = makeSpin("cfdmnote/custom/menuSize",   12);
    m_btnSize    = makeSpin("cfdmnote/custom/btnSize",    12);
    m_dialogSize = makeSpin("cfdmnote/custom/dialogSize", 12);
    m_listSize   = makeSpin("cfdmnote/custom/listSize",   12);

    fmSize->addRow(tr("Itens de menu:"),           m_menuSize);
    fmSize->addRow(tr("Botões:"),                   m_btnSize);
    fmSize->addRow(tr("Texto em diálogos:"),        m_dialogSize);
    fmSize->addRow(tr("Lista de entradas / árvore:"), m_listSize);
    textoLay->addWidget(grpSize);

    // ── Peso (negrito) ────────────────────────────────────────────────────
    auto* grpWeight = new QGroupBox(tr("Espessura do Texto (Negrito)"));
    auto* fmWeight = new QFormLayout(grpWeight);

    auto makeWeightCombo = [&](const QString& key, int defaultIdx) -> QComboBox* {
        auto* c = new QComboBox();
        for (const auto& lbl : kWeightLabels) c->addItem(lbl);
        c->setCurrentIndex(cfg.value(key, defaultIdx).toInt());
        return c;
    };
    m_btnWeight    = makeWeightCombo("cfdmnote/custom/btnWeightIdx",    3); // Negrito
    m_menuWeight   = makeWeightCombo("cfdmnote/custom/menuWeightIdx",   0); // Normal
    m_dlgTitleBold = makeWeightCombo("cfdmnote/custom/dlgTitleIdx",     3); // Negrito

    fmWeight->addRow(tr("Texto dos botões:"),      m_btnWeight);
    fmWeight->addRow(tr("Texto dos menus:"),        m_menuWeight);
    fmWeight->addRow(tr("Títulos de diálogos:"),    m_dlgTitleBold);
    textoLay->addWidget(grpWeight);

    // ── Dica rápida ───────────────────────────────────────────────────────
    auto* hint = new QLabel(tr(
        "<small>💡 <b>Dica:</b> use <i>Prévia</i> para ver o efeito antes de salvar.<br>"
        "Tamanho base (aba Cores) é usado onde não há regra específica.</small>"));
    hint->setWordWrap(true);
    hint->setMargin(6);
    textoLay->addWidget(hint);
    textoLay->addStretch();

    tabs->addTab(tabTexto, tr("✏ Texto"));
    root->addWidget(tabs);

    // ── Botões do rodapé ──────────────────────────────────────────────────
    auto* btns = new QHBoxLayout();

    auto* btnPrev = new QPushButton(tr("👁 Prévia"));
    connect(btnPrev, &QPushButton::clicked, this, &AppearanceDialog::onPreview);
    btns->addWidget(btnPrev);

    auto* btnReset = new QPushButton(tr("↺ Restaurar Padrão"));
    connect(btnReset, &QPushButton::clicked, this, &AppearanceDialog::onReset);
    btns->addWidget(btnReset);

    btns->addStretch();

    auto* btnApply = new QPushButton(tr("✔ Aplicar e Salvar"));
    btnApply->setDefault(true);
    connect(btnApply, &QPushButton::clicked, this, &AppearanceDialog::onApply);
    btns->addWidget(btnApply);

    auto* btnClose = new QPushButton(tr("Fechar"));
    connect(btnClose, &QPushButton::clicked, this, &QDialog::reject);
    btns->addWidget(btnClose);

    root->addLayout(btns);
}

void AppearanceDialog::updatePreviewLabel()
{
    if (!m_preview) return;
    m_preview->setStyleSheet(QStringLiteral(
        "QLabel { background:%1; color:%2; padding:10px; border-radius:4px;"
        " font-family:'%3'; font-size:%4px; }")
        .arg(m_bg.name(), m_text.name(),
             m_fontCombo ? m_fontCombo->currentFont().family() : "sans-serif",
             m_fontSize  ? QString::number(m_fontSize->value()) : "12"));
    m_preview->setText(QStringLiteral(
        "Texto normal &nbsp;"
        "<span style='color:%1'>■ Destaque</span> &nbsp;"
        "<span style='background:%1;color:white;padding:2px 8px;"
        "border-radius:3px;font-weight:bold'>Botão</span> &nbsp;"
        "<span style='font-weight:bold'>Negrito</span>")
        .arg(m_accent.name()));
}

// ─── Color pickers ───────────────────────────────────────────────────────────
void AppearanceDialog::onPickBg() {
    QColor c = QColorDialog::getColor(m_bg, this, tr("Cor de fundo"));
    if (!c.isValid()) return;
    m_bg = c; m_btnBg->setStyleSheet(swatchStyle(c)); updatePreviewLabel();
}
void AppearanceDialog::onPickToolbar() {
    QColor c = QColorDialog::getColor(m_toolbar, this, tr("Cor da toolbar"));
    if (!c.isValid()) return;
    m_toolbar = c; m_btnToolbar->setStyleSheet(swatchStyle(c));
}
void AppearanceDialog::onPickText() {
    QColor c = QColorDialog::getColor(m_text, this, tr("Cor do texto"));
    if (!c.isValid()) return;
    m_text = c; m_btnText->setStyleSheet(swatchStyle(c)); updatePreviewLabel();
}
void AppearanceDialog::onPickAccent() {
    QColor c = QColorDialog::getColor(m_accent, this, tr("Cor de destaque"));
    if (!c.isValid()) return;
    m_accent = c; m_btnAccent->setStyleSheet(swatchStyle(c)); updatePreviewLabel();
}

// ─── Prévia (aplica sem salvar) ───────────────────────────────────────────────
void AppearanceDialog::onPreview()
{
    m_app->setStyleSheet(buildQss(
        m_bg.name(), m_toolbar.name(), m_text.name(), m_accent.name(),
        m_fontCombo->currentFont().family(), m_fontSize->value(),
        m_menuSize->value(), m_btnSize->value(),
        m_dialogSize->value(), m_listSize->value(),
        kWeights.value(m_btnWeight->currentIndex(),  QFont::Bold),
        kWeights.value(m_menuWeight->currentIndex(), QFont::Normal),
        m_dlgTitleBold->currentIndex() >= 3));
    updatePreviewLabel();
}

// ─── Aplicar e salvar ─────────────────────────────────────────────────────────
void AppearanceDialog::onApply()
{
    const QString ff   = m_fontCombo->currentFont().family();
    const int     fsz  = m_fontSize->value();
    const int     mSz  = m_menuSize->value();
    const int     bSz  = m_btnSize->value();
    const int     dSz  = m_dialogSize->value();
    const int     lSz  = m_listSize->value();
    const int     bW   = kWeights.value(m_btnWeight->currentIndex(),  QFont::Bold);
    const int     mW   = kWeights.value(m_menuWeight->currentIndex(), QFont::Normal);
    const bool    dtB  = m_dlgTitleBold->currentIndex() >= 3;

    m_app->setStyleSheet(buildQss(
        m_bg.name(), m_toolbar.name(), m_text.name(), m_accent.name(),
        ff, fsz, mSz, bSz, dSz, lSz, bW, mW, dtB));

    QSettings cfg;
    cfg.setValue("cfdmnote/theme",                "custom");
    cfg.setValue("cfdmnote/custom/bg",            m_bg.name());
    cfg.setValue("cfdmnote/custom/toolbar",        m_toolbar.name());
    cfg.setValue("cfdmnote/custom/text",           m_text.name());
    cfg.setValue("cfdmnote/custom/accent",         m_accent.name());
    cfg.setValue("cfdmnote/custom/fontFamily",     ff);
    cfg.setValue("cfdmnote/custom/fontSize",       fsz);
    cfg.setValue("cfdmnote/custom/menuSize",       mSz);
    cfg.setValue("cfdmnote/custom/btnSize",        bSz);
    cfg.setValue("cfdmnote/custom/dialogSize",     dSz);
    cfg.setValue("cfdmnote/custom/listSize",       lSz);
    cfg.setValue("cfdmnote/custom/btnWeightIdx",   m_btnWeight->currentIndex());
    cfg.setValue("cfdmnote/custom/menuWeightIdx",  m_menuWeight->currentIndex());
    cfg.setValue("cfdmnote/custom/dlgTitleIdx",    m_dlgTitleBold->currentIndex());
    accept();
}

// ─── Restaurar padrão ────────────────────────────────────────────────────────
void AppearanceDialog::onReset()
{
    QSettings cfg;
    cfg.setValue("cfdmnote/theme", "navy");
    cfg.remove("cfdmnote/custom");
    m_bg      = QColor("#1e3a5f");
    m_toolbar = QColor("#4a7fb5");
    m_text    = QColor("#f0f4f8");
    m_accent  = QColor("#60a5fa");
    m_btnBg->setStyleSheet(swatchStyle(m_bg));
    m_btnToolbar->setStyleSheet(swatchStyle(m_toolbar));
    m_btnText->setStyleSheet(swatchStyle(m_text));
    m_btnAccent->setStyleSheet(swatchStyle(m_accent));
    m_fontCombo->setCurrentFont(QFont("Segoe UI"));
    m_fontSize->setValue(12);
    m_menuSize->setValue(12);   m_btnSize->setValue(12);
    m_dialogSize->setValue(12); m_listSize->setValue(12);
    m_btnWeight->setCurrentIndex(3);
    m_menuWeight->setCurrentIndex(0);
    m_dlgTitleBold->setCurrentIndex(3);
    updatePreviewLabel();
    emit resetRequested();
}

// ─── Static: aplica tema salvo ────────────────────────────────────────────────
void AppearanceDialog::applySaved(QApplication* app)
{
    QSettings cfg;
    if (cfg.value("cfdmnote/theme").toString() != "custom") return;

    const QString bg     = cfg.value("cfdmnote/custom/bg",         "#1e3a5f").toString();
    const QString tb     = cfg.value("cfdmnote/custom/toolbar",     "#4a7fb5").toString();
    const QString tx     = cfg.value("cfdmnote/custom/text",        "#f0f4f8").toString();
    const QString ac     = cfg.value("cfdmnote/custom/accent",      "#60a5fa").toString();
    const QString ff     = cfg.value("cfdmnote/custom/fontFamily",  "Segoe UI").toString();
    const int     fsz    = cfg.value("cfdmnote/custom/fontSize",    12).toInt();
    const int     mSz    = cfg.value("cfdmnote/custom/menuSize",    12).toInt();
    const int     bSz    = cfg.value("cfdmnote/custom/btnSize",     12).toInt();
    const int     dSz    = cfg.value("cfdmnote/custom/dialogSize",  12).toInt();
    const int     lSz    = cfg.value("cfdmnote/custom/listSize",    12).toInt();
    const int     bWIdx  = cfg.value("cfdmnote/custom/btnWeightIdx",   3).toInt();
    const int     mWIdx  = cfg.value("cfdmnote/custom/menuWeightIdx",  0).toInt();
    const bool    dtB    = cfg.value("cfdmnote/custom/dlgTitleIdx",    3).toInt() >= 3;

    app->setStyleSheet(buildQss(bg, tb, tx, ac, ff, fsz, mSz, bSz, dSz, lSz,
        kWeights.value(bWIdx, QFont::Bold),
        kWeights.value(mWIdx, QFont::Normal), dtB));
}

// ─── Static: gera QSS completo ───────────────────────────────────────────────
QString AppearanceDialog::buildQss(
    const QString& bg, const QString& toolbarBg,
    const QString& text, const QString& accent,
    const QString& fontFamily, int fontSize,
    int menuFontSize, int btnFontSize,
    int dialogFontSize, int listFontSize,
    int btnWeight, int menuWeight,
    bool boldDialogTitle)
{
    const QColor bgC(bg), acC(accent), txC(text);
    const QString bgLight   = bgC.lighter(130).name();
    const QString bgDark    = bgC.darker(115).name();
    const QString acDark    = acC.darker(120).name();
    const QString acLight   = acC.lighter(150).name();
    const QString border    = bgC.lighter(160).name();
    const QString inputBg   = bgC.lighter(115).name();
    const QString btnWStr   = weightToQss(btnWeight);
    const QString menuWStr  = weightToQss(menuWeight);
    const QString dlgTitleW = boldDialogTitle ? "bold" : "normal";

    return QStringLiteral(R"(
/* ── Base ────────────────────────────────────────────────────────────────── */
* { font-family: '%1'; font-size: %2px; color: %3; }

QMainWindow, QDialog, QWidget#centralWidget, QStackedWidget, QSplitter,
QFrame[frameShape="0"], QFrame[frameShape="1"]
{ background: %4; }

/* ── Menubar ─────────────────────────────────────────────────────────────── */
QMenuBar { background: %5; color: %3; border-bottom: 2px solid %6; padding: 1px 4px; }
QMenuBar::item { background: transparent; padding: 5px 12px; border-radius: 4px; }
QMenuBar::item:selected, QMenuBar::item:pressed { background: %6; color: white; }

/* ── Menu ────────────────────────────────────────────────────────────────── */
QMenu { background: %7; color: %3; border: 1px solid %6; border-radius: 6px; padding: 4px 0; }
QMenu::item { padding: 6px 28px 6px 12px; margin: 1px 4px; border-radius: 3px;
              font-size: %20px; font-weight: %22; }
QMenu::item:selected { background: %6; color: white; }
QMenu::separator { height: 1px; background: %8; margin: 4px 10px; }
QMenu::icon { margin-left: 4px; }

/* ── Toolbar ─────────────────────────────────────────────────────────────── */
QToolBar { background: %5; border: none; border-bottom: 2px solid %6; spacing: 3px; padding: 2px 4px; }
QToolBar::separator { background: %8; width: 1px; margin: 4px 3px; }
QToolButton { color: %3; border: none; border-radius: 4px; padding: 4px 8px; }
QToolButton:hover { background: %9; }
QToolButton:pressed, QToolButton:checked { background: %6; color: white; }

/* ── Tabs ────────────────────────────────────────────────────────────────── */
QTabWidget::pane { border: 1px solid %8; background: %4; }
QTabBar::tab { background: %7; color: %3; padding: 6px 14px; margin-right: 2px;
               border-radius: 4px 4px 0 0; font-size: %21px; }
QTabBar::tab:selected { background: %6; color: white; font-weight: bold; }
QTabBar::tab:hover:!selected { background: %9; }

/* ── Botões ──────────────────────────────────────────────────────────────── */
QPushButton { background: %6; color: white; border: none; border-radius: 4px;
              padding: 5px 14px; font-weight: %18; font-size: %19px; }
QPushButton:hover { background: %10; }
QPushButton:pressed { background: %11; }
QPushButton:disabled { background: %8; color: %12; }
QPushButton:flat { background: transparent; color: %6; border: 1px solid %8;
                   font-weight: normal; }

/* ── Inputs ──────────────────────────────────────────────────────────────── */
QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QComboBox
{ background: %13; color: %3; border: 1px solid %8; border-radius: 4px;
  padding: 3px 6px; font-size: %21px; }
QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus { border: 1px solid %6; }
QComboBox::drop-down { border: none; }
QComboBox QAbstractItemView { background: %7; color: %3; selection-background-color: %6; }

/* ── Listas / Árvores / Tabelas (entradas, grupos) ───────────────────────── */
QListWidget, QTreeWidget, QTableWidget, QTreeView, QListView, QTableView
{ background: %13; color: %3; border: 1px solid %8;
  alternate-background-color: %7; font-size: %17px; }
QListWidget::item, QTreeWidget::item, QTreeView::item, QListView::item
{ padding: 2px 4px; }
QListWidget::item:selected, QTreeWidget::item:selected,
QTreeView::item:selected, QListView::item:selected
{ background: %6; color: white; }
QListWidget::item:hover, QTreeWidget::item:hover,
QTreeView::item:hover, QListView::item:hover
{ background: %9; }
QHeaderView::section { background: %7; color: %3; border: 1px solid %8;
                        padding: 4px 8px; font-weight: 600; }

/* ── Diálogos e Labels ───────────────────────────────────────────────────── */
QDialog { background: %4; }
QDialog QLabel { font-size: %16px; color: %3; }
QDialog QGroupBox { color: %3; border: 1px solid %8; border-radius: 6px;
                     margin-top: 10px; padding-top: 8px; background: %4; }
QDialog QGroupBox::title { subcontrol-origin: margin; left:10px; padding:0 4px;
                             font-weight: %23; font-size: %16px; color: %14; }
QGroupBox { color: %3; border: 1px solid %8; border-radius: 6px;
            margin-top: 10px; padding-top: 6px; font-weight: bold; background: %4; }
QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px;
                    color: %14; font-weight: %23; }

/* ── Scrollbars ──────────────────────────────────────────────────────────── */
QScrollBar:vertical { background: %4; width: 10px; border-radius: 5px; }
QScrollBar::handle:vertical { background: %8; border-radius: 4px; min-height: 20px; }
QScrollBar::handle:vertical:hover { background: %6; }
QScrollBar:horizontal { background: %4; height: 10px; }
QScrollBar::handle:horizontal { background: %8; border-radius: 4px; min-width: 20px; }
QScrollBar::add-line, QScrollBar::sub-line { width:0; height:0; }

/* ── Splitter ────────────────────────────────────────────────────────────── */
QSplitter::handle { background: %6; }
QSplitter::handle:vertical { height: 6px; }
QSplitter::handle:horizontal { width: 6px; }

/* ── DockWidget ──────────────────────────────────────────────────────────── */
QDockWidget::title { background: %6; color: white; padding: 6px 8px; font-weight: bold; }
QDockWidget { color: %3; }

/* ── Status / Tooltip ────────────────────────────────────────────────────── */
QStatusBar { background: %15; color: %3; border-top: 1px solid %6; font-size: 11px; }
QToolTip { background: %15; color: %3; border: 1px solid %6;
           border-radius: 6px; padding: 6px 10px; font-size: 11px; }

/* ── Progress / Slider ───────────────────────────────────────────────────── */
QProgressBar { background: %7; border: 1px solid %8; border-radius: 5px;
               text-align: center; color: %3; height: 18px; }
QProgressBar::chunk { background: %6; border-radius: 4px; }
QSlider::groove:horizontal { background: %8; height: 4px; border-radius: 2px; }
QSlider::handle:horizontal { background: %6; width: 14px; height: 14px;
                              margin: -5px 0; border-radius: 7px; }

/* ── CheckBox / Radio ────────────────────────────────────────────────────── */
QCheckBox, QRadioButton { color: %3; spacing: 6px; font-size: %21px; }
QCheckBox::indicator, QRadioButton::indicator
{ width: 15px; height: 15px; border: 2px solid %8; border-radius: 3px; background: %13; }
QCheckBox::indicator:checked, QRadioButton::indicator:checked
{ background: %6; border-color: %6; }
)")
    .arg(fontFamily)                       // %1
    .arg(fontSize)                         // %2  base font size
    .arg(text)                             // %3  text color
    .arg(bg)                               // %4  background
    .arg(toolbarBg)                        // %5  toolbar bg
    .arg(accent)                           // %6  accent / highlight
    .arg(bgLight)                          // %7  bg menu / tab inactive
    .arg(border)                           // %8  borders
    .arg(acLight)                          // %9  hover
    .arg(acC.lighter(110).name())          // %10 btn hover
    .arg(acDark)                           // %11 btn pressed
    .arg(txC.darker(150).name())           // %12 text disabled
    .arg(inputBg)                          // %13 input bg
    .arg(acC.lighter(140).name())          // %14 groupbox title color
    .arg(bgDark)                           // %15 statusbar / tooltip bg
    .arg(dialogFontSize)                   // %16 dialog / label size
    .arg(listFontSize)                     // %17 list / tree size
    .arg(btnWStr)                          // %18 button weight
    .arg(btnFontSize)                      // %19 button size
    .arg(menuFontSize)                     // %20 menu size
    .arg(fontSize)                         // %21 general inputs / combos / checks
    .arg(menuWStr)                         // %22 menu weight
    .arg(dlgTitleW);                       // %23 dialog/groupbox title weight
}
