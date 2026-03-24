#include "GlossaryDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QFormLayout>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QScreen>
#include <QShortcut>
#include <QKeySequence>
#include <QDateTime>
#include <QTimer>

#include "core/Entry.h"
#include "core/Database.h"

// ─── Construtor ───────────────────────────────────────────────────────────────

GlossaryDialog::GlossaryDialog(Entry* entry,
                                const QSharedPointer<Database>& db,
                                QWidget* parent)
    : QDialog(parent)
    , m_entry(entry)
    , m_db(db)
{
    setWindowTitle(tr("🧠 Seção do Saber — Glossário"));
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
    const QRect screen = QApplication::primaryScreen()->availableGeometry();
    resize(qMin(1000, screen.width() - 80), qMin(640, screen.height() - 80));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Barra de busca ────────────────────────────────────────────────────────
    auto* searchBar = new QWidget();
    searchBar->setStyleSheet("background:#ebf8ff; border-bottom:1px solid #bee3f8; padding:4px;");
    auto* sbLay = new QHBoxLayout(searchBar);
    sbLay->setContentsMargins(10, 4, 10, 4);
    sbLay->addWidget(new QLabel("🔍"));
    m_searchBox = new QLineEdit();
    m_searchBox->setPlaceholderText(tr("Buscar termo, definição ou sinônimo..."));
    m_searchBox->setFixedHeight(26);
    m_searchBox->setStyleSheet("font-size:12px; border:1px solid #bee3f8; "
                                "border-radius:4px; padding:2px 6px;");
    sbLay->addWidget(m_searchBox, 1);

    auto* btnNew = new QPushButton(tr("＋ Novo Termo"));
    btnNew->setFixedHeight(28);
    btnNew->setStyleSheet("QPushButton{background:#38a169;color:white;border-radius:4px;"
                          "padding:0 14px;font-size:11px;}"
                          "QPushButton:hover{background:#276749;}");
    sbLay->addWidget(btnNew);
    root->addWidget(searchBar);

    // ── Splitter principal: categorias | termos | detalhe ─────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setHandleWidth(2);

    // Painel esquerdo — categorias
    auto* catPanel = new QWidget();
    catPanel->setMinimumWidth(140);
    catPanel->setMaximumWidth(200);
    auto* catLay = new QVBoxLayout(catPanel);
    catLay->setContentsMargins(0, 0, 0, 0);
    catLay->setSpacing(0);

    auto* catHeader = new QLabel(tr("  Categorias"));
    catHeader->setFixedHeight(30);
    catHeader->setStyleSheet("background:#2c5282;color:white;font-weight:bold;"
                              "font-size:11px;padding-left:8px;");
    catLay->addWidget(catHeader);

    m_categoryList = new QListWidget();
    m_categoryList->setStyleSheet(
        "QListWidget{border:none;background:#f7fafc;font-size:11px;}"
        "QListWidget::item{padding:5px 8px;border-bottom:1px solid #edf2f7;}"
        "QListWidget::item:hover{background:#ebf8ff;}"
        "QListWidget::item:selected{background:#bee3f8;color:#2c5282;}");
    catLay->addWidget(m_categoryList, 1);
    splitter->addWidget(catPanel);

    // Painel centro — tabela de termos
    auto* termPanel = new QWidget();
    auto* termLay = new QVBoxLayout(termPanel);
    termLay->setContentsMargins(0, 0, 0, 0);
    termLay->setSpacing(0);

    m_termTable = new QTableWidget(0, 3);
    m_termTable->setHorizontalHeaderLabels({tr("Termo"), tr("Categoria"), tr("Definição (resumo)")});
    m_termTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_termTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_termTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_termTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_termTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_termTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_termTable->setAlternatingRowColors(true);
    m_termTable->verticalHeader()->setVisible(false);
    m_termTable->setStyleSheet(
        "QTableWidget{font-size:11px;border:none;}"
        "QHeaderView::section{background:#edf2f7;font-weight:bold;padding:4px;"
        "border:none;border-bottom:2px solid #cbd5e0;}"
        "QTableWidget::item{padding:4px 6px;}");
    termLay->addWidget(m_termTable, 1);
    splitter->addWidget(termPanel);

    // Painel direito — detalhe do termo
    auto* detailPanel = new QGroupBox(tr("Detalhe do Termo"));
    detailPanel->setMinimumWidth(260);
    detailPanel->setStyleSheet("QGroupBox{font-size:11px;font-weight:bold;"
                                "border:1px solid #e2e8f0;border-radius:4px;"
                                "margin-top:6px;padding-top:4px;}"
                                "QGroupBox::title{subcontrol-origin:margin;left:8px;}");
    auto* detLay = new QVBoxLayout(detailPanel);
    detLay->setSpacing(6);

    m_detailTerm = new QLabel();
    m_detailTerm->setStyleSheet("font-size:15px;font-weight:bold;color:#1a365d;");
    m_detailTerm->setWordWrap(true);
    detLay->addWidget(m_detailTerm);

    m_detailCategory = new QLabel();
    m_detailCategory->setStyleSheet("font-size:10px;color:#718096;"
                                     "background:#edf2f7;padding:2px 6px;border-radius:3px;");
    detLay->addWidget(m_detailCategory);

    detLay->addWidget(new QLabel(tr("<b>Definição:</b>")));
    m_detailDef = new QTextEdit();
    m_detailDef->setReadOnly(true);
    m_detailDef->setFixedHeight(100);
    m_detailDef->setStyleSheet("font-size:11px;border:1px solid #e2e8f0;border-radius:3px;");
    detLay->addWidget(m_detailDef);

    m_detailSynonyms = new QLabel();
    m_detailSynonyms->setStyleSheet("font-size:10px;color:#553c9a;");
    m_detailSynonyms->setWordWrap(true);
    detLay->addWidget(m_detailSynonyms);

    detLay->addWidget(new QLabel(tr("<b>Exemplo:</b>")));
    m_detailExample = new QTextEdit();
    m_detailExample->setReadOnly(true);
    m_detailExample->setFixedHeight(60);
    m_detailExample->setStyleSheet("font-size:11px;border:1px solid #e2e8f0;border-radius:3px;"
                                    "color:#4a5568;font-style:italic;");
    detLay->addWidget(m_detailExample);

    detLay->addStretch();

    // Botões de ação no detalhe
    auto* actRow = new QHBoxLayout();
    m_btnInsert = new QPushButton(tr("📝 Inserir na Nota"));
    m_btnInsert->setFixedHeight(28);
    m_btnInsert->setEnabled(false);
    m_btnInsert->setStyleSheet("QPushButton{background:#2b6cb0;color:white;border-radius:4px;"
                                "padding:0 10px;font-size:11px;}"
                                "QPushButton:hover{background:#1a365d;}"
                                "QPushButton:disabled{background:#bee3f8;color:#90cdf4;}");
    m_btnEdit = new QPushButton(tr("✏ Editar"));
    m_btnEdit->setFixedHeight(28);
    m_btnEdit->setEnabled(false);
    m_btnEdit->setStyleSheet("font-size:11px;");
    m_btnDelete = new QPushButton(tr("🗑 Apagar"));
    m_btnDelete->setFixedHeight(28);
    m_btnDelete->setEnabled(false);
    m_btnDelete->setStyleSheet("font-size:11px;color:#c53030;");
    actRow->addWidget(m_btnInsert);
    actRow->addWidget(m_btnEdit);
    actRow->addWidget(m_btnDelete);
    detLay->addLayout(actRow);

    splitter->addWidget(detailPanel);
    splitter->setSizes({160, 480, 280});
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);
    root->addWidget(splitter, 1);

    // ── Rodapé ────────────────────────────────────────────────────────────────
    auto* footer = new QWidget();
    footer->setFixedHeight(36);
    footer->setStyleSheet("background:#f7fafc;border-top:1px solid #e2e8f0;");
    auto* fLay = new QHBoxLayout(footer);
    fLay->setContentsMargins(10, 4, 10, 4);
    auto* hint = new QLabel(tr("💡 Duplo clique no termo para editar  ·  Ctrl+Shift+K para abrir"));
    hint->setStyleSheet("font-size:10px;color:#718096;");
    auto* btnClose = new QPushButton(tr("Fechar"));
    btnClose->setFixedHeight(28);
    fLay->addWidget(hint, 1);
    fLay->addWidget(btnClose);
    root->addWidget(footer);

    // ── Conexões ──────────────────────────────────────────────────────────────
    connect(btnNew,    &QPushButton::clicked,   this, &GlossaryDialog::onNewTerm);
    connect(m_btnEdit, &QPushButton::clicked,   this, &GlossaryDialog::onEditTerm);
    connect(m_btnDelete,&QPushButton::clicked,  this, &GlossaryDialog::onDeleteTerm);
    connect(m_btnInsert,&QPushButton::clicked,  this, &GlossaryDialog::onInsertInNote);
    connect(btnClose,  &QPushButton::clicked,   this, &QDialog::accept);
    connect(m_searchBox,  &QLineEdit::textChanged, this, &GlossaryDialog::onSearch);
    connect(m_categoryList,&QListWidget::currentRowChanged,this,&GlossaryDialog::onCategorySelected);
    connect(m_termTable,  &QTableWidget::currentCellChanged,
            this, [this](int row,int,int,int) {
        if (row < 0) { clearDetail(); return; }
        onTermSelected();
    });
    connect(m_termTable, &QTableWidget::cellDoubleClicked,
            this, [this](int,int) { onEditTerm(); });

    new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(accept()));
    new QShortcut(QKeySequence(Qt::Key_Delete), m_termTable, this,
                  [this]() { if (m_termTable->hasFocus()) onDeleteTerm(); });

    // ── Carga inicial ─────────────────────────────────────────────────────────
    refreshCategories();
    loadTerms();
}

