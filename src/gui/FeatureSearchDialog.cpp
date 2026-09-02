#include "FeatureSearchDialog.h"
#include <QCoreApplication>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QKeyEvent>
#include <QScrollBar>

FeatureSearchDialog::FeatureSearchDialog(const QList<FeatureItem>& features,
                                           QWidget* parent)
    : QDialog(parent), m_features(features)
{
    setWindowTitle(tr("🔍 Buscar Funcionalidade"));
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setMinimumWidth(560);
    setMaximumWidth(700);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Campo de busca ────────────────────────────────────────────────────────
    auto* searchBar = new QWidget();
    searchBar->setObjectName("searchBar");
    searchBar->setStyleSheet(
        "#searchBar { background:#1e3a5f; border-radius:8px 8px 0 0; }");
    auto* sbLay = new QHBoxLayout(searchBar);
    sbLay->setContentsMargins(12, 10, 12, 10);

    auto* icon = new QLabel("🔍");
    icon->setStyleSheet("font-size:18px;");
    sbLay->addWidget(icon);

    m_search = new QLineEdit();
    m_search->setPlaceholderText(
        tr("Buscar funcionalidade... (ex: áudio, mapa, backup, importar)"));
    m_search->setStyleSheet(
        "QLineEdit { background:transparent; border:none; color:#f0f4f8;"
        " font-size:15px; padding:2px 4px; }"
        "QLineEdit::placeholder { color:#94a3b8; }");
    m_search->installEventFilter(this);
    sbLay->addWidget(m_search, 1);

    auto* hint = new QLabel(tr("↑↓ navegar · Enter abrir · Esc fechar"));
    hint->setStyleSheet("color:#60a5fa; font-size:11px;");
    sbLay->addWidget(hint);

    root->addWidget(searchBar);

    // ── Lista de resultados ───────────────────────────────────────────────────
    m_list = new QListWidget();
    m_list->setStyleSheet(R"(
        QListWidget { background:#0f1e35; border:none; outline:none; }
        QListWidget::item { padding:0; border:none; }
        QListWidget::item:selected { background:#2563eb; }
        QListWidget::item:hover:!selected { background:#1e3a5f; }
    )");
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setMinimumHeight(320);
    m_list->setMaximumHeight(480);
    connect(m_list, &QListWidget::itemActivated,
            this, &FeatureSearchDialog::onActivate);
    connect(m_list, &QListWidget::currentItemChanged,
            this, [this](QListWidgetItem* cur, QListWidgetItem*) {
                onCurrentChanged(cur);
            });
    root->addWidget(m_list);

    // ── Painel de detalhes ───────────────────────────────────────────────────
    auto* sep = new QFrame(); sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color:#1e3a5f;");
    root->addWidget(sep);

    m_detail = new QLabel();
    m_detail->setWordWrap(true);
    m_detail->setTextFormat(Qt::RichText);
    m_detail->setContentsMargins(14, 8, 14, 10);
    m_detail->setMinimumHeight(50);
    m_detail->setStyleSheet(
        "QLabel { background:#0f1e35; color:#94a3b8; font-size:12px; }");
    root->addWidget(m_detail);

    connect(m_search, &QLineEdit::textChanged,
            this, &FeatureSearchDialog::onSearch);

    populate();
    m_search->setFocus();
}

// ─── Teclado (Up/Down no search field navegam a lista) ────────────────────────
bool FeatureSearchDialog::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj == m_search && ev->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(ev);
        if (ke->key() == Qt::Key_Down) {
            const int next = m_list->currentRow() + 1;
            if (next < m_list->count()) m_list->setCurrentRow(next);
            return true;
        }
        if (ke->key() == Qt::Key_Up) {
            const int prev = m_list->currentRow() - 1;
            if (prev >= 0) m_list->setCurrentRow(prev);
            return true;
        }
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            if (m_list->currentItem()) {
                onActivate(m_list->currentItem());
                return true;
            }
        }
        if (ke->key() == Qt::Key_Escape) {
            reject(); return true;
        }
    }
    return QDialog::eventFilter(obj, ev);
}

