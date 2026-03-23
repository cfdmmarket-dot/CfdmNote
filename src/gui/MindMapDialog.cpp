#include "MindMapDialog.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QToolBar>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QGraphicsPathItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QApplication>
#include <QScreen>
#include <QTimer>
#include <QWheelEvent>
#include <QSvgGenerator>
#include <QFileDialog>
#include <QMessageBox>
#include <QtMath>

static const QList<QColor> COLORS = {
    {"#3182ce"}, {"#38a169"}, {"#dd6b20"}, {"#805ad5"},
    {"#e53e3e"}, {"#00b5d8"}, {"#d69e2e"}, {"#319795"},
    {"#667eea"}, {"#ed64a6"}, {"#f6ad55"}, {"#68d391"},
};

// ── View com zoom ─────────────────────────────────────────────────────────────
class MindView : public QGraphicsView {
public:
    using QGraphicsView::QGraphicsView;
protected:
    void wheelEvent(QWheelEvent* ev) override {
        scale(ev->angleDelta().y() > 0 ? 1.15 : 0.87, ev->angleDelta().y() > 0 ? 1.15 : 0.87);
        ev->accept();
    }
};

// ─── MindNode ─────────────────────────────────────────────────────────────────

MindNode::MindNode(Kind kind, const QString& label, const QColor& color,
                   Group* group, Entry* entry,
                   bool hasChildren, bool expanded, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_kind(kind), m_label(label), m_color(color)
    , m_group(group), m_entry(entry)
    , m_hasChildren(hasChildren), m_expanded(expanded)
{
    setAcceptHoverEvents(true);
    setCursor(Qt::PointingHandCursor);
    setZValue(kind == Kind::Root ? 10 : kind == Kind::Group ? 5 : 3);
    if (kind == Kind::Group)
        setToolTip(QString("<b>📁 %1</b><br>Clique para %2")
                   .arg(label.toHtmlEscaped())
                   .arg(expanded ? "recolher" : "expandir"));
    else if (kind == Kind::Entry)
        setToolTip(QString("<b>%1</b><br>%2")
                   .arg(label.toHtmlEscaped())
                   .arg(entry ? entry->url().toHtmlEscaped() : ""));
}

QRectF MindNode::boundingRect() const
{
    switch (m_kind) {
        case Kind::Root:  return {-46,-46, 92, 92};
        case Kind::Group: {
            const int w = qMax(90, qMin(180, m_label.length() * 8 + 40));
            return {-(double)w/2, -18, (double)w, 36};
        }
        case Kind::Entry: {
            const int w = qMax(70, qMin(140, m_label.length() * 7 + 24));
            return {-(double)w/2, -13, (double)w, 26};
        }
    }
    return {};
}

void MindNode::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*)
{
    p->setRenderHint(QPainter::Antialiasing);
    const QColor c = m_hovered ? m_color.lighter(130) : m_color;

    if (m_kind == Kind::Root) {
        QRadialGradient g(0,0,46);
        g.setColorAt(0, c.lighter(130));
        g.setColorAt(1, c);
        p->setBrush(g);
        p->setPen(QPen(Qt::white, 3));
        p->drawEllipse(QRectF(-46,-46,92,92));
        p->setPen(Qt::white);
        QFont f; f.setPointSize(11); f.setBold(true); p->setFont(f);
        p->drawText(QRectF(-42,-42,84,84), Qt::AlignCenter,
                    m_label.left(12) + (m_label.length()>12?"…":""));

    } else if (m_kind == Kind::Group) {
        const QRectF r = boundingRect();
        QLinearGradient g(r.topLeft(), r.bottomRight());
        g.setColorAt(0, c.lighter(115));
        g.setColorAt(1, c);
        p->setBrush(g);
        p->setPen(QPen(m_hovered ? Qt::white : c.darker(130),
                       m_hovered ? 2.5 : 1.5));
        p->drawRoundedRect(r, 18, 18);

        // Badge ▶/▼ se tem filhos
        if (m_hasChildren) {
            p->setPen(Qt::white);
            QFont badge; badge.setPointSize(7); p->setFont(badge);
            p->drawText(QRectF(r.right()-18, r.top(), 16, r.height()),
                        Qt::AlignCenter, m_expanded ? "▼" : "▶");
        }

        p->setPen(Qt::white);
        QFont f; f.setPointSize(9); f.setBold(true); p->setFont(f);
        const QRectF textR(r.left()+6, r.top(), r.width()-28, r.height());
        p->drawText(textR, Qt::AlignVCenter | Qt::AlignLeft,
                    m_label.left(18) + (m_label.length()>18?"…":""));

    } else { // Entry
        const QRectF r = boundingRect();
        p->setBrush(QBrush(c.lighter(185)));
        p->setPen(QPen(c, m_hovered ? 1.5 : 1));
        p->drawRoundedRect(r, 13, 13);
        p->setPen(c.darker(150));
        QFont f; f.setPointSize(8); p->setFont(f);
        p->drawText(r.adjusted(6,0,-6,0), Qt::AlignCenter,
                    m_label.left(16) + (m_label.length()>16?"…":""));
    }
}

