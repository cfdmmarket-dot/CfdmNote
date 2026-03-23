#pragma once
#include <QDialog>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QSharedPointer>

class Database;
class Entry;
class QListWidget;
class QLabel;

// ── Item arrastável do Whiteboard ─────────────────────────────────────────────
class WbEntryCard : public QGraphicsItem
{
public:
    explicit WbEntryCard(Entry* entry, const QColor& color, QGraphicsItem* parent = nullptr);
    Entry* entry() const { return m_entry; }
    QString id() const;

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent*) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent*) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*) override;

private:
    Entry*  m_entry;
    QColor  m_color;
    bool    m_hovered = false;
};

// ── Sticky note arrastável ────────────────────────────────────────────────────
class WbStickyNote : public QGraphicsItem
{
public:
    explicit WbStickyNote(const QString& text, const QColor& bg, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;
    void setText(const QString& t) { m_text = t; update(); }
    QString text() const { return m_text; }
    QColor bg() const { return m_bg; }
    QString id() const { return m_id; }

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent*) override;

private:
    QString m_text;
    QColor  m_bg;
    QString m_id;
};

// ── Dialog principal ──────────────────────────────────────────────────────────
class WhiteboardDialog : public QDialog
{
    Q_OBJECT
public:
    explicit WhiteboardDialog(QSharedPointer<Database> db, QWidget* parent = nullptr);
    static void showBoard(QSharedPointer<Database> db, QWidget* parent);

private slots:
    void addStickyNote(const QColor& color);
    void addEntryToBoard();
    void clearBoard();
    void saveLayout();
    void loadLayout();

private:
    QSharedPointer<Database> m_db;
    QGraphicsScene*  m_scene;
    QGraphicsView*   m_view;
    QListWidget*     m_entryList;   // painel de entradas disponíveis
    int              m_nextId = 0;

    static const QString METADATA_KEY;
};
