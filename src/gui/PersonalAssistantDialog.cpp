#include "PersonalAssistantDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGroupBox>
#include <QFrame>
#include <QDesktopServices>
#include <QUrl>

#include "core/Entry.h"
#include "core/Database.h"

// ─── Static launcher ──────────────────────────────────────────────────────────
void PersonalAssistantDialog::show(Entry* entry, QSharedPointer<Database> db, QWidget* parent)
{
    auto* dlg = new PersonalAssistantDialog(entry, db, parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
}

// ─── Construtor ──────────────────────────────────────────────────────────────
PersonalAssistantDialog::PersonalAssistantDialog(Entry* entry,
                                                 QSharedPointer<Database> db,
                                                 QWidget* parent)
    : QDialog(parent), m_entry(entry), m_db(db)
{
    setWindowTitle(tr("🤖 Assistente Pessoal — CFDM Triplex"));
    setMinimumSize(560, 440);
    buildUI();
}

void PersonalAssistantDialog::buildUI()
{
    auto* root = new QVBoxLayout(this);
    root->setSpacing(14);
    root->setContentsMargins(20, 20, 20, 20);

    // ── Cabeçalho ──────────────────────────────────────────────────────────
    auto* hdr = new QLabel(tr(
        "<b style='font-size:16px;'>🤖 Assistente Pessoal</b><br>"
        "<small style='color:#6b7280;'>Orquestrador CFDM Triplex — coordena agentes de IA</small>"));
    hdr->setTextFormat(Qt::RichText);
    root->addWidget(hdr);

    auto* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #e5e7eb;");
    root->addWidget(line);

    // ── Status badge ───────────────────────────────────────────────────────
    auto* badge = new QLabel(tr(
        "<div style='background:#fef3c7; border:1px solid #f59e0b; border-radius:6px; "
        "padding:10px 14px;'>"
        "<b style='color:#92400e;'>⏳ Em desenvolvimento — aguardando repositório</b><br>"
        "<span style='color:#78350f; font-size:12px;'>"
        "O repositório GitHub do CFDM Triplex será cedido em breve. "
        "Assim que disponível, configure-o abaixo para ativar o Assistente Pessoal."
        "</span></div>"));
    badge->setTextFormat(Qt::RichText);
    badge->setWordWrap(true);
    root->addWidget(badge);

    // ── O que o Assistente vai fazer ──────────────────────────────────────
    auto* grpFeatures = new QGroupBox(tr("O que o Assistente Pessoal fará"));
    auto* featLay = new QVBoxLayout(grpFeatures);
    auto* featLabel = new QLabel(tr(
        "<ul style='margin:4px 0; padding-left:20px; line-height:1.7;'>"
        "<li>🧠 <b>Orquestrar agentes de IA</b> a partir dos dados das entradas</li>"
        "<li>📖 Gerar <b>ebooks completos</b> com capítulos, introdução e conclusão</li>"
        "<li>💎 Criar <b>descrições de produtos digitais</b> (copy, bullet points, CTA)</li>"
        "<li>📨 Montar <b>campanhas de email marketing</b> (boas-vindas, nutrição, oferta)</li>"
        "<li>📅 Produzir <b>plano de conteúdo 30 dias</b> (temas, formatos, canais)</li>"
        "<li>🤝 Redigir <b>propostas comerciais</b> com escopo, prazo e valores</li>"
        "<li>🎧 Escrever <b>scripts de audiobook</b> otimizados para narração</li>"
        "<li>🔗 Coordenar com outras features do <b>CFDM Triplex</b></li>"
        "</ul>"));
    featLabel->setTextFormat(Qt::RichText);
    featLay->addWidget(featLabel);
    root->addWidget(grpFeatures);

    // ── Configurar repositório ─────────────────────────────────────────────
    auto* grpRepo = new QGroupBox(tr("Configurar Repositório GitHub"));
    grpRepo->setEnabled(false);   // desabilitado até o repo estar disponível
    auto* repoLay = new QVBoxLayout(grpRepo);
    repoLay->addWidget(new QLabel(tr("URL do repositório CFDM Triplex:")));
    auto* repoEdit = new QLineEdit();
    repoEdit->setPlaceholderText(tr("https://github.com/cfdmmarket-dot/cfdm-triplex  (em breve)"));
    repoLay->addWidget(repoEdit);
    auto* btnConfigure = new QPushButton(tr("⚡ Ativar Assistente"));
    btnConfigure->setEnabled(false);
    repoLay->addWidget(btnConfigure);
    root->addWidget(grpRepo);

    root->addStretch();

    // ── Rodapé ────────────────────────────────────────────────────────────
    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto* btnClose = new QPushButton(tr("Fechar"));
    btnClose->setDefault(true);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    btnRow->addWidget(btnClose);
    root->addLayout(btnRow);
}
