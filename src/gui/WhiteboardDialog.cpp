#include "WhiteboardDialog.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/CustomData.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QToolBar>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QInputDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QApplication>
#include <QScreen>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>
#include <QWheelEvent>

const QString WhiteboardDialog::METADATA_KEY = "CfdmNote::WhiteboardLayout";

// ── ZoomView ──────────────────────────────────────────────────────────────────
class WbZoomView : public QGraphicsView {
public:
    using QGraphicsView::QGraphicsView;
protected:
    void wheelEvent(QWheelEvent* ev) override {
        const double f = ev->angleDelta().y() > 0 ? 1.15 : 0.87;
        scale(f, f);
        ev->accept();
    }
};

// ─── WbEntryCard ──────────────────────────────────────────────────────────────

WbEntryCard::WbEntryCard(Entry* entry, const QColor& color, QGraphicsItem* parent)
    : QGraphicsItem(parent), m_entry(entry), m_color(color)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setAcceptHoverEvents(true);
    setZValue(2);
    setToolTip(QString("<b>%1</b><br>%2<br>%3")
               .arg(entry->title().toHtmlEscaped())
               .arg(entry->group() ? entry->group()->name().toHtmlEscaped() : "")
               .arg(entry->url().toHtmlEscaped()));
}

QString WbEntryCard::id() const
{
    return m_entry ? m_entry->uuid().toString() : QString();
}

QRectF WbEntryCard::boundingRect() const { return QRectF(0, 0, 180, 90); }

void WbEntryCard::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*)
{
    p->setRenderHint(QPainter::Antialiasing);
    const QColor border = m_hovered ? m_color.lighter(130) : m_color;
    const QColor bg     = m_hovered ? QColor("#f7faff") : Qt::white;

    // Sombra
    p->setPen(Qt::NoPen);
    p->setBrush(QColor(0,0,0,30));
    p->drawRoundedRect(4, 4, 178, 88, 8, 8);

    // Fundo
    p->setBrush(bg);
    p->setPen(QPen(border, isSelected() ? 3 : 2));
    p->drawRoundedRect(0, 0, 178, 88, 8, 8);

    // Barra colorida topo
    p->setBrush(m_color);
    p->setPen(Qt::NoPen);
    p->drawRoundedRect(0, 0, 178, 22, 8, 8);
    p->drawRect(0, 8, 178, 14); // canto inferior reto

    // Título
    p->setPen(Qt::white);
    QFont fBold = p->font();
    fBold.setBold(true);
    fBold.setPointSize(9);
    p->setFont(fBold);
    p->drawText(QRectF(6, 3, 166, 18), Qt::AlignVCenter | Qt::AlignLeft,
                m_entry->title().left(22) + (m_entry->title().length() > 22 ? "…" : ""));

    // Grupo
    p->setPen(QColor("#4a5568"));
    QFont fSmall = p->font();
    fSmall.setBold(false);
    fSmall.setPointSize(8);
    p->setFont(fSmall);
    const QString grp = m_entry->group() ? "📁 " + m_entry->group()->name() : "";
    p->drawText(QRectF(6, 26, 166, 14), Qt::AlignVCenter, grp.left(28));

    // Tags
    if (!m_entry->tags().isEmpty()) {
        p->setPen(QColor("#276749"));
        p->drawText(QRectF(6, 42, 166, 14), Qt::AlignVCenter,
                    "🏷️ " + m_entry->tags().left(24));
    }

    // URL
    if (!m_entry->url().isEmpty()) {
        p->setPen(QColor("#3182ce"));
        p->drawText(QRectF(6, 58, 166, 14), Qt::AlignVCenter,
                    "🔗 " + m_entry->url().left(26));
    }

    // Ícone "arrastar"
    p->setPen(QColor("#a0aec0"));
    for (int i = 0; i < 3; ++i)
        p->drawLine(158 + i*5, 10, 158 + i*5, 16);
}

void WbEntryCard::mousePressEvent(QGraphicsSceneMouseEvent* ev)
{
    QGraphicsItem::mousePressEvent(ev);
    setZValue(10);
}

void WbEntryCard::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* ev)
{
    QGraphicsItem::mouseDoubleClickEvent(ev);
    if (!m_entry) return;
    QMessageBox::information(nullptr, m_entry->title(),
        QString("<b>%1</b><br><br>👤 %2<br>🔗 %3<br>🏷️ %4")
        .arg(m_entry->title().toHtmlEscaped())
        .arg(m_entry->username().toHtmlEscaped())
        .arg(m_entry->url().toHtmlEscaped())
        .arg(m_entry->tags().toHtmlEscaped()));
}

