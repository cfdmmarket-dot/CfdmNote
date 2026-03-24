#include "MergeGroupDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QMessageBox>
#include <QApplication>
#include <QScreen>

#include "core/Database.h"
#include "core/Group.h"
#include "core/Entry.h"
#include "gui/DatabaseTabWidget.h"
#include "gui/DatabaseWidget.h"

// ─── Construtor ───────────────────────────────────────────────────────────────

MergeGroupDialog::MergeGroupDialog(DatabaseTabWidget* tabWidget, QWidget* parent)
    : QDialog(parent)
    , m_tabWidget(tabWidget)
{
    setWindowTitle(tr("📥 Importar Grupo entre Bancos"));
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
    const QRect screen = QApplication::primaryScreen()->availableGeometry();
    resize(qMin(960, screen.width() - 60), qMin(620, screen.height() - 60));

    // ── Coleta todos os bancos abertos ────────────────────────────────────────
    if (m_tabWidget) {
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            auto* dbw = qobject_cast<DatabaseWidget*>(m_tabWidget->widget(i));
            if (!dbw || !dbw->database()) continue;
            m_openDbs   << dbw->database();
            m_openDbNames << m_tabWidget->tabText(i);
        }
    }

    // ── Layout ────────────────────────────────────────────────────────────────
    auto* root = new QVBoxLayout(this);
    root->setSpacing(8);
    root->setContentsMargins(12, 12, 12, 12);

    // Cabeçalho
    auto* hdr = new QLabel(tr(
        "<b>📥 Importar Grupo Completo entre Bancos</b><br>"
        "<small style='color:#4a5568'>"
        "Selecione o banco de origem, o grupo a copiar, o banco de destino e onde inserir. "
        "Subgrupos e entradas são copiados recursivamente — com novos IDs gerados automaticamente."
        "</small>"));
    hdr->setWordWrap(true);
    hdr->setStyleSheet("font-size:12px;color:#2c5282;padding:4px 0;");
    root->addWidget(hdr);

    // Linha de seletores de banco
    auto* dbRow = new QHBoxLayout();

    // Origem
    auto* srcDbBox = new QGroupBox(tr("Banco de origem"));
    srcDbBox->setStyleSheet("QGroupBox{font-size:11px;font-weight:bold;}");
    auto* srcDbLay = new QVBoxLayout(srcDbBox);
    m_srcDbCombo = new QComboBox();
    m_srcDbCombo->setStyleSheet("font-size:11px;");
    for (const QString& n : m_openDbNames)
        m_srcDbCombo->addItem(QIcon::fromTheme("database"), n);
    srcDbLay->addWidget(m_srcDbCombo);
    dbRow->addWidget(srcDbBox);

    dbRow->addWidget(new QLabel("  ➜  "));

    // Destino
    auto* dstDbBox = new QGroupBox(tr("Banco de destino"));
    dstDbBox->setStyleSheet("QGroupBox{font-size:11px;font-weight:bold;}");
    auto* dstDbLay = new QVBoxLayout(dstDbBox);
    m_dstDbCombo = new QComboBox();
    m_dstDbCombo->setStyleSheet("font-size:11px;");
    for (const QString& n : m_openDbNames)
        m_dstDbCombo->addItem(QIcon::fromTheme("database"), n);
    // Seleciona o segundo banco como destino padrão (se existir)
    if (m_openDbs.size() >= 2) m_dstDbCombo->setCurrentIndex(1);
    dstDbLay->addWidget(m_dstDbCombo);
    dbRow->addWidget(dstDbBox);

    root->addLayout(dbRow);

    // Splitter: árvore origem | seta | árvore destino
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setHandleWidth(6);

    // Painel de origem
    auto* srcBox = new QGroupBox(tr("Grupos de origem — selecione o grupo a importar"));
    srcBox->setStyleSheet("QGroupBox{font-size:11px;font-weight:bold;"
                           "border:1px solid #bee3f8;border-radius:4px;"
                           "margin-top:6px;padding-top:4px;}"
                           "QGroupBox::title{subcontrol-origin:margin;left:8px;}");
    auto* srcLay = new QVBoxLayout(srcBox);
    m_sourceTree = new QTreeWidget();
    m_sourceTree->setHeaderHidden(true);
    m_sourceTree->setStyleSheet("font-size:11px;");
    m_sourceTree->setSelectionMode(QAbstractItemView::SingleSelection);
    srcLay->addWidget(m_sourceTree);
    splitter->addWidget(srcBox);

    // Seta central
    auto* arrow = new QLabel("➜");
    arrow->setAlignment(Qt::AlignCenter);
    arrow->setStyleSheet("font-size:24px;color:#3182ce;font-weight:bold;");
    arrow->setFixedWidth(36);
    splitter->addWidget(arrow);

    // Painel de destino
    auto* dstBox = new QGroupBox(tr("Grupos de destino — onde inserir o grupo"));
    dstBox->setStyleSheet("QGroupBox{font-size:11px;font-weight:bold;"
                           "border:1px solid #c6f6d5;border-radius:4px;"
                           "margin-top:6px;padding-top:4px;}"
                           "QGroupBox::title{subcontrol-origin:margin;left:8px;}");
    auto* dstLay = new QVBoxLayout(dstBox);
    m_destTree = new QTreeWidget();
    m_destTree->setHeaderHidden(true);
    m_destTree->setStyleSheet("font-size:11px;");
    m_destTree->setSelectionMode(QAbstractItemView::SingleSelection);
    dstLay->addWidget(m_destTree);
    splitter->addWidget(dstBox);

    splitter->setSizes({440, 36, 440});
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    splitter->setStretchFactor(2, 1);
    root->addWidget(splitter, 1);

    // Status
    m_statusLabel = new QLabel(tr("Selecione o grupo de origem e o grupo de destino."));
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("font-size:11px;color:#4a5568;padding:4px 6px;"
                                  "background:#f7fafc;border-radius:3px;");
    root->addWidget(m_statusLabel);

    // Rodapé
    auto* foot = new QHBoxLayout();
    m_btnImport = new QPushButton(tr("📥 Importar Grupo Completo"));
    m_btnImport->setFixedHeight(32);
    m_btnImport->setEnabled(false);
    m_btnImport->setStyleSheet(
        "QPushButton{background:#2b6cb0;color:white;font-weight:bold;"
        "border-radius:4px;padding:0 20px;font-size:12px;}"
        "QPushButton:hover{background:#1a365d;}"
        "QPushButton:disabled{background:#bee3f8;color:#90cdf4;}");
    auto* btnClose = new QPushButton(tr("Fechar"));
    btnClose->setFixedHeight(32);
    foot->addWidget(m_btnImport);
    foot->addStretch();
    foot->addWidget(btnClose);
    root->addLayout(foot);

    // ── Conexões ──────────────────────────────────────────────────────────────
    connect(m_srcDbCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MergeGroupDialog::onSourceDbChanged);
    connect(m_dstDbCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MergeGroupDialog::onDestDbChanged);
    connect(m_sourceTree, &QTreeWidget::currentItemChanged,
            this, [this](QTreeWidgetItem*,QTreeWidgetItem*){ onSourceGroupSelected(); });
    connect(m_destTree, &QTreeWidget::currentItemChanged,
            this, [this](QTreeWidgetItem*,QTreeWidgetItem*){ onDestGroupSelected(); });
    connect(m_btnImport, &QPushButton::clicked, this, &MergeGroupDialog::onImport);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);

    // ── Carga inicial ─────────────────────────────────────────────────────────
    if (m_openDbs.isEmpty()) {
        m_statusLabel->setText(tr("⚠ Nenhum banco aberto. Abra pelo menos dois bancos para usar esta função."));
        m_statusLabel->setStyleSheet("font-size:11px;color:#c05621;padding:4px 6px;"
                                      "background:#fffaf0;border-radius:3px;");
    } else {
        onSourceDbChanged(0);
        onDestDbChanged(m_openDbs.size() >= 2 ? 1 : 0);
    }
}

