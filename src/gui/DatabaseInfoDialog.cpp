#include "DatabaseInfoDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QFileInfo>
#include <QDateTime>
#include <QClipboard>
#include <QApplication>
#include <QLocale>
#include <QTimer>

#include "gui/MainWindow.h"
#include "gui/DatabaseTabWidget.h"
#include "gui/DatabaseWidget.h"
#include "core/Database.h"
#include "core/Group.h"
#include "core/Metadata.h"

// ─── Colunas da tabela ────────────────────────────────────────────────────────

enum Col {
    ColNome = 0,
    ColCaminho,
    ColTamanho,
    ColEntradas,
    ColGrupos,
    ColCriado,
    ColAtualizado,
    ColCount
};

static const QStringList HEADERS = {
    QObject::tr("Nome"),
    QObject::tr("Caminho"),
    QObject::tr("Tamanho"),
    QObject::tr("Entradas"),
    QObject::tr("Grupos"),
    QObject::tr("Criado em"),
    QObject::tr("Atualizado em"),
};

// ─── Construtor ───────────────────────────────────────────────────────────────

DatabaseInfoDialog::DatabaseInfoDialog(MainWindow* mainWindow, QWidget* parent)
    : QDialog(parent)
    , m_mainWindow(mainWindow)
{
    setWindowTitle(tr("Informações dos Bancos de Dados"));
    setMinimumWidth(860);
    setMinimumHeight(320);

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    layout->setContentsMargins(12, 12, 12, 12);

    // Cabeçalho
    auto* header = new QLabel(
        tr("<b>📊  Bancos de dados abertos</b>  —  todas as métricas são lidas ao vivo do disco."));
    header->setStyleSheet("font-size: 12px; color: #2c5282; padding: 4px 0;");
    layout->addWidget(header);

    // Tabela
    m_table = new QTableWidget(0, ColCount);
    m_table->setHorizontalHeaderLabels(HEADERS);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setStyleSheet(
        "QTableWidget { font-size: 12px; border: 1px solid palette(mid); }"
        "QTableWidget::item { padding: 4px 8px; }"
        "QHeaderView::section { background: #ebf8ff; color: #2b6cb0; "
        "  font-weight: bold; padding: 5px 8px; border: 1px solid #bee3f8; }");
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(ColNome,      QHeaderView::Interactive);
    m_table->horizontalHeader()->setSectionResizeMode(ColCaminho,   QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(ColTamanho,   QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColEntradas,  QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColGrupos,    QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColCriado,    QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColAtualizado,QHeaderView::ResizeToContents);
    layout->addWidget(m_table, 1);

    // Rodapé: resumo + botões
    m_summary = new QLabel();
    m_summary->setStyleSheet("font-size: 11px; color: #4a5568;");
    layout->addWidget(m_summary);

    auto* btnRow    = new QHBoxLayout();
    auto* btnRefresh = new QPushButton(QIcon::fromTheme("view-refresh"), tr("Atualizar"));
    auto* btnCopy    = new QPushButton(QIcon::fromTheme("edit-copy"),    tr("Copiar tabela"));
    auto* btnClose   = new QPushButton(tr("Fechar"));
    btnRefresh->setFixedHeight(28);
    btnCopy->setFixedHeight(28);
    btnClose->setFixedHeight(28);

    btnRow->addWidget(btnRefresh);
    btnRow->addWidget(btnCopy);
    btnRow->addStretch();
    btnRow->addWidget(btnClose);
    layout->addLayout(btnRow);

    connect(btnRefresh, &QPushButton::clicked, this, &DatabaseInfoDialog::onRefresh);
    connect(btnCopy,    &QPushButton::clicked, this, &DatabaseInfoDialog::onCopy);
    connect(btnClose,   &QPushButton::clicked, this, &QDialog::accept);

    populate();
}

// ─── Estático ─────────────────────────────────────────────────────────────────

void DatabaseInfoDialog::showInfo(MainWindow* mainWindow, QWidget* parent)
{
    DatabaseInfoDialog dlg(mainWindow, parent);
    dlg.exec();
}

// ─── Preenche tabela ──────────────────────────────────────────────────────────

void DatabaseInfoDialog::populate()
{
    m_table->setSortingEnabled(false);
    m_table->setRowCount(0);

    if (!m_mainWindow) return;

    auto* tabWidget = m_mainWindow->findChild<DatabaseTabWidget*>();
    if (!tabWidget) return;

    int totalEntries = 0;
    int totalGroups  = 0;
    qint64 totalSize = 0;
    int dbCount      = 0;

    for (int i = 0; i < tabWidget->count(); ++i) {
        auto* dbWidget = qobject_cast<DatabaseWidget*>(tabWidget->widget(i));
        if (!dbWidget) continue;
        auto db = dbWidget->database();
        if (!db) continue;

        const QString path = db->filePath();
        const QFileInfo fi(path);

        // Métricas do banco
        const QString name     = db->metadata()->name().isEmpty()
                                  ? fi.fileName()
                                  : db->metadata()->name();
        const qint64  size     = fi.size();
        const int     entries  = db->rootGroup()->entriesRecursive(false).count();
        const int     groups   = db->rootGroup()->groupsRecursive(false).count();
        const QString created  = fi.birthTime().isValid()
                                  ? fi.birthTime().toString("dd/MM/yyyy HH:mm")
                                  : tr("—");
        const QString updated  = fi.lastModified().isValid()
                                  ? fi.lastModified().toString("dd/MM/yyyy HH:mm:ss")
                                  : tr("—");

        // Linha na tabela
        const int row = m_table->rowCount();
        m_table->insertRow(row);

        auto cell = [&](int col, const QString& text, Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter) {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(align);
            m_table->setItem(row, col, item);
            return item;
        };

        auto* nameItem = cell(ColNome, name);
        // Banco principal (sem senha) → ícone diferente
        if (path.contains("cfdmnote", Qt::CaseInsensitive) || db->key()->isEmpty()) {
            nameItem->setIcon(QIcon::fromTheme("folder-open"));
        } else {
            nameItem->setIcon(QIcon::fromTheme("security-high"));
        }

        cell(ColCaminho,   path.isEmpty() ? tr("(não salvo)") : path);
        cell(ColTamanho,   size > 0 ? formatSize(size) : tr("—"),
             Qt::AlignRight | Qt::AlignVCenter);

        auto* entItem = new QTableWidgetItem();
        entItem->setData(Qt::DisplayRole, entries);
        entItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, ColEntradas, entItem);

        auto* grpItem = new QTableWidgetItem();
        grpItem->setData(Qt::DisplayRole, groups);
        grpItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, ColGrupos, grpItem);

        cell(ColCriado,    created);
        cell(ColAtualizado, updated);

        // Destaque para banco ativo
        if (dbWidget == tabWidget->currentDatabaseWidget()) {
            for (int c = 0; c < ColCount; ++c) {
                if (m_table->item(row, c))
                    m_table->item(row, c)->setBackground(QColor("#ebf8ff"));
            }
        }

        totalEntries += entries;
        totalGroups  += groups;
        totalSize    += size;
        ++dbCount;
    }

    m_table->setSortingEnabled(true);
    m_table->resizeColumnsToContents();
    m_table->horizontalHeader()->setSectionResizeMode(ColCaminho, QHeaderView::Stretch);

    // Resumo
    m_summary->setText(tr("  %1 banco(s) aberto(s)  ·  %2 entradas no total  ·  "
                           "%3 grupos  ·  %4 em disco")
                            .arg(dbCount)
                            .arg(QLocale().toString(totalEntries))
                            .arg(QLocale().toString(totalGroups))
                            .arg(formatSize(totalSize)));
}

