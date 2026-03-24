#include "DependencyCheckDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QApplication>
#include <QClipboard>
#include <QProcess>
#include <QSettings>
#include <QLabel>
#include <QScrollArea>
#include <QFrame>
#include <QTimer>

// ─── Definição das dependências ───────────────────────────────────────────────

struct DepDef {
    const char* name;
    const char* feature;
    const char* checkExe;        // executável a testar com --version ou --help
    QStringList checkArgs;       // argumentos do teste
    const char* installCmd;      // comando de instalação recomendado
    bool        required;        // true = obrigatório / false = opcional
    const char* description;     // o que acontece sem ele
};

static QList<DepDef> depDefs()
{
    return {
        {
            "arecord  (alsa-utils)",
            "Capturar Áudio · Assistente de Voz",
            "arecord", {"--version"},
            "sudo apt install alsa-utils",
            false,
            "Sem este pacote não é possível gravar áudio pelo microfone."
        },
        {
            "aplay  (alsa-utils)",
            "Reproduzir Áudio gravado",
            "aplay", {"--version"},
            "sudo apt install alsa-utils",
            false,
            "Sem este pacote não é possível reproduzir o áudio gravado para conferência."
        },
        {
            "python3",
            "Assistente de Voz · Transcrição · Scripts",
            "python3", {"--version"},
            "sudo apt install python3",
            false,
            "Necessário para executar o Whisper (transcrição de voz)."
        },
        {
            "python3 -m whisper  (openai-whisper)",
            "Transcrição de voz → texto",
            "python3", {"-m", "whisper", "--help"},
            "pip install openai-whisper",
            false,
            "Sem Whisper o Assistente de Voz e Capturar Áudio não conseguem converter fala em texto. "
            "O modelo base (~140 MB) é baixado automaticamente na primeira transcrição."
        },
        {
            "whisper-cli  (whisper.cpp)",
            "Transcrição de voz → texto (alternativa rápida)",
            "whisper-cli", {"--help"},
            "# Instalar whisper.cpp: https://github.com/ggerganov/whisper.cpp",
            false,
            "Alternativa mais rápida ao openai-whisper. Opcional se openai-whisper estiver instalado."
        },
        {
            "ffmpeg",
            "Conversão de áudio (compatibilidade)",
            "ffmpeg", {"-version"},
            "sudo apt install ffmpeg",
            false,
            "Melhora a compatibilidade de formatos de áudio. Recomendado mas não obrigatório."
        },
        {
            "xclip  ou  xsel",
            "Copiar para área de transferência",
            "xclip", {"-version"},
            "sudo apt install xclip",
            false,
            "Usado por algumas funções de cópia rápida no Linux. "
            "O Qt tem fallback interno, então a maioria das funções continua funcionando."
        },
        {
            "tesseract-ocr",
            "OCR — extrair texto de imagens (feature futura)",
            "tesseract", {"--version"},
            "sudo apt install tesseract-ocr tesseract-ocr-por",
            false,
            "Necessário para a feature de OCR (leitura de texto em screenshots/fotos). "
            "Ainda não implementada — instale quando a feature for lançada."
        },
    };
}

// ─── Checagem ─────────────────────────────────────────────────────────────────

QList<DependencyCheckDialog::DepResult> DependencyCheckDialog::check()
{
    QList<DepResult> results;
    for (const auto& d : depDefs()) {
        DepResult r;
        r.name       = d.name;
        r.feature    = d.feature;
        r.installCmd = d.installCmd;
        r.required   = d.required;
        r.detail     = d.description;

        QProcess p;
        p.start(d.checkExe, d.checkArgs);
        r.found = p.waitForFinished(3000) && (p.exitCode() == 0 || p.exitCode() == 1);
        if (r.found) {
            // Tenta capturar versão da primeira linha de saída
            const QString out = QString::fromUtf8(
                p.readAllStandardOutput() + p.readAllStandardError()).trimmed();
            const QString firstLine = out.split('\n').first().left(60);
            if (!firstLine.isEmpty()) r.detail = firstLine;
        }
        results << r;
        QApplication::processEvents();
    }
    return results;
}

bool DependencyCheckDialog::allRequiredPresent()
{
    for (const auto& r : check())
        if (r.required && !r.found) return false;
    return true;
}

// ─── Construtor ───────────────────────────────────────────────────────────────