// ─── API pública ──────────────────────────────────────────────────────────────

void GlossaryDialog::showGlossary(Entry* entry,
                                   const QSharedPointer<Database>& db,
                                   QWidget* parent)
{
    GlossaryDialog dlg(entry, db, parent);
    dlg.exec();
}

// ─── Carga de dados ───────────────────────────────────────────────────────────

void GlossaryDialog::refreshCategories()
{
    m_categoryList->blockSignals(true);
    const int prevRow = m_categoryList->currentRow();
    m_categoryList->clear();
    auto* all = new QListWidgetItem(tr("📚 Todos"));
    all->setData(Qt::UserRole, QString());
    m_categoryList->addItem(all);

    NoteManager& nm = NoteManager::instance();
    if (nm.isOpen()) {
        for (const QString& cat : nm.getGlossaryCategories()) {
            auto* item = new QListWidgetItem("  " + cat);
            item->setData(Qt::UserRole, cat);
            m_categoryList->addItem(item);
        }
    }
    m_categoryList->setCurrentRow(qMax(0, prevRow));
    m_categoryList->blockSignals(false);
}

void GlossaryDialog::loadTerms(const QString& category, const QString& search)
{
    NoteManager& nm = NoteManager::instance();
    QList<NoteManager::GlossaryTerm> terms;

    if (!nm.isOpen()) {
        m_termTable->setRowCount(0);
        return;
    }

    if (!search.isEmpty())
        terms = nm.searchGlossaryTerms(search);
    else
        terms = nm.getGlossaryTerms(category);

    m_termTable->setRowCount(0);
    for (const auto& t : terms) {
        const int row = m_termTable->rowCount();
        m_termTable->insertRow(row);

        auto* termItem = new QTableWidgetItem(t.term);
        termItem->setData(Qt::UserRole, t.id);
        termItem->setFont([]{ QFont f; f.setBold(true); return f; }());
        m_termTable->setItem(row, 0, termItem);

        m_termTable->setItem(row, 1, new QTableWidgetItem(t.category));

        const QString shortDef = t.definition.left(80).replace('\n', ' ');
        m_termTable->setItem(row, 2, new QTableWidgetItem(shortDef));
    }

    // Restaura seleção pelo id se ainda existir
    for (int r = 0; r < m_termTable->rowCount(); ++r) {
        if (m_termTable->item(r, 0)->data(Qt::UserRole).toString() == m_currentId) {
            m_termTable->selectRow(r);
            return;
        }
    }
    clearDetail();
}