void MindNode::mousePressEvent(QGraphicsSceneMouseEvent* ev)
{
    QGraphicsItem::mousePressEvent(ev);
    if (m_kind == Kind::Group && scene())
        scene()->setProperty("_clickGroup", QVariant::fromValue(static_cast<void*>(m_group)));
    else if (m_kind == Kind::Entry && scene())
        scene()->setProperty("_clickEntry", QVariant::fromValue(static_cast<void*>(m_entry)));
}

void MindNode::hoverEnterEvent(QGraphicsSceneHoverEvent* ev)
{ QGraphicsItem::hoverEnterEvent(ev); m_hovered = true; update(); }
void MindNode::hoverLeaveEvent(QGraphicsSceneHoverEvent* ev)
{ QGraphicsItem::hoverLeaveEvent(ev); m_hovered = false; update(); }

// ─── MindMapDialog ────────────────────────────────────────────────────────────

MindMapDialog::MindMapDialog(QSharedPointer<Database> db, QWidget* parent)
    : QDialog(parent), m_db(db)
{
    setWindowTitle(tr("🧠 Mapa Mental — CfdmNote"));
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
    const QRect scr = QApplication::primaryScreen()->availableGeometry();
    resize(qMin(1200, scr.width()-40), qMin(800, scr.height()-40));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(0);

    // ── Toolbar ───────────────────────────────────────────────────────────────
    auto* bar = new QToolBar();
    bar->setStyleSheet(
        "QToolBar{background:#1a365d;border:none;padding:4px;spacing:6px;}"
        "QToolButton{color:white;padding:4px 12px;border-radius:4px;}"
        "QToolButton:hover{background:#3182ce;}"
        "QCheckBox{color:#bee3f8;margin:0 8px;}"
        "QLabel{color:#bee3f8;}");

    auto* lbl = new QLabel("  🧠 Mapa Mental");
    lbl->setStyleSheet("font-weight:bold; font-size:13px; color:#ebf8ff;");
    bar->addWidget(lbl);
    bar->addSeparator();

    auto* actFit     = bar->addAction("⊡ Encaixar tudo");
    auto* actCollAll = bar->addAction("➖ Recolher tudo");
    auto* actExpAll  = bar->addAction("➕ Expandir tudo");
    auto* actExport  = bar->addAction("💾 SVG");
    bar->addSeparator();

    m_chkEntries = new QCheckBox(tr("Mostrar entradas"));
    m_chkEntries->setChecked(true);
    bar->addWidget(m_chkEntries);

    auto* hint = new QLabel(tr("   Clique num grupo para expandir/recolher"));
    hint->setStyleSheet("color:#90cdf4; font-size:11px; font-style:italic;");
    bar->addWidget(hint);
    root->addWidget(bar);

    // ── Splitter ──────────────────────────────────────────────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setChildrenCollapsible(false);

    m_scene = new QGraphicsScene(this);
    m_scene->setBackgroundBrush(QColor("#0e1c2f"));

    m_view = new MindView(m_scene);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setRenderHint(QPainter::SmoothPixmapTransform);
    m_view->setDragMode(QGraphicsView::ScrollHandDrag);
    m_view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    splitter->addWidget(m_view);

    // Painel lateral
    auto* panel = new QWidget();
    panel->setFixedWidth(230);
    panel->setStyleSheet("QWidget{background:#f0f7ff;} QLabel{color:#1a365d;}");
    auto* pLay = new QVBoxLayout(panel);
    pLay->setContentsMargins(12,12,12,12);
    pLay->setSpacing(8);

    auto* pH = new QLabel("📋 Detalhes");
    pH->setStyleSheet("font-weight:bold; font-size:12px; color:#2c5282; "
                      "border-bottom:2px solid #3182ce; padding-bottom:5px;");
    pLay->addWidget(pH);

    m_detailTitle = new QLabel(tr("Clique em um nó"));
    m_detailTitle->setWordWrap(true);
    m_detailTitle->setStyleSheet("font-size:12px; font-weight:bold; color:#1a365d;");
    pLay->addWidget(m_detailTitle);

    m_detailSub = new QLabel();
    m_detailSub->setWordWrap(true);
    m_detailSub->setStyleSheet("font-size:11px; color:#4a5568; background:#dbeafe; "
                               "border-radius:4px; padding:4px 6px;");
    pLay->addWidget(m_detailSub);

    m_detailBody = new QLabel();
    m_detailBody->setWordWrap(true);
    m_detailBody->setStyleSheet("font-size:11px; color:#1a202c; background:#f7faff; "
                                "border:1px solid #bee3f8; border-radius:4px; padding:6px;");
    pLay->addWidget(m_detailBody);
    pLay->addStretch();

    // Legenda
    const QStringList legItems = {
        "▶ Nó com filhos — clique p/ expandir",
        "▼ Nó expandido — clique p/ recolher",
        "🖱️ Scroll = zoom",
        "🖐️ Arrastar = mover canvas",
    };
    for (const auto& s : legItems) {
        auto* l = new QLabel(s);
        l->setStyleSheet("font-size:10px; color:#4a5568; padding:1px 0;");
        pLay->addWidget(l);
    }
    pLay->addSpacing(8);
    auto* btnClose = new QPushButton(tr("Fechar"));
    btnClose->setStyleSheet("background:#e2e8f0; color:#1a365d; border-radius:5px; padding:8px;");
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    pLay->addWidget(btnClose);

    splitter->addWidget(panel);
    splitter->setSizes({width()-230, 230});
    root->addWidget(splitter, 1);

    // ── Conexões ──────────────────────────────────────────────────────────────
    connect(actFit, &QAction::triggered, this, &MindMapDialog::fitAll);
    connect(actExport, &QAction::triggered, this, [this]() {
        const QString p = QFileDialog::getSaveFileName(this, tr("Exportar SVG"),
                                                       QDir::homePath(), tr("SVG (*.svg)"));
        if (p.isEmpty()) return;
        const QRectF b = m_scene->itemsBoundingRect().adjusted(-20,-20,20,20);
        QSvgGenerator g; g.setFileName(p); g.setViewBox(b);
        QPainter pt(&g); m_scene->render(&pt, {}, b); pt.end();
        QMessageBox::information(this, tr("Exportado"), tr("Salvo em:\n%1").arg(p));
    });
    connect(actCollAll, &QAction::triggered, this, [this]() {
        m_expanded.clear();
        render();
        QTimer::singleShot(40, this, &MindMapDialog::fitAll);
    });
    connect(actExpAll, &QAction::triggered, this, [this]() {
        if (m_db)
            for (auto* g : m_db->rootGroup()->groupsRecursive(true))
                m_expanded.insert(g);
        render();
        QTimer::singleShot(40, this, &MindMapDialog::fitAll);
    });
    connect(m_chkEntries, &QCheckBox::toggled, this, [this]() {
        render(); QTimer::singleShot(40, this, &MindMapDialog::fitAll);
    });

    // Cliques nos nós via scene property
    connect(m_scene, &QGraphicsScene::changed, this, [this](const QList<QRectF>&) {
        void* gp = m_scene->property("_clickGroup").value<void*>();
        if (gp) {
            m_scene->setProperty("_clickGroup", QVariant());
            auto* g = static_cast<Group*>(gp);
            if (m_expanded.contains(g))
                m_expanded.remove(g);
            else
                m_expanded.insert(g);
            render();
            updateDetail(g);
        }
        void* ep = m_scene->property("_clickEntry").value<void*>();
        if (ep) {
            m_scene->setProperty("_clickEntry", QVariant());
            updateDetail(static_cast<Entry*>(ep));
        }
    });

    // Começa com os grupos de nível 1 visíveis mas recolhidos
    render();
    QTimer::singleShot(120, this, &MindMapDialog::fitAll);
}

