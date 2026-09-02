#include "AudioCaptureDialog.h"

#include <QtMath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFrame>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QMessageBox>
#include <QTextCursor>
#include <QPainter>
#include <QMenu>
#include <QAction>
#include <QScrollBar>
#include <QRegularExpression>
#include <QToolButton>
#include <QSizePolicy>

#include "core/Entry.h"
#include "core/Database.h"
#include "core/EntryAttributes.h"
#include "notes/NoteManager.h"
#include "gui/AttachmentsPanelWidget.h"
#include "gui/ToastNotification.h"

static const QString WAV_PATH =
    QDir::tempPath() + "/cfdmnote_audio_capture.wav";

// ═════════════════════════════════════════════════════════════════════════════
// WaveformWidget
// ═════════════════════════════════════════════════════════════════════════════

WaveformWidget::WaveformWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(64);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setStyleSheet("background: #0f1723; border-radius: 6px;");
    connect(&m_timer, &QTimer::timeout, this, &WaveformWidget::tick);
}

void WaveformWidget::startMonitoring(const QString& wavPath)
{
    m_wavPath = wavPath;
    m_active  = true;
    m_levels.clear();
    m_timer.start(120);  // ~8 fps
}

void WaveformWidget::stopMonitoring()
{
    m_active = false;
    m_timer.stop();
    update();
}

void WaveformWidget::clear()
{
    m_levels.clear();
    m_active = false;
    m_timer.stop();
    update();
}

void WaveformWidget::tick()
{
    const float rms = readLastRms();
    m_levels.append(rms);
    if (m_levels.size() > MAX_BARS)
        m_levels.removeFirst();
    update();
}

float WaveformWidget::readLastRms() const
{
    QFile f(m_wavPath);
    if (!f.open(QIODevice::ReadOnly)) return 0.0f;

    // WAV PCM header = 44 bytes; samples = S16_LE
    const qint64 fileSize = f.size();
    const qint64 dataSize = fileSize - 44;
    if (dataSize <= 0) { f.close(); return 0.0f; }

    // Últimos 100ms = 16000 Hz * 0.1s * 2 bytes = 3200 bytes
    const qint64 chunkBytes = qMin(dataSize, qint64(3200));
    f.seek(fileSize - chunkBytes);
    const QByteArray raw = f.read(chunkBytes);
    f.close();

    if (raw.size() < 2) return 0.0f;

    double sum = 0.0;
    const int n = raw.size() / 2;
    const char* d = raw.constData();
    for (int i = 0; i < n; ++i) {
        qint16 s;
        memcpy(&s, d + i * 2, 2);
        sum += (double)s * s;
    }
    return (float)(qSqrt(sum / n) / 32768.0);
}

void WaveformWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#0f1723"));

    if (m_levels.isEmpty()) {
        p.setPen(QColor("#2d4a6e"));
        p.drawText(rect(), Qt::AlignCenter, tr("Aguardando gravação..."));
        return;
    }

    const int w = width();
    const int h = height();
    const int n = m_levels.size();
    const float barW = qMax(2.0f, (float)w / MAX_BARS);
    const float gap  = qMax(1.0f, barW * 0.15f);
    const float silenceThreshold = 0.015f;

    for (int i = 0; i < n; ++i) {
        const float level = m_levels.at(i);
        const float barH  = qMax(2.0f, level * (h - 8) * 2.8f);
        const float x     = (float)(MAX_BARS - n + i) * barW + gap;
        const float y     = (h - barH) / 2.0f;

        // Cor: silêncio=azul escuro, normal=ciano, alto=amarelo, clip=vermelho
        QColor color;
        if (level < silenceThreshold)       color = QColor("#1a3a5c");  // silêncio
        else if (level < 0.3f)              color = QColor("#00c8ff");  // normal
        else if (level < 0.7f)              color = QColor("#f6c90e");  // alto
        else                                color = QColor("#e53e3e");  // clip

        p.fillRect(QRectF(x, y, barW - gap, barH),
                   QBrush(color));

        // Reflexo abaixo da linha central
        QColor dim = color;
        dim.setAlphaF(0.25f);
        p.fillRect(QRectF(x, (float)h/2 + barH/2, barW - gap, barH * 0.3f),
                   QBrush(dim));
    }

    // Linha central
    p.setPen(QPen(QColor("#2d4a6e"), 1));
    p.drawLine(0, h / 2, w, h / 2);

    // Indicador ao vivo
    if (m_active) {
        p.setPen(QColor("#e53e3e"));
        p.setBrush(QColor("#e53e3e"));
        p.drawEllipse(QRectF(w - 14, 6, 8, 8));
        p.setPen(QColor("#ffffff"));
        p.setFont(QFont("monospace", 7));
        p.drawText(QRectF(w - 48, 4, 32, 14), Qt::AlignRight, "REC");
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// SpellHighlighter
// ═════════════════════════════════════════════════════════════════════════════

SpellHighlighter::SpellHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    m_errorFmt.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    m_errorFmt.setUnderlineColor(QColor("#e53e3e"));

    // Testa se aspell está disponível
    QProcess test;
    test.start("aspell", {"--version"});
    m_available = test.waitForFinished(2000) && test.exitCode() == 0;

    if (m_available) {
        m_proc.start("aspell", {"-a", "--lang=pt_BR"});
        if (!m_proc.waitForStarted(2000)) {
            // tenta inglês
            m_proc.kill();
            m_proc.start("aspell", {"-a"});
            m_available = m_proc.waitForStarted(2000);
        }
        if (m_available) {
            m_proc.readLine(); // consome linha de banner "@(#) International Ispell..."
        }
    }
}

