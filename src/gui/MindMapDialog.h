#pragma once
#include <QDialog>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QSharedPointer>
#include <QSet>
#include <QHash>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>

class Database;
class Entry;
class Group;

// ── Nó clicável ───────────────────────────────────────────────────────────────
class MindNode : public QGraphicsItem
{
public:
    enum class Kind { Root, Group, Entry };

    MindNode(Kind kind, const QString& label, const QColor& color,
             Group* group = nullptr, Entry* entry = nullptr,
             bool hasChildren = false, bool expanded = false,
             QGraphicsItem* parent = nullptr);

    Group*  group()    const { return m_group; }
    Entry*  entry()    const { return m_entry; }
    Kind    kind()     const { return m_kind; }
    bool    expanded() const { return m_expanded; }

    QRectF boundingRect() const override;
    void   paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* ev) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* ev)  override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* ev)  override;

private:
    Kind    m_kind;
    QString m_label;
    QColor  m_color;
    Group*  m_group       = nullptr;
    Entry*  m_entry       = nullptr;
    bool    m_hasChildren = false;
    bool    m_expanded    = false;
    bool    m_hovered     = false;
};

// ── Dialog principal ──────────────────────────────────────────────────────────
class MindMapDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MindMapDialog(QSharedPointer<Database> db, QWidget* parent = nullptr);
    static void showMap(QSharedPointer<Database> db, QWidget* parent);

private:
    void render();
    void renderGroup(Group* g, QPointF pos, double dirAngle,
                     double fanAngle, double radius, int depth,
                     const QColor& color);
    void addCurve(QPointF from, QPointF to, const QColor& color, int w);
    void fitAll();
    void updateDetail(Entry* e);
    void updateDetail(Group* g);

    QSharedPointer<Database> m_db;
    QSet<Group*>   m_expanded;   // grupos atualmente expandidos
    QHash<Group*, QPointF> m_groupPos; // posição de cada grupo no canvas

    QGraphicsScene* m_scene;
    QGraphicsView*  m_view;

    QCheckBox* m_chkEntries;

    // Painel lateral
    QLabel* m_detailTitle;
    QLabel* m_detailSub;
    QLabel* m_detailBody;
};