// ─── Atualizar ────────────────────────────────────────────────────────────────

void DatabaseInfoDialog::onRefresh()
{
    populate();
}

// ─── Copiar tabela como texto ─────────────────────────────────────────────────

void DatabaseInfoDialog::onCopy()
{
    QStringList lines;

    // Cabeçalho
    QStringList header;
    for (int c = 0; c < ColCount; ++c)
        header << m_table->horizontalHeaderItem(c)->text();
    lines << header.join("\t");

    // Linhas
    for (int r = 0; r < m_table->rowCount(); ++r) {
        QStringList row;
        for (int c = 0; c < ColCount; ++c) {
            auto* item = m_table->item(r, c);
            row << (item ? item->text() : "");
        }
        lines << row.join("\t");
    }

    QApplication::clipboard()->setText(lines.join("\n"));

    // Feedback visual no título por 2 segundos
    const QString oldTitle = windowTitle();
    setWindowTitle(tr("Informações dos Bancos — ✅ Copiado!"));
    QTimer::singleShot(2000, this, [this, oldTitle]() { setWindowTitle(oldTitle); });
}

// ─── Formata tamanho ──────────────────────────────────────────────────────────

QString DatabaseInfoDialog::formatSize(qint64 bytes)
{
    if (bytes <= 0)   return "0 B";
    if (bytes < 1024) return QString("%1 B").arg(bytes);
    if (bytes < 1024 * 1024)
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024 * 1024 * 1024)
        return QString("%1 MB").arg(bytes / (1024.0 * 1024), 0, 'f', 2);
    return QString("%1 GB").arg(bytes / (1024.0 * 1024 * 1024), 0, 'f', 2);
}