// ─── API pública ──────────────────────────────────────────────────────────────

void MergeGroupDialog::show(DatabaseTabWidget* tabWidget, QWidget* parent)
{
    MergeGroupDialog dlg(tabWidget, parent);
    dlg.exec();
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

QSharedPointer<Database> MergeGroupDialog::dbAt(int index) const
{
    if (index < 0 || index >= m_openDbs.size()) return {};
    return m_openDbs[index];
}

void MergeGroupDialog::populateGroupTree(QTreeWidget* tree, Group* root,
                                          QTreeWidgetItem* parentItem)
{
    if (!root) return;

    QTreeWidgetItem* item = parentItem
        ? new QTreeWidgetItem(parentItem)
        : new QTreeWidgetItem(tree);

    const int ec = countEntriesRecursive(root);  // entradas diretas + aninhadas
    const int gc = root->children().count();

    QString label = root->name();
    if (ec > 0 || gc > 0)
        label += QString("  (%1✉ %2📁)").arg(ec).arg(gc);

    item->setText(0, label);
    item->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<void*>(root)));
    item->setIcon(0, QIcon::fromTheme("folder"));

    for (Group* child : root->children())
        populateGroupTree(tree, child, item);

    if (!parentItem) {
        item->setExpanded(true);
        // Expande primeiro nível
        for (int i = 0; i < item->childCount(); ++i)
            item->child(i)->setExpanded(true);
    }
}

