#pragma once
#include <QDialog>
#include <QSharedPointer>
#include <QListWidget>
#include <QLabel>
#include <QMap>

class Database;
class Entry;

class RoadmapDialog : public QDialog
{
    Q_OBJECT
public:
    enum class Status { Backlog, InProgress, Done, Waiting };

    explicit RoadmapDialog(QSharedPointer<Database> db, QWidget* parent = nullptr);
    static void showRoadmap(QSharedPointer<Database> db, QWidget* parent);

private slots:
    void onMoveCard(Entry* entry, Status to);
    void onContextMenu(const QPoint& pos, Status col);
    void onCardDoubleClicked(QListWidgetItem* item);
    void filterByGroup(const QString& group);

private:
    void loadEntries();
    void addCard(Entry* e, Status s);
    void saveStatus(Entry* e, Status s);
    Status readStatus(Entry* e) const;
    QString statusLabel(Status s) const;
    QString statusKey(Status s) const;

    QSharedPointer<Database> m_db;

    struct Column {
        Status       status;
        QString      title;
        QString      color;
        QListWidget* list;
        QLabel*      countLabel;
    };
    QList<Column> m_columns;
};
