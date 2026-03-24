#pragma once
#include <QDialog>
#include <QSharedPointer>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QWebEngineView>

class Database;
class Entry;
class Group;

class MindMapDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MindMapDialog(QSharedPointer<Database> db, QWidget* parent = nullptr);
    static void showMap(QSharedPointer<Database> db, QWidget* parent);

private:
    QString buildJson() const;
    QString groupToJson(Group* g, int colorIdx, int depth, const QString& prefix) const;
    void    injectMap();
    void    updateDetail(const QString& id, const QString& topic, const QString& type);

    QSharedPointer<Database> m_db;
    QWebEngineView* m_webView;
    QCheckBox*      m_chkEntries;
    QLabel*         m_detailTitle;
    QLabel*         m_detailSub;
    QLabel*         m_detailBody;
    QLabel*         m_detailPath;
    bool            m_mapLoaded = false;
    QString         m_tmpHtmlPath;
};
