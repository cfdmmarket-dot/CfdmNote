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

// ── Constantes de layout (compartilhadas entre MindNode e MindMapDialog) ──────
static const double MM_ROOT_R  = 42.0;   // raio do nó raiz
static const double MM_NODE_W  = 160.0;  // largura dos nós de grupo/entrada
static const double MM_NODE_HG = 34.0;   // altura do nó de grupo
static const double MM_NODE_HE = 26.0;   // altura do nó de entrada
static const double MM_H_GAP   = 52.0;   // espaço horizontal entre bordas de nós
static const double MM_V_GAP   = 11.0;   // espaço vertical entre irmãos

static const QList<QColor> COLORS = {
    {"#3182ce"}, {"#38a169"}, {"#dd6b20"}, {"#805ad5"},
    {"#e53e3e"}, {"#00b5d8"}, {"#d69e2e"}, {"#319795"},
    {"#667eea"}, {"#ed64a6"},
};

// ── View com zoom por scroll ───────────────────────────────────────────────────
class MindView : public QGraphicsView {
public:
    using QGraphicsView::QGraphicsView;
protected:
    void wheelEvent(QWheelEvent* ev) override {
        const double f = ev->angleDelta().y() > 0 ? 1.15 : 0.87;
        scale(f, f);
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
        setToolTip(QString("<b>%1</b>%2")
                   .arg(label.toHtmlEscaped())
                   .arg(entry && !entry->url().isEmpty()
                        ? "<br>" + entry->url().toHtmlEscaped() : ""));
}

QRectF MindNode::boundingRect() const
{
    switch (m_kind) {
        case Kind::Root:
            return { -MM_ROOT_R, -MM_ROOT_R, MM_ROOT_R*2, MM_ROOT_R*2 };
        case Kind::Group:
            return { -MM_NODE_W/2.0, -MM_NODE_HG/2.0, MM_NODE_W, MM_NODE_HG };
        case Kind::Entry:
            return { -MM_NODE_W/2.0, -MM_NODE_HE/2.0, MM_NODE_W, MM_NODE_HE };
    }
    return {};
}

void MindNode::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*)
{
    p->setRenderHint(QPainter::Antialiasing);
    const QColor c = m_hovered ? m_color.lighter(125) : m_color;

    if (m_kind == Kind::Root) {
        // Círculo com gradiente radial
        QRadialGradient g(0, 0, MM_ROOT_R);
        g.setColorAt(0, c.lighter(145));
        g.setColorAt(1, c);
        p->setBrush(g);
        p->setPen(QPen(Qt::white, 2.5));
        p->drawEllipse(boundingRect());

        p->setPen(Qt::white);
        QFont f; f.setPointSize(10); f.setBold(true); p->setFont(f);
        const QRectF tr = boundingRect().adjusted(6, 6, -6, -6);
        p->drawText(tr, Qt::AlignCenter | Qt::TextWordWrap,
                    m_label.left(16) + (m_label.length() > 16 ? "…" : ""));

    } else if (m_kind == Kind::Group) {
        const QRectF r = boundingRect();

        // Fundo com gradiente linear suave
        QLinearGradient g(r.topLeft(), r.bottomLeft());
        g.setColorAt(0, c.lighter(118));
        g.setColorAt(1, c.darker(108));
        p->setBrush(g);
        p->setPen(QPen(m_hovered ? Qt::white : c.darker(125),
                       m_hovered ? 2.0 : 1.5));
        p->drawRoundedRect(r, 8, 8);

        // Indicador ▶/▼ expandir/recolher
        if (m_hasChildren) {
            p->setPen(QColor(255, 255, 255, 180));
            QFont badge; badge.setPointSize(7); p->setFont(badge);
            p->drawText(QRectF(r.right() - 18, r.top(), 16, r.height()),
                        Qt::AlignCenter, m_expanded ? "▼" : "▶");
        }

        // Texto do grupo
        p->setPen(Qt::white);
        QFont f; f.setPointSize(9); f.setBold(true); p->setFont(f);
        const double rightPad = m_hasChildren ? 20.0 : 8.0;
        const QRectF tr(r.left() + 10, r.top(), r.width() - 10 - rightPad, r.height());
        p->drawText(tr, Qt::AlignVCenter | Qt::AlignLeft,
                    m_label.left(20) + (m_label.length() > 20 ? "…" : ""));

    } else { // Entry
        const QRectF r = boundingRect();

        // Fundo claro com borda colorida
        p->setBrush(QBrush(m_hovered ? c.lighter(195) : c.lighter(210)));
        p->setPen(QPen(c.darker(110), m_hovered ? 2.0 : 1.2));
        p->drawRoundedRect(r, 6, 6);

        // Ícone de entrada
        p->setPen(c.darker(130));
        QFont f; f.setPointSize(8); p->setFont(f);
        p->drawText(r.adjusted(6, 0, -6, 0), Qt::AlignVCenter | Qt::AlignLeft,
                    "● " + (m_label.left(18) + (m_label.length() > 18 ? "…" : "")));
    }
}