void WbEntryCard::hoverEnterEvent(QGraphicsSceneHoverEvent* ev)
{
    QGraphicsItem::hoverEnterEvent(ev);
    m_hovered = true;
    update();
    setCursor(Qt::SizeAllCursor);
}

void WbEntryCard::hoverLeaveEvent(QGraphicsSceneHoverEvent* ev)
{
    QGraphicsItem::hoverLeaveEvent(ev);
    m_hovered = false;
    setZValue(2);
    update();
    setCursor(Qt::ArrowCursor);
}

// ─── WbStickyNote ─────────────────────────────────────────────────────────────

WbStickyNote::WbStickyNote(const QString& text, const QColor& bg, QGraphicsItem* parent)
    : QGraphicsItem(parent), m_text(text), m_bg(bg)
    , m_id(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8))
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setZValue(3);
}

QRectF WbStickyNote::boundingRect() const { return QRectF(0, 0, 160, 120); }

void WbStickyNote::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*)
{
    p->setRenderHint(QPainter::Antialiasing);
    // Sombra
    p->setPen(Qt::NoPen);
    p->setBrush(QColor(0,0,0,25));
    p->drawRoundedRect(4, 4, 158, 118, 6, 6);

    // Fundo
    p->setBrush(m_bg);
    p->setPen(isSelected() ? QPen(Qt::black, 2) : QPen(m_bg.darker(110), 1));
    p->drawRoundedRect(0, 0, 158, 118, 6, 6);

    // Dobra cantoneira (efeito sticky note)
    p->setPen(Qt::NoPen);
    p->setBrush(m_bg.darker(130));
    QPolygonF fold;
    fold << QPointF(138,0) << QPointF(158,20) << QPointF(158,0);
    p->drawPolygon(fold);

    // Texto
    p->setPen(QColor("#1a202c"));
    QFont f = p->font();
    f.setPointSize(10);
    p->setFont(f);
    p->drawText(QRectF(8, 8, 124, 104), Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap, m_text);

    // Hint duplo clique
    p->setPen(QColor(0,0,0,60));
    QFont fTiny = f;
    fTiny.setPointSize(7);
    p->setFont(fTiny);
    p->drawText(QRectF(4, 106, 150, 12), Qt::AlignRight, "✎ duplo clique p/ editar");
}

void WbStickyNote::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* ev)
{
    QGraphicsItem::mouseDoubleClickEvent(ev);
    bool ok;
    const QString newText = QInputDialog::getMultiLineText(
        nullptr, QObject::tr("Editar nota"), QObject::tr("Texto:"), m_text, &ok);
    if (ok) { m_text = newText; update(); }
}

// ─── WhiteboardDialog ─────────────────────────────────────────────────────────