// ─── Slots ────────────────────────────────────────────────────────────────────

void GlossaryDialog::onSearch(const QString& query)
{
    if (query.isEmpty()) {
        onCategorySelected();
    } else {
        m_categoryList->clearSelection();
        loadTerms({}, query);
    }
}

void GlossaryDialog::onCategorySelected()
{
    auto* item = m_categoryList->currentItem();
    const QString cat = item ? item->data(Qt::UserRole).toString() : QString();
    loadTerms(cat, m_searchBox->text());
}

void GlossaryDialog::onTermSelected()
{
    const int row = m_termTable->currentRow();
    if (row < 0) { clearDetail(); return; }

    const QString id = m_termTable->item(row, 0)->data(Qt::UserRole).toString();
    m_currentId = id;

    NoteManager& nm = NoteManager::instance();
    const auto terms = nm.getGlossaryTerms();
    for (const auto& t : terms) {
        if (t.id == id) {
            showTermDetail(t);
            return;
        }
    }
}

void GlossaryDialog::showTermDetail(const NoteManager::GlossaryTerm& t)
{
    m_detailTerm->setText(t.term);
    m_detailCategory->setText("  " + t.category + "  ");
    m_detailDef->setPlainText(t.definition);
    m_detailSynonyms->setText(t.synonyms.isEmpty()
        ? QString() : tr("🔗 Sinônimos: %1").arg(t.synonyms));
    m_detailExample->setPlainText(t.example);

    const bool hasEntry = (m_entry != nullptr);
    m_btnInsert->setEnabled(hasEntry);
    m_btnEdit->setEnabled(true);
    m_btnDelete->setEnabled(true);
}