void MindNode::mousePressEvent(QGraphicsSceneMouseEvent* ev)
{
    QGraphicsItem::mousePressEvent(ev);
    if (m_kind == Kind::Group && scene())
        scene()->setProperty("_clickGroup",
                             QVariant::fromValue(static_cast<void*>(m_group)));
    else if (m_kind == Kind::Entry && scene())
        scene()->setProperty("_clickEntry",
                             QVariant::fromValue(static_cast<void*>(m_entry)));
}

void MindNode::hoverEnterEvent(QGraphicsSceneHoverEvent* ev)
{ QGraphicsItem::hoverEnterEvent(ev); m_hovered = true;  update(); }
void MindNode::hoverLeaveEvent(QGraphicsSceneHoverEvent* ev)
{ QGraphicsItem::hoverLeaveEvent(ev); m_hovered = false; update(); }

// ─── MindMapDialog ────────────────────────────────────────────────────────────

MindMapDialog::MindMapDialog(QSharedPointer<Database> db, QWidget* parent)
    : QDialog(parent), m_db(db)
{
    setWindowTitle(tr("🧠 Mapa Mental — CfdmNote"));
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
    const QRect scr = QApplication::primaryScreen()->availableGeometry();
    resize(qMin(1280, scr.width() - 40), qMin(820, scr.height() - 40));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
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
    m_chkEntries->setChecked(false);
    bar->addWidget(m_chkEntries);

    auto* hint = new QLabel(tr("   Clique num grupo para expandir / recolher"));
    hint->setStyleSheet("color:#90cdf4; font-size:11px; font-style:italic;");
    bar->addWidget(hint);
    root->addWidget(bar);

    // ── Área principal: canvas + painel lateral ───────────────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setChildrenCollapsible(false);

    m_scene = new QGraphicsScene(this);
    m_scene->setBackgroundBrush(QColor("#0d1b2e"));

    m_view = new MindView(m_scene);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setRenderHint(QPainter::SmoothPixmapTransform);
    m_view->setDragMode(QGraphicsView::ScrollHandDrag);
    m_view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    splitter->addWidget(m_view);

    // Painel lateral de detalhes
    auto* panel = new QWidget();
    panel->setFixedWidth(235);
    panel->setStyleSheet("QWidget{background:#f0f7ff;} QLabel{color:#1a365d;}");
    auto* pLay = new QVBoxLayout(panel);
    pLay->setContentsMargins(12, 12, 12, 12);
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
    for (const auto& s : {tr("▶ grupo recolhido — clique p/ abrir"),
                           tr("▼ grupo expandido — clique p/ fechar"),
                           tr("🖱 Scroll = zoom"),
                           tr("🖐 Arrastar = mover canvas")}) {
        auto* l = new QLabel(s);
        l->setStyleSheet("font-size:10px; color:#718096; padding:1px 0;");
        pLay->addWidget(l);
    }
    pLay->addSpacing(8);

    auto* btnClose = new QPushButton(tr("Fechar"));
    btnClose->setStyleSheet(
        "background:#e2e8f0; color:#1a365d; border-radius:5px; padding:8px;"
        "QPushButton:hover{background:#bee3f8;}");
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    pLay->addWidget(btnClose);

    splitter->addWidget(panel);
    splitter->setSizes({width() - 235, 235});
    root->addWidget(splitter, 1);

    // ── Conexões ──────────────────────────────────────────────────────────────
    connect(actFit, &QAction::triggered, this, &MindMapDialog::fitAll);

    connect(actExport, &QAction::triggered, this, [this]() {
        const QString p = QFileDialog::getSaveFileName(
            this, tr("Exportar Mapa Mental"), QDir::homePath(), tr("SVG (*.svg)"));
        if (p.isEmpty()) return;
        const QRectF b = m_scene->itemsBoundingRect().adjusted(-30, -30, 30, 30);
        QSvgGenerator gen;
        gen.setFileName(p);
        gen.setViewBox(b);
        QPainter pt(&gen);
        m_scene->render(&pt, {}, b);
        pt.end();
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
        render();
        QTimer::singleShot(40, this, &MindMapDialog::fitAll);
    });

    // Clique nos nós via propriedade da cena
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

    // Começa com grupos de nível 1 expandidos, sem entradas
    if (m_db)
        for (auto* g : m_db->rootGroup()->children())
            m_expanded.insert(g);

    render();
    QTimer::singleShot(120, this, &MindMapDialog::fitAll);
}