Group* MergeGroupDialog::selectedGroup(QTreeWidget* tree) const
{
    auto* item = tree->currentItem();
    if (!item) return nullptr;
    return static_cast<Group*>(item->data(0, Qt::UserRole).value<void*>());
}

int MergeGroupDialog::countEntriesRecursive(Group* g) const
{
    if (!g) return 0;
    int n = g->entries().count();
    for (Group* c : g->children()) n += countEntriesRecursive(c);
    return n;
}

int MergeGroupDialog::countGroupsRecursive(Group* g) const
{
    if (!g) return 0;
    int n = g->children().count();
    for (Group* c : g->children()) n += countGroupsRecursive(c);
    return n;
}

// ─── Slots ────────────────────────────────────────────────────────────────────

void MergeGroupDialog::onSourceDbChanged(int index)
{
    m_sourceTree->clear();
    const auto db = dbAt(index);
    if (db && db->rootGroup())
        populateGroupTree(m_sourceTree, db->rootGroup());
    updateStatus();
}

void MergeGroupDialog::onDestDbChanged(int index)
{
    m_destTree->clear();
    const auto db = dbAt(index);
    if (db && db->rootGroup())
        populateGroupTree(m_destTree, db->rootGroup());
    updateStatus();
}

void MergeGroupDialog::onSourceGroupSelected() { updateStatus(); }
void MergeGroupDialog::onDestGroupSelected()   { updateStatus(); }

