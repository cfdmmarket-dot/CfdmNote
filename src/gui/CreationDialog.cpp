#include "CreationDialog.h"
#include "NoteEditorWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QClipboard>
#include <QApplication>
#include <QMessageBox>
#include <QScrollArea>
#include <QFrame>

#include "core/Entry.h"
#include "core/Database.h"
#include "core/Group.h"

// ─── Templates disponíveis ────────────────────────────────────────────────────
struct Template {
    QString name;
    QString icon;
    QString description;
    bool    needsAI;     // true = stub CFDM Triplex
    QString body;        // texto do template (suporta {title}, {url}, {notes}, {group}, {tags})
};

static const QList<Template> TEMPLATES = {
    // ── Funcionais agora ─────────────────────────────────────────────────────
    {
        QObject::tr("Email Profissional"),
        "📧", QObject::tr("Email formal usando título e notas da entrada."),
        false,
        QObject::tr(
            "Assunto: {title}\n\n"
            "Olá,\n\n"
            "Espero que esteja bem.\n\n"
            "{notes_first}\n\n"
            "Para mais informações: {url}\n\n"
            "Atenciosamente,\n"
            "[Seu Nome]\n"
        )
    },
    {
        QObject::tr("Plano de Projeto"),
        "📋", QObject::tr("Estrutura de projeto com base na entrada e grupo."),
        false,
        QObject::tr(
            "# Projeto: {title}\n"
            "**Grupo:** {group}\n"
            "**Tags:** {tags}\n"
            "**Referência:** {url}\n\n"
            "## Objetivo\n"
            "{notes_first}\n\n"
            "## Escopo\n"
            "- [ ] Definir requisitos\n"
            "- [ ] Planejar etapas\n"
            "- [ ] Executar\n"
            "- [ ] Revisar e entregar\n\n"
            "## Notas Completas\n"
            "{notes}\n"
        )
    },
    {
        QObject::tr("Post de Blog / Redes Sociais"),
        "📱", QObject::tr("Post formatado para blog ou redes sociais."),
        false,
        QObject::tr(
            "🔥 {title}\n\n"
            "{notes_first}\n\n"
            "🔗 Saiba mais: {url}\n\n"
            "#{tags_hash}\n"
        )
    },
    {
        QObject::tr("Briefing / Resumo Executivo"),
        "📄", QObject::tr("Resumo estruturado da entrada para reuniões."),
        false,
        QObject::tr(
            "BRIEFING — {title}\n"
            "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
            "Grupo: {group}  |  Tags: {tags}\n"
            "URL: {url}\n"
            "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n"
            "{notes}\n"
        )
    },
    {
        QObject::tr("Script de Vídeo / Roteiro"),
        "🎬", QObject::tr("Roteiro básico para vídeo baseado na entrada."),
        false,
        QObject::tr(
            "ROTEIRO: {title}\n\n"
            "[INTRO]\n"
            "Olá! Hoje vamos falar sobre: {title}.\n\n"
            "[DESENVOLVIMENTO]\n"
            "{notes}\n\n"
            "[CALL TO ACTION]\n"
            "Acesse: {url}\n\n"
            "[ENCERRAMENTO]\n"
            "Curta, compartilhe e deixe seu comentário!\n"
            "#Tags: {tags}\n"
        )
    },
    {
        QObject::tr("Lista de Tarefas (Checklist)"),
        "✅", QObject::tr("Transforma as linhas da nota em checklist."),
        false,
        QObject::tr("# Checklist — {title}\n\n{notes_checklist}\n")
    },
    // ── Requer CFDM Triplex ──────────────────────────────────────────────────
    {
        QObject::tr("✨ Ebook Completo  [CFDM Triplex]"),
        "📖", QObject::tr("Gera um ebook completo com capítulos, introdução e conclusão."),
        true, {}
    },
    {
        QObject::tr("✨ Descrição de Produto Digital  [CFDM Triplex]"),
        "💎", QObject::tr("Copy de vendas, bullet points e CTA para produto digital."),
        true, {}
    },
    {
        QObject::tr("✨ Campanha de Email Marketing  [CFDM Triplex]"),
        "📨", QObject::tr("Sequência de emails (boas-vindas, nutrição, oferta)."),
        true, {}
    },
    {
        QObject::tr("✨ Plano de Conteúdo (30 dias)  [CFDM Triplex]"),
        "📅", QObject::tr("Calendário editorial com temas, formatos e canais."),
        true, {}
    },
    {
        QObject::tr("✨ Proposta Comercial  [CFDM Triplex]"),
        "🤝", QObject::tr("Proposta formal com escopo, prazo e valores."),
        true, {}
    },
    {
        QObject::tr("✨ Script Audiobook  [CFDM Triplex]"),
        "🎧", QObject::tr("Roteiro otimizado para narração em audiobook."),
        true, {}
    },
};