void MindMapDialog::showMap(QSharedPointer<Database> db, QWidget* parent)
{
    auto* dlg = new MindMapDialog(db, parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

// ─── Algoritmo de layout em árvore horizontal ─────────────────────────────────
//
//  Raiz à esquerda, ramos se expandem para a direita.
//  Cada nó recebe uma fatia vertical proporcional ao seu sub-projeto:
//
//    subtreeH(nó) = soma de subtreeH(filhos)   se expandido
//                 = NODE_H + V_GAP             se recolhido / folha
//
//  Isso garante zero sobreposição em qualquer nível de profundidade.

double MindMapDialog::subtreeH(Group* g) const
{
    // Nó recolhido ou sem filhos visíveis → ocupa apenas sua própria altura
    if (!m_expanded.contains(g))
        return MM_NODE_HG + MM_V_GAP;

    const bool showEntries = m_chkEntries->isChecked();

    double total = 0;
    for (auto* child : g->children()) {
        if (m_db && child == m_db->metadata()->recycleBin()) continue;
        total += subtreeH(child);
    }
    if (showEntries)
        total += g->entries().size() * (MM_NODE_HE + MM_V_GAP);

    // Expandido mas vazio → altura mínima para o próprio nó
    return total > 0 ? total : (MM_NODE_HG + MM_V_GAP);
}

void MindMapDialog::placeGroup(Group* g, double cx, double cy,
                               int depth, const QColor& color)
{
    if (m_db && g == m_db->metadata()->recycleBin()) return;

    const bool showEntries = m_chkEntries->isChecked();

    // Filhos visíveis (sem lixeira)
    QList<Group*> subGroups;
    for (auto* child : g->children()) {
        if (m_db && child == m_db->metadata()->recycleBin()) continue;
        subGroups << child;
    }
    const bool hasKids = !subGroups.isEmpty()
                      || (showEntries && !g->entries().isEmpty());
    const bool isExp   = m_expanded.contains(g);

    // ── Coloca o nó deste grupo ───────────────────────────────────────────────
    auto* node = new MindNode(MindNode::Kind::Group, g->name(), color,
                              g, nullptr, hasKids, isExp);
    node->setPos(cx, cy);
    m_scene->addItem(node);
    m_groupPos[g] = {cx, cy};

    if (!isExp) return;

    // ── Distribui filhos verticalmente ao redor do centro deste nó ───────────
    double totalH = 0;
    for (auto* child : subGroups) totalH += subtreeH(child);
    if (showEntries) totalH += g->entries().size() * (MM_NODE_HE + MM_V_GAP);
    if (totalH < 1.0) return;

    // X dos filhos = borda direita deste nó + gap + metade da largura do filho
    const double childCX = cx + MM_NODE_W / 2.0 + MM_H_GAP + MM_NODE_W / 2.0;

    // Começa no topo da fatia vertical deste nó
    double curY = cy - totalH / 2.0;

    // Sub-grupos
    for (auto* child : subGroups) {
        const double h       = subtreeH(child);
        const double childCY = curY + h / 2.0;

        // Curva bezier horizontal: borda-direita deste → borda-esquerda do filho
        addCurve({cx + MM_NODE_W / 2.0, cy},
                 {childCX - MM_NODE_W / 2.0, childCY},
                 color, depth <= 1 ? 3 : 2);

        placeGroup(child, childCX, childCY, depth + 1, color);
        curY += h;
    }

    // Entradas (folhas finais)
    if (showEntries) {
        const QColor eColor = color;
        for (auto* e : g->entries()) {
            const double entryCY = curY + (MM_NODE_HE + MM_V_GAP) / 2.0;

            addCurve({cx + MM_NODE_W / 2.0, cy},
                     {childCX - MM_NODE_W / 2.0, entryCY},
                     eColor.lighter(160), 1);

            auto* eNode = new MindNode(MindNode::Kind::Entry, e->title(),
                                       eColor, nullptr, e, false, false);
            eNode->setPos(childCX, entryCY);
            m_scene->addItem(eNode);
            curY += MM_NODE_HE + MM_V_GAP;
        }
    }
}

void MindMapDialog::render()
{
    m_scene->clear();
    m_groupPos.clear();
    if (!m_db) return;

    Group* rootGrp = m_db->rootGroup();
    const QString dbName = m_db->metadata()->name().isEmpty()
                           ? tr("Banco") : m_db->metadata()->name();

    // ── Nó raiz (círculo central) ─────────────────────────────────────────────
    auto* rootNode = new MindNode(MindNode::Kind::Root, dbName,
                                  QColor("#2c5282"), rootGrp);
    rootNode->setPos(0, 0);
    m_scene->addItem(rootNode);
    m_groupPos[rootGrp] = {0, 0};

    // Filtra lixeira dos grupos de nível 1
    QList<Group*> topGroups;
    for (auto* g : rootGrp->children()) {
        if (g == m_db->metadata()->recycleBin()) continue;
        topGroups << g;
    }
    if (topGroups.isEmpty()) return;

    // Altura total ocupada por todos os grupos de nível 1
    double totalH = 0;
    for (auto* g : topGroups) totalH += subtreeH(g);

    // X do centro dos grupos de nível 1: borda direita da raiz + gap + meia largura
    const double lvl1X  = MM_ROOT_R + MM_H_GAP + MM_NODE_W / 2.0;
    double curY = -totalH / 2.0;

    for (int i = 0; i < topGroups.size(); ++i) {
        auto* g = topGroups[i];
        const QColor color = COLORS[i % COLORS.size()];
        const double h     = subtreeH(g);
        const double groupCY = curY + h / 2.0;

        // Curva do nó raiz até este grupo
        addCurve({MM_ROOT_R, 0.0},
                 {lvl1X - MM_NODE_W / 2.0, groupCY},
                 color, 3);

        placeGroup(g, lvl1X, groupCY, 1, color);
        curY += h;
    }
}

void MindMapDialog::addCurve(QPointF from, QPointF to,
                              const QColor& color, int w)
{
    // Bezier horizontal suave: pontos de controle no meio horizontal
    const double dx = (to.x() - from.x()) * 0.55;
    QPainterPath path;
    path.moveTo(from);
    path.cubicTo(from.x() + dx, from.y(),
                 to.x()   - dx, to.y(),
                 to.x(), to.y());

    auto* item = m_scene->addPath(path,
                                  QPen(color, w, Qt::SolidLine,
                                       Qt::RoundCap, Qt::RoundJoin));
    item->setZValue(0);
}

void MindMapDialog::fitAll()
{
    m_view->fitInView(
        m_scene->itemsBoundingRect().adjusted(-50, -50, 50, 50),
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
    m_detailSub->setText(tr("%1 entradas · %2 total").arg(direct).arg(total));
    m_detailBody->setText(tr("%1 subgrupo(s)").arg(g->children().size()));
}