void GlossaryDialog::clearDetail()
{
    m_currentId.clear();
    m_detailTerm->clear();
    m_detailCategory->clear();
    m_detailDef->clear();
    m_detailSynonyms->clear();
    m_detailExample->clear();
    m_btnInsert->setEnabled(false);
    m_btnEdit->setEnabled(false);
    m_btnDelete->setEnabled(false);
}

// ─── Editor de termo ──────────────────────────────────────────────────────────

bool GlossaryDialog::openTermEditor(NoteManager::GlossaryTerm& t)
{
    QDialog dlg(this);
    dlg.setWindowTitle(t.id.isEmpty() ? tr("Novo Termo") : tr("Editar Termo"));
    dlg.resize(500, 400);
    auto* lay = new QVBoxLayout(&dlg);
    lay->setSpacing(8);

    QFormLayout* form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);

    auto* termEdit = new QLineEdit(t.term);
    termEdit->setPlaceholderText(tr("Palavra ou expressão"));

    // Categoria: combo editável com categorias existentes
    auto* catCombo = new QComboBox();
    catCombo->setEditable(true);
    catCombo->addItem("Geral");
    for (const QString& c : NoteManager::instance().getGlossaryCategories())
        if (catCombo->findText(c) == -1) catCombo->addItem(c);
    catCombo->setCurrentText(t.category.isEmpty() ? "Geral" : t.category);

    auto* synEdit = new QLineEdit(t.synonyms);
    synEdit->setPlaceholderText(tr("ex: palavra1, palavra2, expressão"));

    auto* defEdit = new QTextEdit();
    defEdit->setPlainText(t.definition);
    defEdit->setFixedHeight(100);
    defEdit->setPlaceholderText(tr("Definição completa..."));

    auto* exEdit = new QTextEdit();
    exEdit->setPlainText(t.example);
    exEdit->setFixedHeight(60);
    exEdit->setPlaceholderText(tr("Exemplo de uso (opcional)..."));
    exEdit->setStyleSheet("font-style:italic;color:#4a5568;");

    form->addRow(tr("Termo *:"),       termEdit);
    form->addRow(tr("Categoria:"),     catCombo);
    form->addRow(tr("Sinônimos:"),     synEdit);
    form->addRow(tr("Definição *:"),   defEdit);
    form->addRow(tr("Exemplo:"),       exEdit);
    lay->addLayout(form);

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    lay->addWidget(btns);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return false;

    const QString term = termEdit->text().trimmed();
    const QString def  = defEdit->toPlainText().trimmed();
    if (term.isEmpty() || def.isEmpty()) {
        QMessageBox::warning(this, tr("Campo obrigatório"),
                             tr("Termo e Definição são obrigatórios."));
        return false;
    }

    t.term       = term;
    t.definition = def;
    t.category   = catCombo->currentText().trimmed();
    t.synonyms   = synEdit->text().trimmed();
    t.example    = exEdit->toPlainText().trimmed();
    return true;
}