WhiteboardDialog::WhiteboardDialog(QSharedPointer<Database> db, QWidget* parent)
    : QDialog(parent), m_db(db)
{
    setWindowTitle(tr("🖌️ Whiteboard — CfdmNote"));
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
    const QRect scr = QApplication::primaryScreen()->availableGeometry();
    resize(qMin(1200, scr.width()-60), qMin(760, scr.height()-60));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(0);

    // ── Toolbar ───────────────────────────────────────────────────────────────
    auto* bar = new QToolBar();
    bar->setStyleSheet("QToolBar { background: #2d3748; border: none; padding: 4px; spacing: 4px; }"
                       "QToolButton { color: white; padding: 4px 10px; border-radius: 4px; font-size: 12px; }"
                       "QToolButton:hover { background: #4a5568; }");

    auto* lblT = new QLabel("  🖌️  Whiteboard");
    lblT->setStyleSheet("color: #e2e8f0; font-weight: bold; font-size: 13px;");
    bar->addWidget(lblT);
    bar->addSeparator();

    auto* actYellow = bar->addAction("🟡 Nota Amarela");
    auto* actBlue   = bar->addAction("🔵 Nota Azul");
    auto* actGreen  = bar->addAction("🟢 Nota Verde");
    auto* actPink   = bar->addAction("🩷 Nota Rosa");
    bar->addSeparator();
    auto* actAdd    = bar->addAction("➕ Adicionar Entrada");
    actAdd->setToolTip(tr("Adiciona a entrada selecionada do painel esquerdo ao canvas"));
    bar->addSeparator();
    auto* actSave   = bar->addAction("💾 Salvar Layout");
    auto* actLoad   = bar->addAction("📂 Carregar Layout");
    bar->addSeparator();
    auto* actClear  = bar->addAction("🗑 Limpar");

    root->addWidget(bar);

    // ── Splitter: lista de entradas | canvas ──────────────────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setChildrenCollapsible(false);

    // Painel de entradas disponíveis
    auto* leftPanel = new QWidget();
    leftPanel->setFixedWidth(220);
    leftPanel->setStyleSheet("QWidget { background: #f7fafc; } "
                             "QLabel { color: #2d3748; }");
    auto* leftLay = new QVBoxLayout(leftPanel);
    leftLay->setContentsMargins(8,8,8,8);
    leftLay->setSpacing(4);

    auto* leftHeader = new QLabel("📚 Entradas disponíveis");
    leftHeader->setStyleSheet("font-weight: bold; font-size: 12px; color: #2c5282; "
                              "border-bottom: 2px solid #3182ce; padding-bottom: 4px;");
    leftLay->addWidget(leftHeader);

    auto* hintLbl = new QLabel(tr("Selecione e clique\n'Adicionar Entrada'"));
    hintLbl->setStyleSheet("font-size: 10px; color: #718096; background: #ebf8ff; "
                           "border-radius: 4px; padding: 4px;");
    hintLbl->setAlignment(Qt::AlignCenter);
    leftLay->addWidget(hintLbl);

    m_entryList = new QListWidget();
    m_entryList->setStyleSheet(
        "QListWidget { border: 1px solid #bee3f8; border-radius: 4px; }"
        "QListWidget::item { padding: 6px 4px; border-bottom: 1px solid #edf2f7; }"
        "QListWidget::item:selected { background: #bee3f8; color: #1a365d; }");
    if (db) {
        const auto entries = db->rootGroup()->entriesRecursive();
        for (auto* e : entries) {
            auto* item = new QListWidgetItem(e->title());
            item->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(e)));
            item->setToolTip(QString("%1\n%2").arg(e->title(), e->url()));
            m_entryList->addItem(item);
        }
    }
    leftLay->addWidget(m_entryList, 1);
    splitter->addWidget(leftPanel);

    // Canvas
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(-2000, -2000, 4000, 4000);
    m_scene->setBackgroundBrush(QBrush(QColor("#f0f4f8"),
                                       Qt::Dense4Pattern));  // grid visual

    m_view = new WbZoomView(m_scene);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setDragMode(QGraphicsView::RubberBandDrag);
    m_view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    splitter->addWidget(m_view);
    splitter->setSizes({220, width()-220});
    root->addWidget(splitter, 1);

    // ── Rodapé ────────────────────────────────────────────────────────────────
    auto* footer = new QWidget();
    footer->setFixedHeight(36);
    footer->setStyleSheet("background: #2d3748;");
    auto* fLay = new QHBoxLayout(footer);
    fLay->setContentsMargins(12,4,12,4);
    auto* fInfo = new QLabel(tr("🖱️ Scroll: zoom  |  Arrastar fundo: mover canvas  |  Duplo clique nas notas: editar"));
    fInfo->setStyleSheet("color: #a0aec0; font-size: 11px;");
    fLay->addWidget(fInfo);
    fLay->addStretch();
    auto* btnClose = new QPushButton(tr("Fechar"));
    btnClose->setStyleSheet("background: #4a5568; color: white; border-radius: 4px; padding: 4px 16px;");
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    fLay->addWidget(btnClose);
    root->addWidget(footer);

    // ── Conexões ──────────────────────────────────────────────────────────────
    connect(actYellow, &QAction::triggered, this, [this]() { addStickyNote(QColor("#fef3c7")); });
    connect(actBlue,   &QAction::triggered, this, [this]() { addStickyNote(QColor("#dbeafe")); });
    connect(actGreen,  &QAction::triggered, this, [this]() { addStickyNote(QColor("#d1fae5")); });
    connect(actPink,   &QAction::triggered, this, [this]() { addStickyNote(QColor("#fce7f3")); });
    connect(actAdd,    &QAction::triggered, this, &WhiteboardDialog::addEntryToBoard);
    connect(actSave,   &QAction::triggered, this, &WhiteboardDialog::saveLayout);
    connect(actLoad,   &QAction::triggered, this, &WhiteboardDialog::loadLayout);
    connect(actClear,  &QAction::triggered, this, &WhiteboardDialog::clearBoard);

    // Paleta de grupo para as entradas
    static const QList<QColor> pal = {
        QColor("#3182ce"), QColor("#38a169"), QColor("#dd6b20"),
        QColor("#805ad5"), QColor("#e53e3e"), QColor("#00b5d8") };

    // Adiciona entradas que tinham layout salvo automaticamente
    loadLayout();
}