bool SpellHighlighter::checkWord(const QString& word) const
{
    if (!m_available || word.length() < 2) return true;
    // Números e palavras com dígitos são ignorados
    if (word.contains(QRegularExpression("\\d"))) return true;

    const QString cmd = word + "\n";
    const_cast<QProcess&>(m_proc).write(cmd.toUtf8());
    const_cast<QProcess&>(m_proc).waitForReadyRead(500);
    const QString resp = QString::fromUtf8(
        const_cast<QProcess&>(m_proc).readLine()).trimmed();

    // aspell: '*' = correto, '&' ou '#' = incorreto
    return resp.startsWith('*') || resp.startsWith('+') || resp.isEmpty();
}

QStringList SpellHighlighter::suggestionsFor(const QString& word) const
{
    if (!m_available) return {};
    const QString cmd = word + "\n";
    const_cast<QProcess&>(m_proc).write(cmd.toUtf8());
    const_cast<QProcess&>(m_proc).waitForReadyRead(500);
    const QString resp = QString::fromUtf8(
        const_cast<QProcess&>(m_proc).readLine()).trimmed();
    // Formato: "& word count offset: sug1, sug2, ..."
    if (!resp.startsWith('&')) return {};
    const int colon = resp.indexOf(':');
    if (colon < 0) return {};
    return resp.mid(colon + 2).split(", ", Qt::SkipEmptyParts).mid(0, 8);
}