DependencyCheckDialog::DependencyCheckDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("🔍 Verificação de Dependências — CfdmNote"));
    resize(820, 520);

    auto* lay = new QVBoxLayout(this);
    lay->setSpacing(10);
    lay->setContentsMargins(14, 14, 14, 14);

    // Cabeçalho
    auto* hdr = new QLabel(tr(
        "<b>🔍 Verificação de Dependências</b><br>"
        "<small style='color:#4a5568'>O CfdmNote verifica se todos os programas auxiliares "
        "estão instalados. Itens opcionais só afetam features específicas.</small>"));
    hdr->setStyleSheet("font-size:12px; color:#2c5282;");
    hdr->setWordWrap(true);
    lay->addWidget(hdr);

    // Progresso (visível durante a checagem)
    m_progress = new QProgressBar();
    m_progress->setRange(0, 0);  // indeterminado
    m_progress->setFixedHeight(6);
    m_progress->setStyleSheet("QProgressBar{border:none; background:#e2e8f0; border-radius:3px;}"
                               "QProgressBar::chunk{background:#3182ce; border-radius:3px;}");
    lay->addWidget(m_progress);

    // Tabela
    m_table = new QTableWidget(0, 4);
    m_table->setHorizontalHeaderLabels({
        tr("Status"), tr("Dependência"), tr("Feature habilitada"), tr("Instalar com")});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->setColumnWidth(0, 64);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setStyleSheet(
        "QTableWidget{font-size:11px;}"
        "QHeaderView::section{background:#edf2f7; font-weight:bold; padding:4px; border:none;"
        "  border-bottom:2px solid #cbd5e0;}"
        "QTableWidget::item{padding:4px 6px;}");
    lay->addWidget(m_table);

    // Resumo
    m_summary = new QLabel();
    m_summary->setWordWrap(true);
    m_summary->setStyleSheet("font-size:11px; padding:6px 8px; border-radius:4px;");
    lay->addWidget(m_summary);

    // Rodapé
    auto* foot = new QHBoxLayout();
    m_btnCopy = new QPushButton(tr("📋 Copiar comandos faltando"));
    m_btnCopy->setStyleSheet(
        "QPushButton{background:#3182ce;color:white;border-radius:4px;padding:5px 14px;}"
        "QPushButton:hover{background:#2b6cb0;}");
    m_btnClose = new QPushButton(tr("Fechar"));
    m_btnClose->setFixedHeight(30);
    foot->addWidget(m_btnCopy);
    foot->addStretch();
    foot->addWidget(m_btnClose);
    lay->addLayout(foot);

    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_btnCopy, &QPushButton::clicked, this, [this]() {
        QStringList cmds;
        for (int row = 0; row < m_table->rowCount(); ++row) {
            auto* statusItem = m_table->item(row, 0);
            auto* cmdItem    = m_table->item(row, 3);
            if (!statusItem || !cmdItem) continue;
            const QString status = statusItem->text();
            const QString cmd    = cmdItem->text().trimmed();
            // Copia qualquer item faltando (❌ obrigatório ou ⚠ opcional), exceto "—" e comentários
            if ((status.contains("❌") || status.contains("⚠")) &&
                cmd != "—" && !cmd.startsWith("#") && !cmd.isEmpty())
                cmds << cmd;
        }
        if (cmds.isEmpty()) {
            m_btnCopy->setText(tr("📋 Nada para copiar"));
        } else {
            QApplication::clipboard()->setText(cmds.join("\n"));
            m_btnCopy->setText(tr("✅ Copiado! (%1 comando(s))").arg(cmds.size()));
            m_btnCopy->setStyleSheet(
                "QPushButton{background:#276749;color:white;border-radius:4px;padding:5px 14px;}"
                "QPushButton:hover{background:#1c4532;}");
        }
        // Restaura o botão após 3 segundos
        QTimer::singleShot(3000, m_btnCopy, [this]() {
            m_btnCopy->setText(tr("📋 Copiar comandos faltando"));
            m_btnCopy->setStyleSheet(
                "QPushButton{background:#3182ce;color:white;border-radius:4px;padding:5px 14px;}"
                "QPushButton:hover{background:#2b6cb0;}");
        });
    });

    // Executa checagem
    runChecks();
}

