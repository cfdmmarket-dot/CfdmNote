#include "RelatedEntriesWidget.h"

#include <QScrollArea>
#include <QHBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QMenu>
#include <QInputDialog>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QUuid>
#include <QLineEdit>

#include "core/Group.h"
#include "core/EntryAttributes.h"

static constexpr const char* ATTR_RELATED = "CfdmNote::RelatedEntries";

// ─── Helpers ──────────────────────────────────────────────────────────────────

/*static*/
QString RelatedEntriesWidget::domainOf(const QString& url)
{
    QString h = QUrl(url).host().toLower();
    if (h.startsWith("www.")) h = h.mid(4);
    return h;
}

// ─── Constructor ──────────────────────────────────────────────────────────────

RelatedEntriesWidget::RelatedEntriesWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    // Scroll area englobando tudo
    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* container = new QWidget();
    auto* containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(6, 6, 6, 6);
    containerLayout->setSpacing(8);

    // ── Seção: mesmo domínio (auto) ──────────────────────────────────────────
    m_autoGroup = new QGroupBox(tr("🌐 Mesmo domínio"));
    m_autoGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; color: #2c5282; border: 1px solid #bee3f8;"
        "border-radius: 6px; margin-top: 8px; padding-top: 4px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }");
    m_autoContent = new QVBoxLayout(m_autoGroup);
    m_autoContent->setSpacing(4);
    m_autoContent->setContentsMargins(6, 4, 6, 6);
    containerLayout->addWidget(m_autoGroup);

    // ── Seção: referências manuais ───────────────────────────────────────────
    m_manualGroup = new QGroupBox(tr("📌 Referências Manuais"));
    m_manualGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; color: #276749; border: 1px solid #9ae6b4;"
        "border-radius: 6px; margin-top: 8px; padding-top: 4px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }");
    m_manualContent = new QVBoxLayout(m_manualGroup);
    m_manualContent->setSpacing(4);
    m_manualContent->setContentsMargins(6, 4, 6, 6);

    // Botão "+ Adicionar referência"
    auto* btnAdd = new QPushButton(tr("  ➕  Adicionar referência cruzada..."));
    btnAdd->setFixedHeight(28);
    btnAdd->setStyleSheet(
        "QPushButton { border: 1px dashed #38a169; border-radius: 4px;"
        "color: #276749; font-size: 11px; }"
        "QPushButton:hover { background: #f0fff4; }");
    connect(btnAdd, &QPushButton::clicked, this, &RelatedEntriesWidget::addManualLink);
    m_manualContent->addWidget(btnAdd);

    containerLayout->addWidget(m_manualGroup);
    containerLayout->addStretch();

    scroll->setWidget(container);
    outerLayout->addWidget(scroll);

    // Placeholder inicial
    auto* hint = new QLabel(tr(
        "<span style='color:#718096;font-size:11px'>"
        "Selecione uma entrada para ver referências relacionadas.</span>"));
    hint->setAlignment(Qt::AlignCenter);
    hint->setWordWrap(true);
    outerLayout->addWidget(hint);

    m_autoGroup->setVisible(false);
    m_manualGroup->setVisible(false);
    scroll->setVisible(false);

    // Guarda referência ao hint para ocultar depois
    hint->setObjectName("relHint");
}

// ─── API pública ──────────────────────────────────────────────────────────────

void RelatedEntriesWidget::setEntry(Entry* entry, QSharedPointer<Database> db)
{
    m_entry = entry;
    m_db    = db;
    refresh();
}

void RelatedEntriesWidget::clear()
{
    m_entry = nullptr;
    m_db.reset();
    refresh();
}

// ─── refresh ──────────────────────────────────────────────────────────────────

void RelatedEntriesWidget::refresh()
{
    // Oculta/mostra scroll vs hint
    auto* hint   = findChild<QLabel*>("relHint");
    auto* scroll = findChild<QScrollArea*>();

    if (!m_entry || !m_db) {
        if (m_autoGroup)   m_autoGroup->setVisible(false);
        if (m_manualGroup) m_manualGroup->setVisible(false);
        if (hint)   hint->setVisible(true);
        if (scroll) scroll->setVisible(false);
        return;
    }

    if (hint)   hint->setVisible(false);
    if (scroll) scroll->setVisible(true);

    buildAutoSection();
    buildManualSection();

    m_autoGroup->setVisible(true);
    m_manualGroup->setVisible(true);
}

// ─── Auto section (mesmo domínio) ────────────────────────────────────────────