void WhiteboardDialog::showBoard(QSharedPointer<Database> db, QWidget* parent)
{
    auto* dlg = new WhiteboardDialog(db, parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

void WhiteboardDialog::addStickyNote(const QColor& color)
{
    bool ok;
    const QString text = QInputDialog::getMultiLineText(
        this, tr("Nova Nota"), tr("Texto da nota:"), "", &ok);
    if (!ok) return;

    auto* note = new WbStickyNote(text, color);
    const QPointF center = m_view->mapToScene(m_view->viewport()->rect().center());
    note->setPos(center.x() + (m_nextId % 5) * 20 - 40,
                 center.y() + (m_nextId % 3) * 20 - 20);
    m_scene->addItem(note);
    ++m_nextId;
}

void WhiteboardDialog::addEntryToBoard()
{
    auto* item = m_entryList->currentItem();
    if (!item) {
        QMessageBox::information(this, tr("Whiteboard"),
            tr("Selecione uma entrada no painel esquerdo primeiro."));
        return;
    }
    auto* e = static_cast<Entry*>(item->data(Qt::UserRole).value<void*>());
    if (!e) return;

    // Verifica se já está no canvas
    for (auto* gi : m_scene->items()) {
        if (auto* card = dynamic_cast<WbEntryCard*>(gi))
            if (card->entry() == e) {
                QMessageBox::information(this, tr("Whiteboard"),
                    tr("Esta entrada já está no canvas."));
                return;
            }
    }

    static const QList<QColor> pal = {
        QColor("#3182ce"), QColor("#38a169"), QColor("#dd6b20"),
        QColor("#805ad5"), QColor("#e53e3e"), QColor("#00b5d8") };

    int colorIdx = 0;
    if (e->group()) {
        const auto allGroups = m_db->rootGroup()->groupsRecursive(true);
        colorIdx = allGroups.indexOf(const_cast<Group*>(e->group())) % pal.size();
    }

    auto* card = new WbEntryCard(e, pal[qAbs(colorIdx) % pal.size()]);
    const QPointF center = m_view->mapToScene(m_view->viewport()->rect().center());
    card->setPos(center.x() - 90 + (m_nextId % 6) * 30,
                 center.y() - 45 + (m_nextId % 4) * 25);
    m_scene->addItem(card);
    ++m_nextId;
}

void WhiteboardDialog::clearBoard()
{
    if (QMessageBox::question(this, tr("Limpar Whiteboard"),
            tr("Remover todos os itens do canvas?"),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;
    m_scene->clear();
}

void WhiteboardDialog::saveLayout()
{
    if (!m_db) return;

    QJsonArray arr;
    for (auto* gi : m_scene->items()) {
        QJsonObject obj;
        obj["x"] = gi->pos().x();
        obj["y"] = gi->pos().y();

        if (auto* card = dynamic_cast<WbEntryCard*>(gi)) {
            obj["type"] = "entry";
            obj["uuid"] = card->entry() ? card->entry()->uuid().toString() : "";
        } else if (auto* note = dynamic_cast<WbStickyNote*>(gi)) {
            obj["type"]  = "sticky";
            obj["text"]  = note->text();
            obj["color"] = note->bg().name();
            obj["id"]    = note->id();
        }
        arr.append(obj);
    }

    const QString json = QJsonDocument(arr).toJson(QJsonDocument::Compact);
    m_db->metadata()->customData()->set(METADATA_KEY, json);
    QString err;
    m_db->save(Database::Atomic, {}, &err);

    QMessageBox::information(this, tr("Whiteboard"),
        tr("Layout salvo com sucesso!"));
}

void WhiteboardDialog::loadLayout()
{
    if (!m_db) return;
    const QString json = m_db->metadata()->customData()->value(METADATA_KEY);
    if (json.isEmpty()) return;

    m_scene->clear();

    static const QList<QColor> pal = {
        QColor("#3182ce"), QColor("#38a169"), QColor("#dd6b20"),
        QColor("#805ad5"), QColor("#e53e3e"), QColor("#00b5d8") };

    const QJsonArray arr = QJsonDocument::fromJson(json.toUtf8()).array();
    for (const auto& val : arr) {
        const QJsonObject obj = val.toObject();
        const double x = obj["x"].toDouble();
        const double y = obj["y"].toDouble();
        const QString type = obj["type"].toString();

        if (type == "entry") {
            const QUuid uuid = QUuid(obj["uuid"].toString());
            Entry* e = m_db->rootGroup()->findEntryByUuid(uuid);
            if (!e) continue;
            int ci = 0;
            if (e->group()) {
                const auto allGroups = m_db->rootGroup()->groupsRecursive(true);
                ci = allGroups.indexOf(const_cast<Group*>(e->group())) % pal.size();
            }
            auto* card = new WbEntryCard(e, pal[qAbs(ci) % pal.size()]);
            card->setPos(x, y);
            m_scene->addItem(card);
        } else if (type == "sticky") {
            auto* note = new WbStickyNote(obj["text"].toString(),
                                          QColor(obj["color"].toString()));
            note->setPos(x, y);
            m_scene->addItem(note);
        }
    }
}