void SpellHighlighter::highlightBlock(const QString& text)
{
    if (!m_available) return;

    // Itera pelas palavras do bloco
    static QRegularExpression wordRe("\\b[\\p{L}'-]{2,}\\b");
    auto it = wordRe.globalMatch(text);
    while (it.hasNext()) {
        auto m = it.next();
        if (!checkWord(m.captured())) {
            setFormat(m.capturedStart(), m.capturedLength(), m_errorFmt);
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// AudioCaptureDialog
// ═════════════════════════════════════════════════════════════════════════════

AudioCaptureDialog::AudioCaptureDialog(Entry* entry,
                                        QSharedPointer<Database> db,
                                        QWidget* parent)
    : QDialog(parent)
    , m_entry(entry)
    , m_db(db)
{
    setWindowTitle(tr("Capturar Áudio / Transcrição"));
    setMinimumSize(640, 680);
    resize(700, 740);

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    layout->setContentsMargins(12, 12, 12, 12);

    // ── Cabeçalho ─────────────────────────────────────────────────────────────
    auto* header = new QLabel(
        entry
            ? tr("<b>🎤 Captura de Áudio</b>  —  <i>%1</i>")
                 .arg(entry->title().toHtmlEscaped())
            : tr("<b>🎤 Captura de Áudio</b>"));
    header->setStyleSheet("font-size: 13px; color: #1a365d; padding: 2px 0;");
    layout->addWidget(header);

    // ═══════════════════════════════════════════════════════════════════════
    // SEÇÃO GRAVAÇÃO
    // ═══════════════════════════════════════════════════════════════════════
    auto* grpRec = new QGroupBox(tr("Gravação"));
    auto* recLayout = new QVBoxLayout(grpRec);
    recLayout->setSpacing(6);

    // Waveform
    m_waveform = new WaveformWidget(this);
    recLayout->addWidget(m_waveform);

    // Timer + botões
    auto* recRow = new QHBoxLayout();
    m_timerLabel = new QLabel("00:00");
    m_timerLabel->setFixedWidth(72);
    m_timerLabel->setAlignment(Qt::AlignCenter);
    m_timerLabel->setStyleSheet(
        "font-size: 26px; font-family: monospace; font-weight: bold;"
        "color: #1a365d;");
    recRow->addWidget(m_timerLabel);

    m_btnRecord = new QPushButton(tr("⏺ Gravar"));
    m_btnRecord->setFixedHeight(34);
    m_btnRecord->setStyleSheet(
        "QPushButton{background:#e53e3e;color:white;font-weight:bold;"
        "border-radius:6px;padding:0 18px;font-size:13px;}"
        "QPushButton:hover{background:#c53030;}"
        "QPushButton:disabled{background:#fed7d7;color:#fc8181;}");

    m_btnStop = new QPushButton(tr("⏹ Parar"));
    m_btnStop->setFixedHeight(34);
    m_btnStop->setEnabled(false);
    m_btnStop->setStyleSheet(
        "QPushButton{background:#718096;color:white;font-weight:bold;"
        "border-radius:6px;padding:0 18px;font-size:13px;}"
        "QPushButton:hover{background:#4a5568;}"
        "QPushButton:disabled{background:#e2e8f0;color:#a0aec0;}");

    m_btnPlay = new QPushButton(tr("▶ Reproduzir"));
    m_btnPlay->setFixedHeight(34);
    m_btnPlay->setEnabled(false);
    m_btnPlay->setStyleSheet(
        "QPushButton{background:#38a169;color:white;font-weight:bold;"
        "border-radius:6px;padding:0 18px;font-size:13px;}"
        "QPushButton:hover{background:#276749;}"
        "QPushButton:disabled{background:#c6f6d5;color:#68d391;}");

    recRow->addWidget(m_btnRecord);
    recRow->addWidget(m_btnStop);
    recRow->addWidget(m_btnPlay);
    recRow->addStretch();
    recLayout->addLayout(recRow);

    // Info do arquivo gravado
    m_fileInfoLabel = new QLabel(tr("Nenhum arquivo gravado ainda."));
    m_fileInfoLabel->setStyleSheet(
        "font-size: 10px; color: #4a5568; font-family: monospace;"
        "padding: 2px 4px; background: #f7fafc; border-radius: 3px;");
    m_fileInfoLabel->setWordWrap(true);
    recLayout->addWidget(m_fileInfoLabel);

    layout->addWidget(grpRec);

    // ═══════════════════════════════════════════════════════════════════════
    // SEÇÃO TRANSCRIÇÃO
    // ═══════════════════════════════════════════════════════════════════════
    auto* grpTransc = new QGroupBox(tr("Transcrição"));
    auto* transcLayout = new QVBoxLayout(grpTransc);
    transcLayout->setSpacing(6);

    // Linha de opções
    auto* optsRow = new QHBoxLayout();
    optsRow->addWidget(new QLabel(tr("Modelo:")));
    m_modelCombo = new QComboBox();
    m_modelCombo->addItems({"tiny", "base", "small", "medium", "large"});
    m_modelCombo->setCurrentText("base");
    m_modelCombo->setToolTip(
        tr("tiny=rápido/menos preciso  |  large=lento/mais preciso\n"
           "Recomendado para PT-BR: base ou small"));
    optsRow->addWidget(m_modelCombo);
    optsRow->addSpacing(10);

    optsRow->addWidget(new QLabel(tr("Idioma:")));
    m_langCombo = new QComboBox();
    m_langCombo->addItems({
        "Portuguese","English","Spanish","French",
        "German","Italian","Japanese","Chinese","auto"
    });
    optsRow->addWidget(m_langCombo);
    optsRow->addSpacing(10);

    m_btnTranscribe = new QPushButton(tr("🔤 Transcrever"));
    m_btnTranscribe->setFixedHeight(30);
    m_btnTranscribe->setEnabled(false);
    m_btnTranscribe->setStyleSheet(
        "QPushButton{background:#3182ce;color:white;font-weight:bold;"
        "border-radius:5px;padding:0 14px;}"
        "QPushButton:hover{background:#2b6cb0;}"
        "QPushButton:disabled{background:#bee3f8;color:#90cdf4;}");
    optsRow->addWidget(m_btnTranscribe);
    optsRow->addSpacing(8);
    m_chkAutoTranscribe = new QCheckBox(tr("Auto ao parar"));
    m_chkAutoTranscribe->setToolTip(tr(
        "Inicia a transcrição automaticamente quando a gravação terminar"));
    m_chkAutoTranscribe->setChecked(true);
    optsRow->addWidget(m_chkAutoTranscribe);
    optsRow->addStretch();
    transcLayout->addLayout(optsRow);

    // Área de texto com corretor
    m_transcription = new QTextEdit();
    m_transcription->setMinimumHeight(160);
    m_transcription->setPlaceholderText(
        tr("O texto transcrito aparecerá aqui.\n"
           "Você pode editar livremente — palavras sublinhadas em vermelho têm sugestões (clique direito)."));
    m_transcription->setStyleSheet(
        "QTextEdit{font-size:13px;line-height:1.5;"
        "border:1px solid #cbd5e0;border-radius:5px;padding:6px;}");
    m_transcription->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_transcription, &QTextEdit::customContextMenuRequested,
            this, &AudioCaptureDialog::showSuggestions);
    transcLayout->addWidget(m_transcription);

    // Barra de ferramentas do texto
    auto* textTools = new QHBoxLayout();
    m_btnSpell = new QPushButton(tr("✓ Verificar Ortografia"));
    m_btnSpell->setFixedHeight(26);
    m_btnSpell->setStyleSheet(
        "QPushButton{font-size:11px;color:#4a5568;border:1px solid #cbd5e0;"
        "border-radius:4px;padding:0 10px;}"
        "QPushButton:hover{background:#edf2f7;}");
    connect(m_btnSpell, &QPushButton::clicked, this, &AudioCaptureDialog::onSpellCheck);
    textTools->addWidget(m_btnSpell);

    auto* btnClear = new QPushButton(tr("🗑 Limpar texto"));
    btnClear->setFixedHeight(26);
    btnClear->setStyleSheet(m_btnSpell->styleSheet());
    connect(btnClear, &QPushButton::clicked, m_transcription, &QTextEdit::clear);
    textTools->addWidget(btnClear);
    textTools->addStretch();
    transcLayout->addLayout(textTools);

    layout->addWidget(grpTransc);

    // ═══════════════════════════════════════════════════════════════════════
    // SEÇÃO IA — CFDM Triplex
    // ═══════════════════════════════════════════════════════════════════════
    auto* grpAI = new QGroupBox(tr("🤖 Gerar com IA  —  CFDM Triplex"));
    grpAI->setStyleSheet(
        "QGroupBox{font-weight:bold;color:#553c9a;"
        "border:1px solid #9f7aea;border-radius:6px;margin-top:6px;padding-top:6px;}"
        "QGroupBox::title{subcontrol-origin:margin;left:8px;color:#553c9a;}");
    auto* aiLayout = new QVBoxLayout(grpAI);
    aiLayout->setSpacing(6);

    auto* aiDesc = new QLabel(
        tr("Use a transcrição como base para criar conteúdo com IA:"));
    aiDesc->setStyleSheet("font-size:11px;color:#4a5568;font-weight:normal;");
    aiLayout->addWidget(aiDesc);

    auto* aiRow = new QHBoxLayout();
    const QStringList aiActions = {
        "📖 Ebook", "🎧 Audiobook", "📦 Produto Digital",
        "✍️ Escrever Texto", "📋 Resumo"
    };
    for (const QString& label : aiActions) {
        auto* btn = new QPushButton(label);
        btn->setFixedHeight(30);
        btn->setStyleSheet(
            "QPushButton{font-size:11px;background:#f3ebff;"
            "color:#553c9a;border:1px solid #9f7aea;"
            "border-radius:5px;padding:0 10px;font-weight:bold;}"
            "QPushButton:hover{background:#e9d8fd;}"
            "QPushButton:pressed{background:#d6bcfa;}");
        const QString action = label;
        connect(btn, &QPushButton::clicked, this, [this, action]() {
            onAiAction(action);
        });
        aiRow->addWidget(btn);
    }
    aiRow->addStretch();
    aiLayout->addLayout(aiRow);
    layout->addWidget(grpAI);

    // ═══════════════════════════════════════════════════════════════════════
    // STATUS + RODAPÉ
    // ═══════════════════════════════════════════════════════════════════════
    m_statusLabel = new QLabel(tr("Pronto. Clique em ⏺ Gravar para começar."));
    m_statusLabel->setStyleSheet(
        "font-size:11px;color:#4a5568;padding:2px 4px;"
        "background:#f7fafc;border-radius:3px;");
    m_statusLabel->setWordWrap(true);
    layout->addWidget(m_statusLabel);

    auto* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    auto* footRow = new QHBoxLayout();
    m_btnInsert = new QPushButton(tr("📝 Inserir na Nota"));
    m_btnInsert->setFixedHeight(32);
    m_btnInsert->setEnabled(false);
    m_btnInsert->setStyleSheet(
        "QPushButton{background:#2b6cb0;color:white;font-weight:bold;"
        "border-radius:5px;padding:0 18px;}"
        "QPushButton:hover{background:#1a365d;}"
        "QPushButton:disabled{background:#bee3f8;color:#90cdf4;}");

    auto* btnClose = new QPushButton(tr("Fechar"));
    btnClose->setFixedHeight(32);
    footRow->addWidget(m_btnInsert);
    footRow->addStretch();
    footRow->addWidget(btnClose);
    layout->addLayout(footRow);

    // ── Timer + Corretor ─────────────────────────────────────────────────
    m_tickTimer = new QTimer(this);
    m_tickTimer->setInterval(1000);

    // Spell highlighter (não-bloqueante — só destaca após digitação)
    m_spell = new SpellHighlighter(m_transcription->document());
    if (!m_spell->isAvailable()) {
        m_btnSpell->setToolTip(
            tr("aspell não encontrado.\nInstale: sudo apt install aspell aspell-pt"));
    }

    // ── Conexões ─────────────────────────────────────────────────────────
    connect(m_btnRecord,    &QPushButton::clicked, this, &AudioCaptureDialog::onRecord);
    connect(m_btnStop,      &QPushButton::clicked, this, &AudioCaptureDialog::onStop);
    connect(m_btnPlay,      &QPushButton::clicked, this, &AudioCaptureDialog::onPlay);
    connect(m_btnTranscribe,&QPushButton::clicked, this, &AudioCaptureDialog::onTranscribe);
    connect(m_btnInsert,    &QPushButton::clicked, this, &AudioCaptureDialog::onInsertInNote);
    connect(btnClose,       &QPushButton::clicked, this, &QDialog::accept);
    connect(m_tickTimer,    &QTimer::timeout,      this, &AudioCaptureDialog::onTick);
    connect(m_transcription,&QTextEdit::textChanged, this, [this]() {
        m_btnInsert->setEnabled(m_entry &&
            !m_transcription->toPlainText().trimmed().isEmpty());
    });

    // Detecta motor de transcrição disponível
    const QString engine = whisperEngine();
    if (engine.isEmpty()) {
        setStatus(tr("⚠ Whisper não encontrado — gravação funciona, transcrição indisponível."),
                  "#c05621");
        QTimer::singleShot(300, this, [this]() {
            ToastNotification::show(
                parentWidget() ? parentWidget() : this,
                tr("🎤 Transcrição não configurada"),
                tr("Para ativar a transcrição automática, instale o Whisper:"),
                "pip install openai-whisper", 20000);
        });
    } else {
        setStatus(tr("Motor: %1  |  Ortografia: %2")
                      .arg(engine.section('/', -1))
                      .arg(m_spell->isAvailable() ? "aspell ✅" : "aspell não instalado"));
    }

    updateFileInfo();
}

AudioCaptureDialog::~AudioCaptureDialog()
{
    if (m_recProcess  && m_recProcess->state()  != QProcess::NotRunning) {
        m_recProcess->kill(); m_recProcess->waitForFinished(1000);
    }
    if (m_transcProc  && m_transcProc->state()  != QProcess::NotRunning) {
        m_transcProc->kill(); m_transcProc->waitForFinished(1000);
    }
    if (m_playProc    && m_playProc->state()    != QProcess::NotRunning) {
        m_playProc->kill(); m_playProc->waitForFinished(1000);
    }
}

void AudioCaptureDialog::captureAudio(Entry* entry,
                                       QSharedPointer<Database> db,
                                       QWidget* parent)
{
    AudioCaptureDialog dlg(entry, db, parent);
    dlg.exec();
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

// Retorna o caminho completo do executável se existir no PATH
static QString findInPath(const QString& name)
{
    for (const QString& dir : qEnvironmentVariable("PATH").split(':')) {
        const QString full = dir + "/" + name;
        if (QFileInfo(full).isExecutable()) return full;
    }
    return {};
}

QString AudioCaptureDialog::whisperEngine() const
{
    // 1. faster-whisper CLI — preferido: CPU nativo, sem CUDA, sem ffmpeg obrigatório
    if (!findInPath("faster-whisper").isEmpty()) return "faster-whisper";

    // Verifica se faster-whisper está instalado como módulo python
    {
        QProcess p;
        p.start("python3", {"-c", "import faster_whisper; print('ok')"});
        if (p.waitForFinished(4000) && p.exitCode() == 0)
            return "python3 -m faster_whisper";
    }

    // 2. whisper-cli (C++ nativo, sem Python)
    if (!findInPath("whisper-cli").isEmpty()) return "whisper-cli";

    // 3. openai-whisper via python3 -m whisper (verifica APENAS importabilidade)
    {
        QProcess p;
        p.start("python3", {"-c", "import whisper; print('ok')"});
        if (p.waitForFinished(6000) && p.exitCode() == 0)
            return "python3 -m whisper";
    }

    // 4. whisper executável direto no PATH
    if (!findInPath("whisper").isEmpty()) return "whisper";

    return {};
}

QString AudioCaptureDialog::tmpWavPath() const { return WAV_PATH; }

void AudioCaptureDialog::setStatus(const QString& msg, const QString& color)
{
    m_statusLabel->setText(msg);
    m_statusLabel->setStyleSheet(
        QString("font-size:11px;color:%1;padding:2px 4px;"
                "background:#f7fafc;border-radius:3px;").arg(color));
}

void AudioCaptureDialog::updateButtons()
{
    m_btnRecord->setEnabled(!m_recording);
    m_btnStop->setEnabled(m_recording);
    m_btnPlay->setEnabled(m_hasAudio && !m_recording);
    m_btnTranscribe->setEnabled(m_hasAudio && !m_recording);
}

void AudioCaptureDialog::updateFileInfo()
{
    const QFileInfo fi(WAV_PATH);
    if (!fi.exists() || fi.size() < 512) {
        m_fileInfoLabel->setText(tr("Nenhum arquivo gravado ainda."));
        return;
    }
    const double mb = fi.size() / (1024.0 * 1024.0);
    // Duração: (tamanho - 44 bytes header) / (16000 Hz * 2 bytes/sample * 1 canal)
    const double secs = qMax(0.0, (fi.size() - 44) / (16000.0 * 2.0));
    const int m = (int)secs / 60;
    const int s = (int)secs % 60;
    m_fileInfoLabel->setText(
        tr("📁  %1  |  %.2f MB  |  %2:%3")
            .arg(WAV_PATH)
            .arg(mb)
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0')));
}

// ─── Slots ───────────────────────────────────────────────────────────────────

void AudioCaptureDialog::onRecord()
{
    if (m_playProc && m_playProc->state() != QProcess::NotRunning) {
        m_playProc->kill(); m_playProc->waitForFinished(500);
    }
    QFile::remove(WAV_PATH);
    m_waveform->clear();

    m_recProcess = new QProcess(this);
    m_recProcess->start("arecord",
        {"-f", "S16_LE", "-r", "16000", "-c", "1", "-t", "wav", WAV_PATH});

    if (!m_recProcess->waitForStarted(2000)) {
        setStatus(tr("Erro: arecord não encontrado. Instale: sudo apt install alsa-utils"),
                  "#c05621");
        delete m_recProcess; m_recProcess = nullptr;
        return;
    }

    connect(m_recProcess, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code) { onRecordFinished(code); });

    m_elapsed = 0; m_recording = true;
    m_tickTimer->start();
    m_waveform->startMonitoring(WAV_PATH);
    m_timerLabel->setStyleSheet(
        "font-size:26px;font-family:monospace;font-weight:bold;color:#e53e3e;");
    setStatus(tr("⏺ Gravando... fale agora. Clique em ⏹ Parar quando terminar."), "#c53030");
    updateButtons();
}

void AudioCaptureDialog::onStop()
{
    if (m_recProcess && m_recProcess->state() != QProcess::NotRunning) {
        m_recProcess->terminate();
        m_recProcess->waitForFinished(2000);
    }
    m_tickTimer->stop();
    m_waveform->stopMonitoring();
    m_recording = false;
    m_timerLabel->setStyleSheet(
        "font-size:26px;font-family:monospace;font-weight:bold;color:#1a365d;");
    updateButtons();
}

void AudioCaptureDialog::onRecordFinished(int)
{
    m_recording = false;
    m_tickTimer->stop();
    m_waveform->stopMonitoring();
    m_timerLabel->setStyleSheet(
        "font-size:26px;font-family:monospace;font-weight:bold;color:#1a365d;");
    m_hasAudio = QFileInfo(WAV_PATH).size() > 512;
    updateFileInfo();
    if (m_hasAudio) {
        // Salva cópia permanente e registra como anexo da entrada
        if (m_entry) {
            const QString audioDir = QDir::homePath() + "/.local/share/cfdmnote/audio";
            QDir().mkpath(audioDir);
            const QString stamp   = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
            const QString dstPath = audioDir + "/" + m_entry->uuidToHex().left(8)
                                  + "_" + stamp + ".wav";
            if (QFile::copy(WAV_PATH, dstPath)) {
                QString raw = m_entry->attributes()->value(AttachmentsPanelWidget::KEY_AUDIO);
                QStringList lines = raw.split('\n', Qt::SkipEmptyParts);
                lines << dstPath;
                m_entry->attributes()->set(AttachmentsPanelWidget::KEY_AUDIO,
                                           lines.join('\n'), false);
                if (m_db) { QString err; m_db->save(Database::Atomic, {}, &err); }
            }
        }
        setStatus(tr("✅ Gravação salva (%1s). Clique em 🔤 Transcrever ou reproduza para conferir.")
                      .arg(m_elapsed));
    } else {
        setStatus(tr("⚠ Nenhum áudio capturado. Verifique o microfone."), "#c05621");
    }
    updateButtons();
}

void AudioCaptureDialog::onPlay()
{
    if (m_playProc && m_playProc->state() != QProcess::NotRunning) {
        m_playProc->kill(); return;
    }
    m_playProc = new QProcess(this);
    m_playProc->start("aplay", {WAV_PATH});
    setStatus(tr("▶ Reproduzindo..."));
    connect(m_playProc, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this]() { setStatus(tr("Reprodução concluída.")); });
}

void AudioCaptureDialog::onTick()
{
    ++m_elapsed;
    const int m = m_elapsed / 60;
    const int s = m_elapsed % 60;
    m_timerLabel->setText(QString("%1:%2")
        .arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0')));
    if (m_elapsed >= 300) {
        setStatus(tr("⏱ Limite de 5 min — gravação parada automaticamente."));
        onStop();
    }
}

void AudioCaptureDialog::onTranscribe()
{
    if (!m_hasAudio) return;
    const QString engine = whisperEngine();
    if (engine.isEmpty()) {
        setStatus(tr("Motor não encontrado."), "#c05621");
        ToastNotification::show(parentWidget() ? parentWidget() : this,
            tr("🎤 Instale o Whisper"),
            tr("Execute no terminal e reinicie o CfdmNote:"),
            "pip install openai-whisper", 20000);
        return;
    }

    const QString model = m_modelCombo->currentText();
    const QString lang  = m_langCombo->currentText();

    m_btnTranscribe->setEnabled(false);
    setStatus(tr("⏳ Transcrevendo com %1 (modelo %2)…  aguarde.")
                  .arg(engine.section('/', -1), model));

    if (m_transcProc) {
        m_transcProc->disconnect();
        if (m_transcProc->state() != QProcess::NotRunning) {
            m_transcProc->kill(); m_transcProc->waitForFinished(1000);
        }
        m_transcProc->deleteLater(); m_transcProc = nullptr;
    }

    m_transcStderr.clear();
    m_transcProc = new QProcess(this);
    m_transcProc->setWorkingDirectory(QDir::tempPath());
    // Acumula stderr completo (NÃO ler aqui descarta o buffer para onTranscribeFinished)
    connect(m_transcProc, &QProcess::readyReadStandardError, this, [this]() {
        const QString chunk = QString::fromUtf8(m_transcProc->readAllStandardError());
        m_transcStderr += chunk;
        // Mostra a última linha não-vazia no status
        const QStringList lines = chunk.trimmed().split('\n');
        for (int i = lines.size() - 1; i >= 0; --i) {
            const QString t = lines[i].trimmed();
            if (!t.isEmpty()) { setStatus(tr("⏳ %1").arg(t.left(90))); break; }
        }
    });

    // Força CPU — evita CUDA incompatível
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("CUDA_VISIBLE_DEVICES", "");
    env.insert("PYTHONWARNINGS",        "ignore");
    m_transcProc->setProcessEnvironment(env);

    QStringList args;
    const QString langShort = (lang == "auto") ? "auto" : lang.left(2).toLower();

    if (engine == "faster-whisper") {
        // faster-whisper <file> --model <model> --language <lang> --device cpu
        args << WAV_PATH
             << "--model"    << model
             << "--language" << langShort
             << "--device"   << "cpu";
        m_transcProc->start("faster-whisper", args);
    } else if (engine == "python3 -m faster_whisper") {
        args << "-c"
             << QString("from faster_whisper import WhisperModel; "
                        "m=WhisperModel('%1',device='cpu'); "
                        "segs,_=m.transcribe('%2',language='%3'); "
                        "[print(s.text) for s in segs]")
                    .arg(model, WAV_PATH, langShort == "auto" ? "" : langShort);
        m_transcProc->start("python3", args);
    } else if (engine == "whisper-cli") {
        args << "--model"    << QString("ggml-%1.bin").arg(model)
             << "--language" << langShort
             << "--output-txt" << "--file" << WAV_PATH;
        m_transcProc->start("whisper-cli", args);
    } else if (engine == "python3 -m whisper") {
        args << "-m" << "whisper" << WAV_PATH
             << "--model"         << model
             << "--language"      << (lang == "auto" ? "auto" : lang)
             << "--output_format" << "txt"
             << "--output_dir"    << QDir::tempPath()
             << "--device"        << "cpu";
        m_transcProc->start("python3", args);
    } else {
        // whisper binário direto
        args << WAV_PATH
             << "--model"         << model
             << "--language"      << (lang == "auto" ? "auto" : lang)
             << "--output_format" << "txt"
             << "--output_dir"    << QDir::tempPath()
             << "--device"        << "cpu";
        m_transcProc->start(engine, args);
    }

    if (!m_transcProc->waitForStarted(3000)) {
        setStatus(tr("Erro: não foi possível iniciar a transcrição."), "#c05621");
        m_btnTranscribe->setEnabled(true);
    }

    connect(m_transcProc,
            QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code) { onTranscribeFinished(code); });
}

void AudioCaptureDialog::onTranscribeFinished(int exitCode)
{
    m_btnTranscribe->setEnabled(true);

    // Drena qualquer saída restante
    m_transcStderr += QString::fromUtf8(m_transcProc->readAllStandardError());
    const QString stdoutData = QString::fromUtf8(m_transcProc->readAllStandardOutput()).trimmed();

    if (exitCode != 0) {
        setStatus(tr("❌ Erro na transcrição (código %1) — veja detalhes abaixo.").arg(exitCode),
                  "#c05621");
        // Mostra detalhes completos numa caixa com scroll
        QDialog* dlg = new QDialog(this);
        dlg->setWindowTitle(tr("Erro de Transcrição — código %1").arg(exitCode));
        dlg->setMinimumSize(560, 340);
        auto* lay = new QVBoxLayout(dlg);
        lay->addWidget(new QLabel(tr(
            "<b>Saída de erro do Whisper:</b><br>"
            "<small style='color:#718096;'>Copie e envie este log para diagnóstico.</small>")));
        auto* txt = new QTextEdit();
        txt->setReadOnly(true);
        txt->setFontFamily("monospace");
        const QString combined = m_transcStderr.trimmed() + "\n" + stdoutData;
        txt->setPlainText(combined.isEmpty()
            ? tr("(sem saída — whisper saiu silenciosamente com código %1)").arg(exitCode)
            : combined);
        lay->addWidget(txt);
        // Dica de instalação do faster-whisper
        auto* hint = new QLabel(tr(
            "<small><b>Dica:</b> Se o erro menciona CUDA, tente instalar faster-whisper "
            "(mais rápido, sem GPU):<br>"
            "<code>pip install faster-whisper</code></small>"));
        hint->setTextFormat(Qt::RichText);
        hint->setWordWrap(true);
        lay->addWidget(hint);
        auto* btnClose = new QPushButton(tr("Fechar"));
        connect(btnClose, &QPushButton::clicked, dlg, &QDialog::accept);
        lay->addWidget(btnClose);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->exec();
        return;
    }

    // Lê resultado: arquivo .txt gerado, ou stdout
    QString text;
    const QFileInfo wavInfo(WAV_PATH);
    const QString txtPath = QDir::tempPath() + "/" + wavInfo.completeBaseName() + ".txt";
    QFile f(txtPath);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        text = QString::fromUtf8(f.readAll()).trimmed();
        f.close(); QFile::remove(txtPath);
    }
    if (text.isEmpty()) text = stdoutData;

    // Se saiu vazio, tenta extrair do stderr (alguns engines escrevem lá)
    if (text.isEmpty()) {
        QStringList lines;
        for (const QString& l : m_transcStderr.split('\n')) {
            const QString t = l.trimmed();
            if (!t.isEmpty() && !t.startsWith('[') && !t.startsWith("100%")
                && !t.startsWith("Downloading") && !t.startsWith("whisper")
                && !t.startsWith("Detecting") && !t.startsWith("Transcribing"))
                lines << t;
        }
        text = lines.join('\n').trimmed();
    }

    if (text.isEmpty()) {
        setStatus(tr("⚠ Transcrição vazia — tente modelo maior ou verifique o áudio."),
                  "#c05621");
        return;
    }

    m_transcription->setPlainText(text);
    m_transcription->moveCursor(QTextCursor::Start);
    setStatus(tr("✅ Transcrição concluída (%1 caracteres). Edite e clique em 📝 Inserir na Nota.")
                  .arg(text.length()), "#276749");
    m_btnInsert->setEnabled(m_entry != nullptr);
}

void AudioCaptureDialog::onInsertInNote()
{
    if (!m_entry) return;
    const QString text = m_transcription->toPlainText().trimmed();
    if (text.isEmpty()) return;

    const QString prefix = QString("\n\n---\n🎤 *%1*\n\n%2")
        .arg(QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm"))
        .arg(text);

    const QString uuid   = m_entry->uuidToHex();
    NoteManager&  nm     = NoteManager::instance();
    const QString noteId = nm.getNoteIdForEntry(uuid);
    if (!noteId.isEmpty()) {
        nm.saveNoteContent(noteId, nm.getNoteContent(noteId) + prefix);
    }
    m_entry->setNotes(m_entry->notes() + prefix);
    if (m_db) { QString err; m_db->save(Database::Atomic, {}, &err); }

    setStatus(tr("✅ Transcrição inserida na nota da entrada."), "#276749");
    m_btnInsert->setEnabled(false);
}

void AudioCaptureDialog::onSpellCheck()
{
    if (!m_spell->isAvailable()) {
        setStatus(tr("aspell não disponível. Instale: sudo apt install aspell aspell-pt"),
                  "#c05621");
        return;
    }
    // Força re-highlight
    m_spell->rehighlight();
    setStatus(tr("✓ Verificação ortográfica aplicada — palavras em vermelho têm sugestões (clique direito)."));
}

void AudioCaptureDialog::showSuggestions(const QPoint& pos)
{
    QTextCursor cursor = m_transcription->cursorForPosition(pos);
    cursor.select(QTextCursor::WordUnderCursor);
    const QString word = cursor.selectedText().trimmed();

    QMenu menu(m_transcription);

    if (!word.isEmpty() && m_spell->isAvailable()) {
        const QStringList suggs = m_spell->suggestionsFor(word);
        if (!suggs.isEmpty()) {
            menu.addSection(tr("Sugestões para \"%1\"").arg(word));
            for (const QString& s : suggs) {
                QAction* act = menu.addAction(s);
                connect(act, &QAction::triggered, this, [this, cursor, s]() mutable {
                    cursor.insertText(s);
                });
            }
            menu.addSeparator();
        }
    }

    // Menu padrão de edição
    menu.addAction(tr("Recortar"),  m_transcription, &QTextEdit::cut);
    menu.addAction(tr("Copiar"),    m_transcription, &QTextEdit::copy);
    menu.addAction(tr("Colar"),     m_transcription, &QTextEdit::paste);
    menu.addAction(tr("Selecionar tudo"), m_transcription, &QTextEdit::selectAll);
    menu.exec(m_transcription->viewport()->mapToGlobal(pos));
}

void AudioCaptureDialog::onAiAction(const QString& action)
{
    const QString text = m_transcription->toPlainText().trimmed();

    // Stub CFDM Triplex — integração futura
    QMessageBox dlg(this);
    dlg.setWindowTitle(tr("🤖 CFDM Triplex IA"));
    dlg.setIcon(QMessageBox::Information);

    if (text.isEmpty()) {
        dlg.setText(tr("<b>%1</b><br><br>"
                       "Transcreva um áudio primeiro para usar como base do conteúdo.").arg(action));
    } else {
        dlg.setText(tr("<b>%1</b><br><br>"
                       "Texto base (%2 caracteres) pronto para enviar ao CFDM Triplex.<br><br>"
                       "<i>Integração com os agentes IA em desenvolvimento (Feature 012).</i><br>"
                       "O texto será vinculado ao projeto ao ativar o CFDM Triplex.")
                       .arg(action).arg(text.length()));
    }
    dlg.exec();
}