void RelatedEntriesWidget::buildAutoSection()
{
    // Limpa conteúdo anterior (exceto possíveis layouts)
    QLayoutItem* item;
    while ((item = m_autoContent->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    const QString myDomain = domainOf(m_entry->url());
    const QString myUuid   = m_entry->uuidToHex();

    QList<Entry*> found;
    if (!myDomain.isEmpty() && m_db) {
        const auto allEntries = m_db->rootGroup()->entriesRecursive();
        for (auto* e : allEntries) {
            if (e->uuidToHex() == myUuid) continue;
            if (domainOf(e->url()) == myDomain)
                found << e;
        }
    }

    if (found.isEmpty()) {
        auto* lbl = new QLabel(tr(
            "<span style='color:#a0aec0;font-size:11px'>"
            "Nenhuma entrada com o mesmo domínio encontrada.</span>"));
        lbl->setWordWrap(true);
        m_autoContent->addWidget(lbl);
    } else {
        m_autoGroup->setTitle(
            tr("🌐 Mesmo domínio — %1  (%2 entrada(s))")
            .arg(myDomain).arg(found.size()));
        for (auto* e : found)
            m_autoContent->addWidget(makeEntryButton(e, false));
    }
}

// ─── Manual section ───────────────────────────────────────────────────────────

void RelatedEntriesWidget::buildManualSection()
{
    // Limpa tudo exceto o botão "Adicionar" (último item)
    while (m_manualContent->count() > 1) {
        auto* item = m_manualContent->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    const QStringList uuids = manualUuids();
    if (uuids.isEmpty()) return;

    for (const QString& uuid : uuids) {
        if (!m_db) continue;
        Entry* e = m_db->rootGroup()->findEntryByUuid(QUuid::fromString(uuid));
        if (!e) continue;
        m_manualContent->insertWidget(m_manualContent->count() - 1,
                                      makeEntryButton(e, true));
    }
}

// ─── makeEntryButton ──────────────────────────────────────────────────────────

QPushButton* RelatedEntriesWidget::makeEntryButton(Entry* e, bool isManual)
{
    // Caminho do grupo: Raiz > Subgrupo > ...
    QStringList groupPath;
    Group* g = e->group();
    while (g) {
        groupPath.prepend(g->name());
        g = g->parentGroup();
    }
    const QString path = groupPath.join(" › ");

    auto* btn = new QPushButton();
    btn->setFixedHeight(32);
    btn->setStyleSheet(
        "QPushButton { border: 1px solid #e2e8f0; border-radius: 4px;"
        "text-align: left; padding: 0 8px; font-size: 11px; }"
        "QPushButton:hover { background: #ebf8ff; border-color: #90cdf4; }");

    btn->setIcon(QIcon::fromTheme("dialog-password"));

    btn->setText(QString("  %1\n  <span style='color:#718096'>%2</span>")
                 .arg(e->title().toHtmlEscaped(), path));

    // Tooltip com detalhes
    btn->setToolTip(QString(
        "<b>%1</b><br>"
        "<span style='color:#a0aec0'>Grupo: %2</span><br>"
        "<span style='color:#a0aec0'>URL: %3</span><br><br>"
        "<i>Clique para ver detalhes</i>")
        .arg(e->title().toHtmlEscaped(),
             path,
             e->url().toHtmlEscaped()));

    // Usa texto simples (sem HTML no QPushButton diretamente)
    btn->setText(QString("  %1  —  %2").arg(e->title(), path));

    connect(btn, &QPushButton::clicked, this,
            [this, e, isManual]() { showEntryPopup(e, isManual); });

    return btn;
}

// ─── showEntryPopup ───────────────────────────────────────────────────────────

void RelatedEntriesWidget::showEntryPopup(Entry* e, bool isManual)
{
    if (!e) return;

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Entrada relacionada"));
    dlg.setMinimumWidth(420);
    auto* layout = new QVBoxLayout(&dlg);
    layout->setSpacing(8);

    // Cabeçalho
    auto* titleLbl = new QLabel(
        QString("<b style='font-size:14px'>%1</b>").arg(e->title().toHtmlEscaped()));
    titleLbl->setTextFormat(Qt::RichText);
    layout->addWidget(titleLbl);

    auto* line = new QFrame(); line->setFrameShape(QFrame::HLine);
    layout->addWidget(line);

    // Detalhes
    auto makeRow = [&](const QString& label, const QString& value) {
        if (value.isEmpty()) return;
        auto* row = new QHBoxLayout();
        auto* lbl = new QLabel("<b>" + label + "</b>");
        lbl->setFixedWidth(90);
        lbl->setAlignment(Qt::AlignTop | Qt::AlignRight);
        auto* val = new QLabel(value.toHtmlEscaped());
        val->setTextInteractionFlags(Qt::TextSelectableByMouse);
        val->setWordWrap(true);
        row->addWidget(lbl);
        row->addWidget(val, 1);
        layout->addLayout(row);
    };

    // Grupo
    QStringList groupPath;
    Group* g = e->group();
    while (g) { groupPath.prepend(g->name()); g = g->parentGroup(); }
    makeRow(tr("Grupo:"),    groupPath.join(" › "));
    makeRow(tr("Usuário:"),  e->username());
    makeRow(tr("URL:"),      e->url());
    makeRow(tr("Tags:"),     e->tags());

    const QString comment = e->attributes()->value("CfdmNote::Comment");
    if (!comment.isEmpty())
        makeRow(tr("Comentário:"), comment);

    layout->addWidget(new QFrame()); // espaço

    // Botões de ação
    auto* btnRow = new QHBoxLayout();

    auto* btnCopyTitle = new QPushButton(tr("📋 Copiar título"));
    btnCopyTitle->setFixedHeight(28);
    connect(btnCopyTitle, &QPushButton::clicked, &dlg, [e]() {
        QApplication::clipboard()->setText(e->title());
    });

    auto* btnCopyUser = new QPushButton(tr("👤 Copiar usuário"));
    btnCopyUser->setFixedHeight(28);
    btnCopyUser->setEnabled(!e->username().isEmpty());
    connect(btnCopyUser, &QPushButton::clicked, &dlg, [e]() {
        QApplication::clipboard()->setText(e->username());
    });

    auto* btnOpenUrl = new QPushButton(tr("🔗 Abrir URL"));
    btnOpenUrl->setFixedHeight(28);
    btnOpenUrl->setEnabled(!e->url().isEmpty());
    connect(btnOpenUrl, &QPushButton::clicked, &dlg, [e]() {
        QDesktopServices::openUrl(QUrl(e->url()));
    });

    auto* btnNavigate = new QPushButton(tr("🎯 Ir para entrada"));
    btnNavigate->setFixedHeight(28);
    btnNavigate->setStyleSheet(
        "QPushButton { background: #3182ce; color: white; border-radius: 4px; "
        "font-weight: bold; padding: 0 12px; }"
        "QPushButton:hover { background: #2c5282; }");
    connect(btnNavigate, &QPushButton::clicked, &dlg, [this, e, &dlg]() {
        emit navigateRequested(e->uuidToHex());
        dlg.accept();
    });

    btnRow->addWidget(btnCopyTitle);
    btnRow->addWidget(btnCopyUser);
    btnRow->addWidget(btnOpenUrl);
    btnRow->addStretch();
    btnRow->addWidget(btnNavigate);
    layout->addLayout(btnRow);

    // Remover referência manual (só para manuais)
    if (isManual) {
        auto* btnRemove = new QPushButton(tr("🗑 Remover esta referência"));
        btnRemove->setFixedHeight(26);
        btnRemove->setStyleSheet("color: #e53e3e; font-size: 11px;");
        connect(btnRemove, &QPushButton::clicked, &dlg,
                [this, e, &dlg]() {
                    removeManualLink(e->uuidToHex());
                    dlg.accept();
                });
        layout->addWidget(btnRemove);
    }

    auto* btnClose = new QPushButton(tr("Fechar"));
    btnClose->setFixedHeight(28);
    connect(btnClose, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout->addWidget(btnClose);

    dlg.exec();
}

// ─── Manual refs CRUD ─────────────────────────────────────────────────────────

QStringList RelatedEntriesWidget::manualUuids() const
{
    if (!m_entry) return {};
    const QString raw = m_entry->attributes()->value(ATTR_RELATED);
    if (raw.trimmed().isEmpty()) return {};
    return raw.split(',', Qt::SkipEmptyParts);
}

void RelatedEntriesWidget::saveManualUuids(const QStringList& uuids)
{
    if (!m_entry || !m_db) return;
    m_entry->attributes()->set(ATTR_RELATED, uuids.join(','), false);
    QString err;
    m_db->save(Database::Atomic, {}, &err);
}

void RelatedEntriesWidget::addManualLink()
{
    if (!m_entry || !m_db) return;

    // Coleta todas as entradas (exceto a atual) ordenadas por título
    const auto allEntries = m_db->rootGroup()->entriesRecursive();
    QList<Entry*> others;
    for (auto* e : allEntries)
        if (e->uuidToHex() != m_entry->uuidToHex())
            others << e;

    if (others.isEmpty()) {
        return;
    }

    // Menu de seleção rápida
    auto* menu = new QMenu(this);
    menu->setTitle(tr("Adicionar referência cruzada"));

    // Agrupa por grupo pai
    QMap<QString, QList<Entry*>> byGroup;
    for (auto* e : others) {
        const QString grpName = e->group() ? e->group()->name() : tr("Raiz");
        byGroup[grpName] << e;
    }

    const QStringList existingUuids = manualUuids();

    for (auto it = byGroup.begin(); it != byGroup.end(); ++it) {
        auto* sub = menu->addMenu(QIcon::fromTheme("folder"), it.key());
        for (auto* e : it.value()) {
            const bool already = existingUuids.contains(e->uuidToHex());
            auto* act = sub->addAction(
                QIcon::fromTheme(already ? "dialog-ok" : "dialog-password"),
                e->title() + (already ? tr("  ✓ já adicionado") : ""));
            act->setEnabled(!already);
            const QString uuid = e->uuidToHex();
            connect(act, &QAction::triggered, this, [this, uuid]() {
                QStringList uuids = manualUuids();
                if (!uuids.contains(uuid)) {
                    uuids << uuid;
                    saveManualUuids(uuids);
                    buildManualSection();
                }
            });
        }
    }

    menu->exec(QCursor::pos());
    menu->deleteLater();
}

void RelatedEntriesWidget::removeManualLink(const QString& uuidHex)
{
    QStringList uuids = manualUuids();
    uuids.removeAll(uuidHex);
    saveManualUuids(uuids);
    buildManualSection();
}
