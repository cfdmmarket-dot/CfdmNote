#include "RoadmapDialog.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QAction>
#include <QFrame>
#include <QApplication>
#include <QScreen>
#include <QMessageBox>

static const QString ATTR_STATUS = "CfdmNote::RoadmapStatus";

// ── Widget de card individual ─────────────────────────────────────────────────
static QWidget* makeCard(Entry* e, const QString& colColor)
{
    auto* card = new QFrame();
    card->setObjectName("roadmapCard");
    card->setStyleSheet(
        "QFrame#roadmapCard { background: white; border-radius: 8px; "
        "border-left: 4px solid " + colColor + "; "
        "border-top: 1px solid #e2e8f0; border-right: 1px solid #e2e8f0; "
        "border-bottom: 1px solid #e2e8f0; margin: 3px; }"
        "QFrame#roadmapCard:hover { background: #f7faff; }");
    card->setMinimumHeight(72);
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(10,8,10,8);
    lay->setSpacing(3);

    auto* titleLbl = new QLabel(e->title().left(40) + (e->title().length() > 40 ? "…" : ""));
    titleLbl->setStyleSheet("font-weight: bold; font-size: 12px; color: #1a365d;");
    titleLbl->setWordWrap(true);
    lay->addWidget(titleLbl);

    if (e->group()) {
        auto* groupLbl = new QLabel("📁 " + e->group()->name());
        groupLbl->setStyleSheet("font-size: 10px; color: #4a5568;");
        lay->addWidget(groupLbl);
    }

    if (!e->tags().isEmpty()) {
        auto* tagLbl = new QLabel("🏷️ " + e->tags());
        tagLbl->setStyleSheet("font-size: 10px; color: #276749; background: #c6f6d5; "
                              "border-radius: 3px; padding: 1px 4px;");
        tagLbl->setWordWrap(true);
        lay->addWidget(tagLbl);
    }

    const QString comment = e->attributes()->value("CfdmNote::Comment");
    if (!comment.isEmpty()) {
        auto* commLbl = new QLabel("💬 " + comment.left(50));
        commLbl->setStyleSheet("font-size: 10px; color: #553c9a;");
        commLbl->setWordWrap(true);
        lay->addWidget(commLbl);
    }
    return card;
}

// ─── RoadmapDialog ────────────────────────────────────────────────────────────