// ─── CRUD slots ───────────────────────────────────────────────────────────────

void GlossaryDialog::onNewTerm()
{
    NoteManager& nm = NoteManager::instance();
    if (!nm.isOpen()) {
        QMessageBox::warning(this, tr("Glossário"),
            tr("Banco de dados não está aberto. Abra um banco primeiro."));
        return;
    }

    NoteManager::GlossaryTerm t;
    // Pré-preenche a categoria selecionada
    if (auto* item = m_categoryList->currentItem())
        t.category = item->data(Qt::UserRole).toString();

    if (!openTermEditor(t)) return;
    const QString newId = nm.addGlossaryTerm(t);
    if (newId.isEmpty()) {
        QMessageBox::warning(this, tr("Erro"), tr("Não foi possível salvar o termo."));
        return;
    }
    m_currentId = newId;
    refreshCategories();
    loadTerms(m_categoryList->currentItem()
                  ? m_categoryList->currentItem()->data(Qt::UserRole).toString()
                  : QString(),
              m_searchBox->text());
}

void GlossaryDialog::onEditTerm()
{
    if (m_currentId.isEmpty()) return;
    NoteManager& nm = NoteManager::instance();
    const auto terms = nm.getGlossaryTerms();
    for (auto t : terms) {
        if (t.id != m_currentId) continue;
        if (!openTermEditor(t)) return;
        nm.updateGlossaryTerm(t);
        refreshCategories();
        loadTerms(m_categoryList->currentItem()
                      ? m_categoryList->currentItem()->data(Qt::UserRole).toString()
                      : QString(),
                  m_searchBox->text());
        return;
    }
}

void GlossaryDialog::onDeleteTerm()
{
    if (m_currentId.isEmpty()) return;
    const int row = m_termTable->currentRow();
    const QString name = row >= 0 ? m_termTable->item(row, 0)->text() : m_currentId;
    if (QMessageBox::question(this, tr("Apagar Termo"),
            tr("Apagar o termo <b>%1</b>?").arg(name.toHtmlEscaped()),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    NoteManager::instance().deleteGlossaryTerm(m_currentId);
    m_currentId.clear();
    refreshCategories();
    loadTerms(m_categoryList->currentItem()
                  ? m_categoryList->currentItem()->data(Qt::UserRole).toString()
                  : QString(),
              m_searchBox->text());
}

void GlossaryDialog::onInsertInNote()
{
    if (!m_entry || m_currentId.isEmpty()) return;

    const QString term = m_detailTerm->text();
    const QString def  = m_detailDef->toPlainText().trimmed();
    const QString syn  = m_detailSynonyms->text();
    const QString ex   = m_detailExample->toPlainText().trimmed();

    QString insert = QString("\n\n---\n📖 **%1**\n%2").arg(term, def);
    if (!syn.isEmpty())  insert += "\n" + syn;
    if (!ex.isEmpty())   insert += "\n\n*Exemplo: " + ex + "*";

    // Adiciona nas notes nativas da entrada
    m_entry->setNotes(m_entry->notes() + insert);
    if (m_db) {
        QString err;
        m_db->save(Database::Atomic, {}, &err);
    }

    m_btnInsert->setText(tr("✅ Inserido!"));
    QTimer::singleShot(2000, m_btnInsert, [this]() {
        m_btnInsert->setText(tr("📝 Inserir na Nota"));
    });
}