void DependencyCheckDialog::runChecks()
{
    m_progress->setVisible(true);
    m_table->setRowCount(0);
    QApplication::processEvents();

    const auto results = check();

    m_progress->setVisible(false);

    int missing = 0, missingRequired = 0;

    for (const auto& r : results) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);

        // Coluna 0 — Status
        QString statusText;
        QString statusColor;
        if (r.found) {
            statusText  = "  ✅";
            statusColor = "#276749";
        } else if (r.required) {
            statusText  = "  ❌";
            statusColor = "#c53030";
            ++missing; ++missingRequired;
        } else {
            statusText  = "  ⚠";
            statusColor = "#c05621";
            ++missing;
        }
        auto* statusItem = new QTableWidgetItem(statusText);
        statusItem->setForeground(QColor(statusColor));
        statusItem->setFont([]{ QFont f; f.setPointSize(13); return f; }());
        statusItem->setToolTip(r.detail);
        m_table->setItem(row, 0, statusItem);

        // Coluna 1 — Nome
        auto* nameItem = new QTableWidgetItem(r.name);
        nameItem->setToolTip(r.detail);
        if (!r.found) {
            QFont f = nameItem->font();
            f.setItalic(true);
            nameItem->setFont(f);
        }
        m_table->setItem(row, 1, nameItem);

        // Coluna 2 — Feature
        auto* featureItem = new QTableWidgetItem(r.feature);
        featureItem->setForeground(QColor(r.found ? "#2c5282" : "#744210"));
        m_table->setItem(row, 2, featureItem);

        // Coluna 3 — Instalar
        auto* cmdItem = new QTableWidgetItem(r.found ? tr("—") : r.installCmd);
        if (!r.found && !r.installCmd.startsWith("#")) {
            cmdItem->setForeground(QColor("#553c9a"));
            QFont f = cmdItem->font();
            f.setFamily("monospace");
            cmdItem->setFont(f);
        } else {
            cmdItem->setForeground(QColor("#718096"));
        }
        m_table->setItem(row, 3, cmdItem);

        // Cor de fundo da linha
        const QColor rowBg = r.found ? QColor("#f0fff4")
                           : r.required ? QColor("#fff5f5")
                                        : QColor("#fffaf0");
        for (int col = 0; col < 4; ++col) {
            if (m_table->item(row, col))
                m_table->item(row, col)->setBackground(rowBg);
        }
    }

    m_table->resizeRowsToContents();

    // Resumo
    if (missing == 0) {
        m_summary->setText(tr("✅  Tudo instalado! Todas as features do CfdmNote estão disponíveis."));
        m_summary->setStyleSheet("font-size:11px; background:#f0fff4; color:#276749; "
                                  "padding:6px 10px; border-radius:4px; border:1px solid #9ae6b4;");
        m_btnCopy->setEnabled(false);
    } else if (missingRequired > 0) {
        m_summary->setText(tr(
            "❌  %1 dependência(s) obrigatória(s) faltando — algumas funções essenciais não vão funcionar.\n"
            "⚠  %2 dependência(s) opcional(is) ausente(s) — features específicas desabilitadas.")
            .arg(missingRequired).arg(missing - missingRequired));
        m_summary->setStyleSheet("font-size:11px; background:#fff5f5; color:#c53030; "
                                  "padding:6px 10px; border-radius:4px; border:1px solid #fc8181;");
    } else {
        m_summary->setText(tr(
            "⚠  %1 dependência(s) opcional(is) ausente(s) — features específicas desabilitadas.\n"
            "Clique em 'Copiar comandos' para instalar tudo de uma vez.")
            .arg(missing));
        m_summary->setStyleSheet("font-size:11px; background:#fffaf0; color:#c05621; "
                                  "padding:6px 10px; border-radius:4px; border:1px solid #f6ad55;");
    }
}

// ─── API pública ──────────────────────────────────────────────────────────────

void DependencyCheckDialog::checkOnStartup(QWidget* parent)
{
    QSettings s("CfdmNote", "CfdmNote");
    const bool firstRun = !s.value("startup/depcheck_done", false).toBool();

    // Verifica rapidamente se há algo faltando
    bool hasIssue = false;
    for (const auto& r : check())
        if (!r.found) { hasIssue = true; break; }

    if (firstRun || hasIssue) {
        s.setValue("startup/depcheck_done", true);
        DependencyCheckDialog dlg(parent);
        dlg.exec();
    }
}

void DependencyCheckDialog::showFull(QWidget* parent)
{
    DependencyCheckDialog dlg(parent);
    dlg.exec();
}