RoadmapDialog::RoadmapDialog(QSharedPointer<Database> db, QWidget* parent)
    : QDialog(parent), m_db(db)
{
    setWindowTitle(tr("🗺️ Roadmap / Kanban — CfdmNote"));
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
    const QRect scr = QApplication::primaryScreen()->availableGeometry();
    resize(qMin(1100, scr.width()-60), qMin(720, scr.height()-60));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(0);

    // ── Cabeçalho ─────────────────────────────────────────────────────────────
    auto* header = new QWidget();
    header->setStyleSheet("QWidget { background: #1a365d; } QLabel { color: #bee3f8; }");
    auto* hLay = new QHBoxLayout(header);
    hLay->setContentsMargins(16,10,16,10);

    auto* hTitle = new QLabel("🗺️  Roadmap / Kanban de Entradas");
    hTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #ebf8ff;");
    hLay->addWidget(hTitle);
    hLay->addStretch();

    // Filtro por grupo
    auto* lblG = new QLabel(tr("Grupo: "));
    hLay->addWidget(lblG);
    auto* cmbGroup = new QComboBox();
    cmbGroup->setFixedWidth(160);
    cmbGroup->setStyleSheet("QComboBox { background: #2c5282; color: white; border-radius: 4px; padding: 4px 8px; }");
    cmbGroup->addItem(tr("Todos"));
    if (db) {
        for (auto* g : db->rootGroup()->groupsRecursive(false))
            cmbGroup->addItem(g->name());
    }
    hLay->addWidget(cmbGroup);

    auto* btnClose = new QPushButton(tr("Fechar"));
    btnClose->setStyleSheet("QPushButton { background: #3182ce; color: white; border-radius: 4px; padding: 6px 16px; } "
                            "QPushButton:hover { background: #4299e1; }");
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    hLay->addWidget(btnClose);
    root->addWidget(header);

    // ── Info instrução ─────────────────────────────────────────────────────────
    auto* infoBar = new QLabel(tr("  💡 Clique com botão direito em qualquer card para mover entre colunas   |   "
                                  "Duplo clique para ver detalhes da entrada"));
    infoBar->setStyleSheet("background: #ebf8ff; color: #2c5282; font-size: 11px; "
                           "padding: 6px 16px; border-bottom: 1px solid #bee3f8;");
    root->addWidget(infoBar);

    // ── Colunas Kanban ─────────────────────────────────────────────────────────
    auto* kanbanArea = new QWidget();
    kanbanArea->setStyleSheet("QWidget { background: #e8f0f8; }");
    auto* kanbanLay = new QHBoxLayout(kanbanArea);
    kanbanLay->setContentsMargins(12,12,12,12);
    kanbanLay->setSpacing(12);

    const QList<std::tuple<Status,QString,QString>> colDefs = {
        { Status::Backlog,    tr("📋  Backlog"),       "#4a5568" },
        { Status::InProgress, tr("🔄  Em Progresso"),  "#3182ce" },
        { Status::Done,       tr("✅  Concluído"),      "#38a169" },
        { Status::Waiting,    tr("⏸  Aguardando"),     "#dd6b20" },
    };

    for (auto& [status, title, color] : colDefs) {
        auto* colWidget = new QFrame();
        colWidget->setStyleSheet(
            "QFrame { background: white; border-radius: 10px; "
            "border-top: 4px solid " + color + "; }");
        colWidget->setMinimumWidth(220);
        auto* colLay = new QVBoxLayout(colWidget);
        colLay->setContentsMargins(8,8,8,8);
        colLay->setSpacing(6);

        // Cabeçalho da coluna
        auto* colHead = new QHBoxLayout();
        auto* colTitle = new QLabel(title);
        colTitle->setStyleSheet(QString("font-weight: bold; font-size: 12px; color: %1;").arg(color));
        auto* countLbl = new QLabel("0");
        countLbl->setStyleSheet(QString("background: %1; color: white; border-radius: 10px; "
                                        "padding: 1px 7px; font-size: 11px; font-weight: bold;").arg(color));
        colHead->addWidget(colTitle);
        colHead->addStretch();
        colHead->addWidget(countLbl);
        colLay->addLayout(colHead);

        auto* list = new QListWidget();
        list->setStyleSheet(
            "QListWidget { border: none; background: transparent; }"
            "QListWidget::item { background: transparent; border: none; padding: 0; }");
        list->setSpacing(2);
        list->setDragDropMode(QAbstractItemView::NoDragDrop);

        const Status capStatus = status;
        const QString capColor = color;
        list->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(list, &QListWidget::customContextMenuRequested, this,
                [this, capStatus](const QPoint& pos) { onContextMenu(pos, capStatus); });
        connect(list, &QListWidget::itemDoubleClicked, this, &RoadmapDialog::onCardDoubleClicked);

        colLay->addWidget(list, 1);

        m_columns << Column{status, title, color, list, countLbl};
        kanbanLay->addWidget(colWidget, 1);
    }

    auto* scrollArea = new QScrollArea();
    scrollArea->setWidget(kanbanArea);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    root->addWidget(scrollArea, 1);

    // Conexão filtro grupo
    connect(cmbGroup, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this, cmbGroup](int idx) {
                filterByGroup(idx == 0 ? QString() : cmbGroup->currentText());
            });

    loadEntries();
}

void RoadmapDialog::showRoadmap(QSharedPointer<Database> db, QWidget* parent)
{
    auto* dlg = new RoadmapDialog(db, parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

void RoadmapDialog::loadEntries()
{
    for (auto& col : m_columns) col.list->clear();

    if (!m_db) return;

    for (auto* e : m_db->rootGroup()->entriesRecursive())
        addCard(e, readStatus(e));

    for (auto& col : m_columns)
        col.countLabel->setText(QString::number(col.list->count()));
}

void RoadmapDialog::addCard(Entry* e, Status s)
{
    for (auto& col : m_columns) {
        if (col.status != s) continue;
        auto* item = new QListWidgetItem(col.list);
        auto* w = makeCard(e, col.color);
        item->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(e)));
        item->setSizeHint(w->sizeHint());
        col.list->setItemWidget(item, w);
        break;
    }
}