void MergeGroupDialog::updateStatus()
{
    Group* src  = selectedGroup(m_sourceTree);
    Group* dest = selectedGroup(m_destTree);

    if (!src || !dest) {
        m_btnImport->setEnabled(false);
        m_statusLabel->setText(tr("Selecione o grupo de origem e o grupo de destino."));
        m_statusLabel->setStyleSheet("font-size:11px;color:#4a5568;padding:4px 6px;"
                                      "background:#f7fafc;border-radius:3px;");
        return;
    }

    // Não permite copiar para si mesmo (mesmo banco, mesmo grupo)
    const int srcIdx = m_srcDbCombo->currentIndex();
    const int dstIdx = m_dstDbCombo->currentIndex();
    if (srcIdx == dstIdx && src == dest) {
        m_btnImport->setEnabled(false);
        m_statusLabel->setText(tr("⚠ Origem e destino são o mesmo grupo."));
        m_statusLabel->setStyleSheet("font-size:11px;color:#c05621;padding:4px 6px;"
                                      "background:#fffaf0;border-radius:3px;");
        return;
    }

    const int entries = countEntriesRecursive(src);
    const int groups  = countGroupsRecursive(src);
    const QString srcDb = m_openDbNames.value(srcIdx);
    const QString dstDb = m_openDbNames.value(dstIdx);

    m_statusLabel->setText(tr(
        "📥 <b>%1</b> <small>(%2)</small> → dentro de <b>%3</b> <small>(%4)</small><br>"
        "<small>%5 entradas · %6 subgrupos · todos os subníveis incluídos</small>")
        .arg(src->name().toHtmlEscaped(), srcDb.toHtmlEscaped(),
             dest->name().toHtmlEscaped(), dstDb.toHtmlEscaped())
        .arg(entries).arg(groups));
    m_statusLabel->setStyleSheet("font-size:11px;color:#2c5282;padding:4px 6px;"
                                  "background:#ebf8ff;border-radius:3px;");
    m_btnImport->setEnabled(true);
}

// ─── Importação ───────────────────────────────────────────────────────────────

void MergeGroupDialog::onImport()
{
    Group* src  = selectedGroup(m_sourceTree);
    Group* dest = selectedGroup(m_destTree);
    if (!src || !dest) return;

    const int dstIdx = m_dstDbCombo->currentIndex();
    const auto destDb = dbAt(dstIdx);
    if (!destDb) return;

    const int entries = countEntriesRecursive(src);
    const int groups  = countGroupsRecursive(src);

    const auto reply = QMessageBox::question(this,
        tr("Confirmar Importação"),
        tr("Importar <b>%1</b> → dentro de <b>%2</b>?<br><br>"
           "• %3 entradas · %4 subgrupos · todos os subníveis<br>"
           "• Novos IDs gerados automaticamente<br>"
           "• O banco de destino será salvo")
            .arg(src->name().toHtmlEscaped(),
                 dest->name().toHtmlEscaped())
            .arg(entries).arg(groups),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    // Clone recursivo completo (novos UUIDs para grupo + todas as entradas)
    Group* cloned = src->clone(Entry::CloneDefault, Group::CloneDefault);
    if (!cloned) {
        QMessageBox::critical(this, tr("Erro"), tr("Falha ao clonar o grupo."));
        return;
    }

    // Insere no grupo de destino (automaticamente fica no banco de destino)
    cloned->setParent(dest);

    // Salva o banco de destino
    QString err;
    const bool saved = destDb->save(Database::Atomic, {}, &err);
    if (!saved) {
        QMessageBox::warning(this, tr("Aviso"),
            tr("Grupo importado mas não foi possível salvar o banco:\n%1").arg(err));
    }

    // Feedback visual
    m_btnImport->setText(tr("✅ Importado!"));
    m_btnImport->setEnabled(false);
    m_btnImport->setStyleSheet(
        "QPushButton{background:#276749;color:white;font-weight:bold;"
        "border-radius:4px;padding:0 20px;font-size:12px;}");

    m_statusLabel->setText(tr(
        "✅ Grupo <b>%1</b> importado com sucesso "
        "(%2 entradas · %3 subgrupos). "
        "Feche o diálogo para ver as mudanças na interface.")
        .arg(cloned->name().toHtmlEscaped())
        .arg(entries).arg(groups));
    m_statusLabel->setStyleSheet("font-size:11px;color:#276749;padding:4px 6px;"
                                  "background:#f0fff4;border-radius:3px;");

    // Atualiza a árvore de destino para mostrar o novo grupo
    m_destTree->clear();
    if (destDb->rootGroup())
        populateGroupTree(m_destTree, destDb->rootGroup());
}