// ─── Static launcher ──────────────────────────────────────────────────────────
void CreationDialog::create(Entry* entry, QSharedPointer<Database> db, QWidget* parent)
{
    if (!entry) {
        QMessageBox::information(parent, QObject::tr("Criar"),
            QObject::tr("Selecione uma entrada primeiro."));
        return;
    }
    auto* dlg = new CreationDialog(entry, db, parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

// ─── Construtor ──────────────────────────────────────────────────────────────
CreationDialog::CreationDialog(Entry* entry, QSharedPointer<Database> db, QWidget* parent)
    : QDialog(parent), m_entry(entry), m_db(db)
{
    const QString title = entry ? entry->title() : tr("(sem entrada)");
    setWindowTitle(tr("🛠 Criar a partir de: %1").arg(title));
    setMinimumSize(700, 560);
    buildUI();
}

void CreationDialog::buildUI()
{
    auto* root = new QVBoxLayout(this);

    // ── Cabeçalho ────────────────────────────────────────────────────────────
    auto* hdr = new QLabel(tr(
        "<b>🛠 Central de Criação de Conteúdo</b><br>"
        "<small>Use os dados desta entrada como base para gerar emails, projetos, posts e mais.</small>"));
    hdr->setTextFormat(Qt::RichText);
    hdr->setMargin(6);
    root->addWidget(hdr);

    // ── Seletor de template ───────────────────────────────────────────────────
    auto* topRow = new QHBoxLayout();
    topRow->addWidget(new QLabel(tr("Gerar:")));
    m_templateCombo = new QComboBox();
    for (const auto& t : TEMPLATES) {
        m_templateCombo->addItem(t.icon + " " + t.name);
        if (t.needsAI)
            m_templateCombo->setItemData(m_templateCombo->count() - 1,
                QColor("#7c3aed"), Qt::ForegroundRole);
    }
    topRow->addWidget(m_templateCombo, 1);
    root->addLayout(topRow);

    // ── Splitter: contexto | saída ────────────────────────────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal);

    // Contexto da entrada (read-only)
    auto* grpCtx = new QGroupBox(tr("Dados da Entrada"));
    auto* ctxLay = new QVBoxLayout(grpCtx);
    m_contextView = new QTextEdit();
    m_contextView->setReadOnly(true);
    m_contextView->setPlainText(buildContext());
    m_contextView->setMinimumWidth(220);
    ctxLay->addWidget(m_contextView);
    splitter->addWidget(grpCtx);

    // Área de saída
    auto* grpOut = new QGroupBox(tr("Conteúdo Gerado"));
    auto* outLay = new QVBoxLayout(grpOut);
    m_output = new QTextEdit();
    m_output->setPlaceholderText(tr("Clique em Gerar para criar o conteúdo..."));
    outLay->addWidget(m_output);
    splitter->addWidget(grpOut);

    splitter->setSizes({240, 440});
    root->addWidget(splitter, 1);

    // ── Status ────────────────────────────────────────────────────────────────
    m_statusLabel = new QLabel();
    m_statusLabel->setTextFormat(Qt::RichText);
    root->addWidget(m_statusLabel);

    // ── Botões ────────────────────────────────────────────────────────────────
    auto* btnRow = new QHBoxLayout();

    m_btnGenerate = new QPushButton(tr("⚡ Gerar"));
    m_btnGenerate->setDefault(true);
    connect(m_btnGenerate, &QPushButton::clicked, this, &CreationDialog::onGenerate);
    btnRow->addWidget(m_btnGenerate);

    m_btnCopy = new QPushButton(tr("📋 Copiar"));
    m_btnCopy->setEnabled(false);
    connect(m_btnCopy, &QPushButton::clicked, this, &CreationDialog::onCopy);
    btnRow->addWidget(m_btnCopy);

    m_btnInsert = new QPushButton(tr("📝 Inserir na Nota"));
    m_btnInsert->setEnabled(false);
    connect(m_btnInsert, &QPushButton::clicked, this, &CreationDialog::onInsertInNote);
    btnRow->addWidget(m_btnInsert);

    btnRow->addStretch();

    auto* btnClose = new QPushButton(tr("Fechar"));
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    btnRow->addWidget(btnClose);

    root->addLayout(btnRow);

    // Atualiza descrição ao mudar template
    connect(m_templateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CreationDialog::onTemplateChanged);
    onTemplateChanged(0);
}

// ─── Contexto da entrada ──────────────────────────────────────────────────────
QString CreationDialog::buildContext() const
{
    if (!m_entry) return {};
    QStringList lines;
    lines << tr("Título: ") + m_entry->title();
    if (!m_entry->url().isEmpty())
        lines << tr("URL: ") + m_entry->url();
    if (m_entry->group())
        lines << tr("Grupo: ") + m_entry->group()->name();
    if (!m_entry->tags().isEmpty())
        lines << tr("Tags: ") + m_entry->tags();
    if (!m_entry->notes().isEmpty()) {
        lines << QString();
        lines << tr("--- Notas ---");
        lines << m_entry->notes().left(800);
    }
    return lines.join('\n');
}

// ─── Aplicar template ─────────────────────────────────────────────────────────
QString CreationDialog::applyTemplate(const QString& tmpl) const
{
    if (!m_entry) return tmpl;

    const QString title  = m_entry->title();
    const QString url    = m_entry->url().isEmpty() ? tr("(sem URL)") : m_entry->url();
    const QString group  = m_entry->group() ? m_entry->group()->name() : tr("(raiz)");
    const QString tags   = m_entry->tags().isEmpty()
                           ? tr("(sem tags)") : m_entry->tags();
    const QString notes  = m_entry->notes().trimmed();

    // Primeira frase/parágrafo das notas
    const QString notesFirst = notes.section('\n', 0, 0).trimmed().left(200);

    // Notas como checklist
    QStringList checkLines;
    for (const QString& l : notes.split('\n')) {
        const QString t = l.trimmed();
        if (!t.isEmpty()) checkLines << "- [ ] " + t;
    }
    const QString notesCheck = checkLines.join('\n');

    // Tags como hashtags
    QStringList hashTags;
    for (const QString& tag : m_entry->tags().split(',', Qt::SkipEmptyParts))
        hashTags << "#" + tag.simplified().replace(' ', '_');
    const QString tagsHash = hashTags.join(' ');

    QString result = tmpl;
    result.replace("{title}",          title);
    result.replace("{url}",            url);
    result.replace("{group}",          group);
    result.replace("{tags}",           tags);
    result.replace("{tags_hash}",      tagsHash);
    result.replace("{notes}",          notes);
    result.replace("{notes_first}",    notesFirst);
    result.replace("{notes_checklist}",notesCheck);
    return result;
}

// ─── Slots ────────────────────────────────────────────────────────────────────
void CreationDialog::onTemplateChanged(int index)
{
    if (index < 0 || index >= TEMPLATES.size()) return;
    const auto& t = TEMPLATES[index];
    m_statusLabel->setText(QStringLiteral(
        "<small>%1 %2%3</small>")
        .arg(t.icon, t.description,
             t.needsAI ? tr(" &nbsp;—&nbsp; <span style='color:#7c3aed'>"
                            "⚡ Requer CFDM Triplex</span>") : QString()));
    m_btnGenerate->setText(t.needsAI ? tr("⚡ Gerar (requer CFDM Triplex)")
                                     : tr("⚡ Gerar"));
    m_output->clear();
    m_btnCopy->setEnabled(false);
    m_btnInsert->setEnabled(false);
}

void CreationDialog::onGenerate()
{
    const int idx = m_templateCombo->currentIndex();
    if (idx < 0 || idx >= TEMPLATES.size()) return;
    const auto& t = TEMPLATES[idx];

    if (t.needsAI) {
        QMessageBox::information(this, tr("CFDM Triplex — Em breve"),
            tr("<b>%1 %2</b><br><br>"
               "Esta geração por IA faz parte do <b>CFDM Triplex</b> (Feature 019+).<br><br>"
               "O assistente orquestrador receberá os dados desta entrada,<br>"
               "processará com os agentes de IA e devolverá o conteúdo completo.<br><br>"
               "<i>Configure o Assistente Pessoal quando o repositório estiver disponível.</i>")
            .arg(t.icon, t.name));
        return;
    }

    const QString result = applyTemplate(t.body);
    m_output->setPlainText(result);
    m_btnCopy->setEnabled(true);
    m_btnInsert->setEnabled(true);
    m_statusLabel->setText(tr("<small style='color:#22c55e;'>✔ Gerado com sucesso.</small>"));
}

void CreationDialog::onCopy()
{
    QApplication::clipboard()->setText(m_output->toPlainText());
    m_statusLabel->setText(tr("<small style='color:#22c55e;'>✔ Copiado para a área de transferência.</small>"));
}

void CreationDialog::onInsertInNote()
{
    const QString text = m_output->toPlainText().trimmed();
    if (text.isEmpty()) return;

    // Procura NoteEditorWidget na hierarquia pai
    QWidget* w = parentWidget();
    while (w) {
        auto* ned = w->findChild<NoteEditorWidget*>();
        if (ned) { ned->appendText(text); break; }
        w = w->parentWidget();
    }
    m_statusLabel->setText(tr("<small style='color:#22c55e;'>✔ Inserido na nota da entrada.</small>"));
    accept();
}

void CreationDialog::onAiAction(const QString& action)
{
    Q_UNUSED(action)
}