void RoadmapDialog::onContextMenu(const QPoint& pos, Status currentCol)
{
    // Descobre qual coluna gerou o evento
    QListWidget* srcList = nullptr;
    for (auto& col : m_columns)
        if (col.status == currentCol) { srcList = col.list; break; }
    if (!srcList) return;

    auto* item = srcList->itemAt(pos);
    if (!item) return;
    auto* entry = static_cast<Entry*>(item->data(Qt::UserRole).value<void*>());
    if (!entry) return;

    QMenu menu(this);
    menu.setTitle(tr("Mover para…"));
    menu.setStyleSheet("QMenu { background: #1e3a5f; color: #e2e8f0; border: 1px solid #3182ce; "
                       "border-radius: 6px; padding: 4px 0; }"
                       "QMenu::item { padding: 6px 20px; } "
                       "QMenu::item:selected { background: #3182ce; }");

    for (auto& col : m_columns) {
        if (col.status == currentCol) continue;
        auto* act = menu.addAction(col.title);
        const Status toStatus = col.status;
        connect(act, &QAction::triggered, this, [this, entry, toStatus]() {
            onMoveCard(entry, toStatus);
        });
    }
    menu.exec(srcList->viewport()->mapToGlobal(pos));
}

void RoadmapDialog::onMoveCard(Entry* entry, Status to)
{
    saveStatus(entry, to);
    loadEntries();
}

void RoadmapDialog::onCardDoubleClicked(QListWidgetItem* item)
{
    if (!item) return;
    auto* e = static_cast<Entry*>(item->data(Qt::UserRole).value<void*>());
    if (!e) return;

    QMessageBox box(this);
    box.setWindowTitle(e->title());
    box.setText(QString(
        "<b>%1</b><br><br>"
        "👤 Usuário: %2<br>"
        "🔗 URL: %3<br>"
        "🏷️ Tags: %4<br>"
        "📁 Grupo: %5<br>"
        "🗺️ Status: %6"
    ).arg(e->title().toHtmlEscaped())
     .arg(e->username().toHtmlEscaped())
     .arg(e->url().toHtmlEscaped())
     .arg(e->tags().toHtmlEscaped())
     .arg(e->group() ? e->group()->name().toHtmlEscaped() : tr("—"))
     .arg(statusLabel(readStatus(e)).toHtmlEscaped()));
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
}

void RoadmapDialog::filterByGroup(const QString& group)
{
    for (auto& col : m_columns) {
        for (int i = 0; i < col.list->count(); ++i) {
            auto* item = col.list->item(i);
            auto* e = static_cast<Entry*>(item->data(Qt::UserRole).value<void*>());
            const bool show = group.isEmpty() || (e && e->group() && e->group()->name() == group);
            item->setHidden(!show);
        }
    }
}

void RoadmapDialog::saveStatus(Entry* e, Status s)
{
    if (!m_db || !e) return;
    e->beginUpdate();
    e->attributes()->set(ATTR_STATUS, statusKey(s), true);
    e->endUpdate();
    QString err;
    m_db->save(Database::Atomic, {}, &err);
}

RoadmapDialog::Status RoadmapDialog::readStatus(Entry* e) const
{
    const QString v = e->attributes()->value(ATTR_STATUS);
    if (v == "in_progress") return Status::InProgress;
    if (v == "done")        return Status::Done;
    if (v == "waiting")     return Status::Waiting;
    return Status::Backlog;
}

QString RoadmapDialog::statusLabel(Status s) const
{
    switch (s) {
        case Status::Backlog:    return tr("📋 Backlog");
        case Status::InProgress: return tr("🔄 Em Progresso");
        case Status::Done:       return tr("✅ Concluído");
        case Status::Waiting:    return tr("⏸ Aguardando");
    }
    return {};
}

QString RoadmapDialog::statusKey(Status s) const
{
    switch (s) {
        case Status::Backlog:    return "backlog";
        case Status::InProgress: return "in_progress";
        case Status::Done:       return "done";
        case Status::Waiting:    return "waiting";
    }
    return "backlog";
}