void MindMapDialog::showMap(QSharedPointer<Database> db, QWidget* parent)
{
    auto* dlg = new MindMapDialog(db, parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

// ─── Renderização ─────────────────────────────────────────────────────────────

void MindMapDialog::render()
{
    m_scene->clear();
    m_groupPos.clear();
    if (!m_db) return;

    Group* rootGrp = m_db->rootGroup();
    const QString dbName = m_db->metadata()->name().isEmpty()
                           ? tr("Banco") : m_db->metadata()->name();

    // Nó raiz
    auto* rootNode = new MindNode(MindNode::Kind::Root, dbName,
                                  QColor("#2c5282"), rootGrp);
    rootNode->setPos(0, 0);
    m_scene->addItem(rootNode);
    m_groupPos[rootGrp] = {0, 0};

    // Distribui filhos de nível 1 em 360° ao redor da raiz
    const auto top = rootGrp->children();
    if (top.isEmpty()) return;

    const int n = top.size();
    for (int i = 0; i < n; ++i) {
        const double angle = (2.0 * M_PI * i) / n - M_PI / 2.0;
        const double fan   = (2.0 * M_PI) / n;
        renderGroup(top[i], {0,0}, angle, qMin(fan, M_PI * 0.85),
                    220.0, 1, COLORS[i % COLORS.size()]);
    }
}

void MindMapDialog::renderGroup(Group* g, QPointF parentPos,
                                double dirAngle, double fanAngle,
                                double radius, int depth,
                                const QColor& color)
{
    // Posição deste grupo
    const QPointF pos(parentPos.x() + radius * qCos(dirAngle),
                      parentPos.y() + radius * qSin(dirAngle));
    m_groupPos[g] = pos;

    const bool isExpanded = m_expanded.contains(g);
    const bool hasKids = !g->children().isEmpty()
                      || (m_chkEntries->isChecked() && !g->entries().isEmpty());

    // Curva do pai ao nó
    addCurve(parentPos, pos, color, depth == 1 ? 3 : 2);

    // Nó do grupo
    auto* node = new MindNode(MindNode::Kind::Group, g->name(), color,
                              g, nullptr, hasKids, isExpanded);
    node->setPos(pos);
    m_scene->addItem(node);

    if (!isExpanded) return;

    // ── Filho expandidos: espalha em leque na direção do nó ──────────────────
    const auto subs    = g->children();
    const auto entries = m_chkEntries->isChecked() ? g->entries() : QList<Entry*>{};
    const int totalChildren = subs.size() + entries.size();
    if (totalChildren == 0) return;

    // O leque se abre para fora (mesma direção do pai→este nó)
    // com ângulo máximo de 110° para não voltar sobre si mesmo
    const double childFan    = qMin(fanAngle, M_PI * 0.60);
    const double childRadius = qMax(130.0, radius * 0.68);
    const double step        = totalChildren > 1
                               ? childFan / (totalChildren - 1)
                               : 0;
    const double startAngle  = dirAngle - childFan / 2.0;

    int idx = 0;

    // Subgrupos
    const double subFan = totalChildren > 1 ? childFan / totalChildren : childFan;
    for (auto* sub : subs) {
        const double a = startAngle + idx * step;
        renderGroup(sub, pos, a, subFan * 0.80, childRadius,
                    depth + 1, color.lighter(100 + idx * 6));
        ++idx;
    }

    // Entradas (folhas)
    for (auto* e : entries) {
        const double a   = startAngle + idx * step;
        const QPointF ep(pos.x() + childRadius * 0.70 * qCos(a),
                         pos.y() + childRadius * 0.70 * qSin(a));
        addCurve(pos, ep, color.lighter(160), 1);
        auto* en = new MindNode(MindNode::Kind::Entry, e->title(),
                                color, nullptr, e);
        en->setPos(ep);
        m_scene->addItem(en);
        ++idx;
    }
}

void MindMapDialog::addCurve(QPointF from, QPointF to,
                             const QColor& color, int w)
{
    QPainterPath path;
    path.moveTo(from);
    const QPointF ctrl1 = from + (to - from) * 0.4;
    const QPointF ctrl2 = from + (to - from) * 0.6;
    path.cubicTo(ctrl1, ctrl2, to);
    auto* pi = m_scene->addPath(path, QPen(color, w, Qt::SolidLine,
                                           Qt::RoundCap, Qt::RoundJoin));
    pi->setZValue(0);
}

void MindMapDialog::fitAll()
{
    m_view->fitInView(m_scene->itemsBoundingRect().adjusted(-50,-50,50,50),
                      Qt::KeepAspectRatio);
}

void MindMapDialog::updateDetail(Entry* e)
{
    if (!e) return;
    m_detailTitle->setText(e->title());
    m_detailSub->setText(e->group() ? "📁 " + e->group()->name() : "");
    QStringList body;
    if (!e->username().isEmpty()) body << "👤 " + e->username();
    if (!e->url().isEmpty())      body << "🔗 " + e->url();
    if (!e->tags().isEmpty())     body << "🏷️ " + e->tags();
    const QString c = e->attributes()->value("CfdmNote::Comment");
    if (!c.isEmpty())             body << "💬 " + c;
    m_detailBody->setText(body.join("\n"));
}

void MindMapDialog::updateDetail(Group* g)
{
    if (!g) return;
    m_detailTitle->setText("📁 " + g->name());
    const int direct = g->entries().size();
    const int total  = g->entriesRecursive().size();
    m_detailSub->setText(tr("%1 entradas diretas · %2 no total").arg(direct).arg(total));
    m_detailBody->setText(tr("%1 subgrupo(s)").arg(g->children().size()));
}