// ─── Filtra ────────────────────────────────────────────────────────────────
void FeatureSearchDialog::onSearch(const QString& text)
{
    populate(text);
}

void FeatureSearchDialog::populate(const QString& filter)
{
    m_list->clear();
    m_filtered.clear();

    const QString f = filter.trimmed().toLower();

    for (int i = 0; i < m_features.size(); ++i) {
        const auto& feat = m_features[i];
        if (!f.isEmpty()) {
            const bool match =
                feat.name.toLower().contains(f)       ||
                feat.category.toLower().contains(f)   ||
                feat.description.toLower().contains(f)||
                feat.shortcut.toLower().contains(f);
            if (!match) continue;
        }
        m_filtered << i;

        // ── Widget personalizado para cada item ──────────────────────────────
        auto* w = new QWidget();
        w->setStyleSheet("QWidget { background:transparent; }");
        auto* hl = new QHBoxLayout(w);
        hl->setContentsMargins(12, 7, 12, 7);
        hl->setSpacing(10);

        // Emoji / ícone
        auto* emojiLbl = new QLabel(feat.emoji);
        emojiLbl->setStyleSheet("font-size:20px; min-width:28px;");
        emojiLbl->setAlignment(Qt::AlignCenter);
        hl->addWidget(emojiLbl);

        // Nome + categoria
        auto* nameLbl = new QLabel(QStringLiteral(
            "<span style='color:#f0f4f8;font-size:13px;font-weight:600;'>%1</span>"
            "<br><span style='color:#64748b;font-size:11px;'>%2</span>")
            .arg(feat.name.toHtmlEscaped(), feat.category.toHtmlEscaped()));
        nameLbl->setTextFormat(Qt::RichText);
        hl->addWidget(nameLbl, 1);

        // Atalho
        if (!feat.shortcut.isEmpty()) {
            auto* scLbl = new QLabel(feat.shortcut);
            scLbl->setStyleSheet(
                "color:#60a5fa; font-size:11px; background:#1e3a5f;"
                " border:1px solid #3b5bdb; border-radius:4px; padding:2px 6px;");
            scLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            hl->addWidget(scLbl);
        }

        auto* item = new QListWidgetItem(m_list);
        item->setSizeHint(QSize(0, 52));
        item->setData(Qt::UserRole, m_filtered.size() - 1); // índice em m_filtered
        m_list->setItemWidget(item, w);
    }

    if (m_list->count() > 0) {
        m_list->setCurrentRow(0);
    } else {
        m_detail->setText(tr("<i>Nenhuma funcionalidade encontrada.</i>"));
    }
}

// ─── Detalhe ao selecionar ───────────────────────────────────────────────────
void FeatureSearchDialog::onCurrentChanged(QListWidgetItem* current)
{
    if (!current) { m_detail->clear(); return; }
    const int listIdx = current->data(Qt::UserRole).toInt();
    if (listIdx < 0 || listIdx >= m_filtered.size()) return;
    const auto& feat = m_features[m_filtered[listIdx]];
    m_detail->setText(QStringLiteral(
        "<b style='color:#60a5fa;'>%1 %2</b>"
        "&nbsp;&nbsp;<span style='color:#475569;font-size:11px;'>%3</span>"
        "<br><span style='color:#94a3b8;'>%4</span>")
        .arg(feat.emoji, feat.name.toHtmlEscaped(),
             feat.shortcut.toHtmlEscaped(),
             feat.description.toHtmlEscaped()));
}

// ─── Lançar ──────────────────────────────────────────────────────────────────
void FeatureSearchDialog::onActivate(QListWidgetItem* item)
{
    const int listIdx = item->data(Qt::UserRole).toInt();
    if (listIdx < 0 || listIdx >= m_filtered.size()) return;
    launch(m_filtered[listIdx]);
}

void FeatureSearchDialog::launch(int idx)
{
    if (idx < 0 || idx >= m_features.size()) return;
    accept();
    QMetaObject::invokeMethod(
        QCoreApplication::instance(),
        [this, idx]() { m_features[idx].action(); },
        Qt::QueuedConnection);
}
