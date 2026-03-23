#include "AudioCaptureDialog.h"

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

#include "core/Entry.h"
#include "core/Database.h"
#include "notes/NoteManager.h"
#include "gui/ToastNotification.h"

static const QString WAV_PATH =
    QDir::tempPath() + "/cfdmnote_audio_capture.wav";

// ─────────────────────────────────────────────────────────────────────────────

AudioCaptureDialog::AudioCaptureDialog(Entry* entry,
                                        QSharedPointer<Database> db,
                                        QWidget* parent)
    : QDialog(parent)
    , m_entry(entry)
    , m_db(db)
{
    setWindowTitle(tr("Capturar Áudio / Transcrição"));
    setMinimumWidth(560);

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    layout->setContentsMargins(12, 12, 12, 12);

    // ── Cabeçalho ─────────────────────────────────────────────────────────────
    auto* header = new QLabel(
        entry
            ? tr("<b>🎤 Gravação de Áudio</b>  —  entrada: <i>%1</i>")
                 .arg(entry->title().toHtmlEscaped())
            : tr("<b>🎤 Gravação de Áudio</b>"));
    header->setStyleSheet("font-size: 12px; color: #2c5282; padding: 2px 0;");
    layout->addWidget(header);

    // ── Controles de gravação ─────────────────────────────────────────────────
    auto* grpRec = new QGroupBox(tr("Gravação"));
    auto* recLayout = new QVBoxLayout(grpRec);

    m_timerLabel = new QLabel("00:00");
    m_timerLabel->setAlignment(Qt::AlignCenter);
    m_timerLabel->setStyleSheet(
        "font-size: 32px; font-family: monospace; font-weight: bold;"
        "color: #1a365d; padding: 8px;");
    recLayout->addWidget(m_timerLabel);

    auto* btnRow = new QHBoxLayout();
    m_btnRecord = new QPushButton(tr("● Gravar"));
    m_btnRecord->setFixedHeight(36);
    m_btnRecord->setStyleSheet(
        "QPushButton { background: #e53e3e; color: white; font-weight: bold;"
        "  border-radius: 6px; padding: 0 20px; font-size: 13px; }"
        "QPushButton:hover { background: #c53030; }"
        "QPushButton:disabled { background: #fed7d7; color: #fc8181; }");

    m_btnStop = new QPushButton(tr("■ Parar"));
    m_btnStop->setFixedHeight(36);
    m_btnStop->setEnabled(false);
    m_btnStop->setStyleSheet(
        "QPushButton { background: #718096; color: white; font-weight: bold;"
        "  border-radius: 6px; padding: 0 20px; font-size: 13px; }"
        "QPushButton:hover { background: #4a5568; }"
        "QPushButton:disabled { background: #e2e8f0; color: #a0aec0; }");

    m_btnPlay = new QPushButton(tr("▶ Reproduzir"));
    m_btnPlay->setFixedHeight(36);
    m_btnPlay->setEnabled(false);
    m_btnPlay->setStyleSheet(
        "QPushButton { background: #38a169; color: white; font-weight: bold;"
        "  border-radius: 6px; padding: 0 20px; font-size: 13px; }"
        "QPushButton:hover { background: #276749; }"
        "QPushButton:disabled { background: #c6f6d5; color: #68d391; }");

    btnRow->addWidget(m_btnRecord);
    btnRow->addWidget(m_btnStop);
    btnRow->addWidget(m_btnPlay);
    recLayout->addLayout(btnRow);
    layout->addWidget(grpRec);

    // ── Opções de transcrição ─────────────────────────────────────────────────
    auto* grpTransc = new QGroupBox(tr("Transcrição"));
    auto* transcLayout = new QVBoxLayout(grpTransc);

    auto* optsRow = new QHBoxLayout();
    optsRow->addWidget(new QLabel(tr("Modelo:")));
    m_modelCombo = new QComboBox();
    m_modelCombo->addItems({"tiny", "base", "small", "medium", "large"});
    m_modelCombo->setToolTip(
        tr("tiny = mais rápido (menos preciso)\n"
           "large = mais lento (mais preciso)\n"
           "Recomendado: base ou small para PT-BR"));
    optsRow->addWidget(m_modelCombo);

    optsRow->addSpacing(12);
    optsRow->addWidget(new QLabel(tr("Idioma:")));
    m_langCombo = new QComboBox();
    m_langCombo->addItems({
        "Portuguese", "English", "Spanish", "French",
        "German", "Italian", "Japanese", "Chinese", "auto"
    });
    optsRow->addWidget(m_langCombo);
    optsRow->addStretch();

    m_btnTranscribe = new QPushButton(QIcon::fromTheme("system-run"),
                                       tr("  Transcrever"));
    m_btnTranscribe->setFixedHeight(30);
    m_btnTranscribe->setEnabled(false);
    optsRow->addWidget(m_btnTranscribe);
    transcLayout->addLayout(optsRow);

    m_transcription = new QTextEdit();
    m_transcription->setPlaceholderText(
        tr("A transcrição aparecerá aqui após gravar e clicar em Transcrever.\n"
           "Você também pode digitar ou editar o texto livremente."));
    m_transcription->setMinimumHeight(120);
    transcLayout->addWidget(m_transcription);
    layout->addWidget(grpTransc);

    // ── Status ────────────────────────────────────────────────────────────────
    m_statusLabel = new QLabel(tr("Pronto. Clique em Gravar para começar."));
    m_statusLabel->setStyleSheet("font-size: 11px; color: #4a5568;");
    m_statusLabel->setWordWrap(true);
    layout->addWidget(m_statusLabel);

    // ── Rodapé ────────────────────────────────────────────────────────────────
    auto* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    auto* footRow = new QHBoxLayout();
    m_btnInsert = new QPushButton(tr("Inserir na Nota"));
    m_btnInsert->setFixedHeight(30);
    m_btnInsert->setEnabled(false);
    m_btnInsert->setStyleSheet(
        "QPushButton { background: #2b6cb0; color: white; font-weight: bold;"
        "  border-radius: 4px; padding: 0 16px; }"
        "QPushButton:hover { background: #1a365d; }"
        "QPushButton:disabled { background: #bee3f8; color: #90cdf4; }");

    auto* btnClose = new QPushButton(tr("Fechar"));
    btnClose->setFixedHeight(30);

    footRow->addWidget(m_btnInsert);
    footRow->addStretch();
    footRow->addWidget(btnClose);
    layout->addLayout(footRow);

    // ── Timer de elapsed ─────────────────────────────────────────────────────
    m_tickTimer = new QTimer(this);
    m_tickTimer->setInterval(1000);

    // ── Conexões ─────────────────────────────────────────────────────────────
    connect(m_btnRecord,    &QPushButton::clicked, this, &AudioCaptureDialog::onRecord);
    connect(m_btnStop,      &QPushButton::clicked, this, &AudioCaptureDialog::onStop);
    connect(m_btnPlay,      &QPushButton::clicked, this, &AudioCaptureDialog::onPlay);
    connect(m_btnTranscribe,&QPushButton::clicked, this, &AudioCaptureDialog::onTranscribe);
    connect(m_btnInsert,    &QPushButton::clicked, this, &AudioCaptureDialog::onInsertInNote);
    connect(btnClose,       &QPushButton::clicked, this, &QDialog::accept);
    connect(m_tickTimer,    &QTimer::timeout,      this, &AudioCaptureDialog::onTick);
    connect(m_transcription,&QTextEdit::textChanged, this, [this]() {
        m_btnInsert->setEnabled(m_entry != nullptr &&
                                !m_transcription->toPlainText().trimmed().isEmpty());
    });

    // Detecta motor de transcrição disponível
    const QString engine = whisperEngine();
    if (engine.isEmpty()) {
        setStatus(tr("⚠ Motor de transcrição não encontrado. Veja o aviso ao lado."),
                  "#c05621");

        // Popup flutuante não-bloqueante com instruções de instalação
        QTimer::singleShot(300, this, [this]() {
            ToastNotification::show(
                parentWidget() ? parentWidget() : this,
                tr("🎤 Transcrição não configurada"),
                tr("A gravação de áudio já funciona!\n\n"
                   "Para ativar a transcrição automática, instale o Whisper:"),
                "pip install openai-whisper",
                20000   // 20 segundos
            );
        });
    } else {
        setStatus(tr("Motor detectado: %1").arg(engine));
    }
}

AudioCaptureDialog::~AudioCaptureDialog()
{
    if (m_recProcess  && m_recProcess->state()  != QProcess::NotRunning) {
        m_recProcess->kill();
        m_recProcess->waitForFinished(1000);
    }
    if (m_transcProc  && m_transcProc->state()  != QProcess::NotRunning) {
        m_transcProc->kill();
        m_transcProc->waitForFinished(1000);
    }
    if (m_playProc    && m_playProc->state()    != QProcess::NotRunning) {
        m_playProc->kill();
        m_playProc->waitForFinished(1000);
    }
}

// ─────────────────────────────────────────────────────────────────────────────

void AudioCaptureDialog::captureAudio(Entry* entry,
                                       QSharedPointer<Database> db,
                                       QWidget* parent)
{
    AudioCaptureDialog dlg(entry, db, parent);
    dlg.exec();
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

QString AudioCaptureDialog::whisperEngine() const
{
    // Tenta whisper-cli (whisper.cpp)
    QProcess p;
    p.start("whisper-cli", {"--help"});
    if (p.waitForFinished(1500) && p.exitCode() == 0)
        return "whisper-cli (whisper.cpp)";

    // Tenta python3 -m whisper
    QProcess p2;
    p2.start("python3", {"-m", "whisper", "--help"});
    if (p2.waitForFinished(3000) && p2.exitCode() == 0)
        return "python3 -m whisper (openai-whisper)";

    return {};
}

QString AudioCaptureDialog::tmpWavPath() const
{
    return WAV_PATH;
}

void AudioCaptureDialog::setStatus(const QString& msg, const QString& color)
{
    m_statusLabel->setText(msg);
    m_statusLabel->setStyleSheet(
        QString("font-size: 11px; color: %1;").arg(color));
}

void AudioCaptureDialog::updateButtons()
{
    m_btnRecord->setEnabled(!m_recording);
    m_btnStop->setEnabled(m_recording);
    m_btnPlay->setEnabled(m_hasAudio && !m_recording);
    m_btnTranscribe->setEnabled(m_hasAudio && !m_recording);
}

// ─────────────────────────────────────────────────────────────────────────────
// Slots
// ─────────────────────────────────────────────────────────────────────────────

void AudioCaptureDialog::onRecord()
{
    // Para reprodução se estiver rodando
    if (m_playProc && m_playProc->state() != QProcess::NotRunning) {
        m_playProc->kill();
        m_playProc->waitForFinished(500);
    }

    // Remove arquivo anterior
    QFile::remove(WAV_PATH);

    m_recProcess = new QProcess(this);
    // arecord: 16kHz, mono, 16-bit signed LE — formato ideal para whisper
    m_recProcess->start("arecord",
        {"-f", "S16_LE", "-r", "16000", "-c", "1", "-t", "wav", WAV_PATH});

    if (!m_recProcess->waitForStarted(2000)) {
        setStatus(tr("Erro: arecord não encontrado. Instale com: sudo apt install alsa-utils"),
                  "#c05621");
        delete m_recProcess;
        m_recProcess = nullptr;
        return;
    }

    connect(m_recProcess, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code) { onRecordFinished(code); });

    m_elapsed   = 0;
    m_recording = true;
    m_tickTimer->start();
    m_timerLabel->setStyleSheet(
        "font-size: 32px; font-family: monospace; font-weight: bold;"
        "color: #e53e3e; padding: 8px;");  // vermelho enquanto grava
    setStatus(tr("⏺ Gravando... fale agora. Clique em Parar quando terminar."), "#c53030");
    updateButtons();
}

void AudioCaptureDialog::onStop()
{
    if (m_recProcess && m_recProcess->state() != QProcess::NotRunning) {
        m_recProcess->terminate();
        m_recProcess->waitForFinished(2000);
    }
    m_tickTimer->stop();
    m_recording = false;
    m_timerLabel->setStyleSheet(
        "font-size: 32px; font-family: monospace; font-weight: bold;"
        "color: #1a365d; padding: 8px;");
    updateButtons();
}

void AudioCaptureDialog::onRecordFinished(int /*exitCode*/)
{
    m_recording = false;
    m_tickTimer->stop();
    m_timerLabel->setStyleSheet(
        "font-size: 32px; font-family: monospace; font-weight: bold;"
        "color: #1a365d; padding: 8px;");

    m_hasAudio = QFileInfo(WAV_PATH).size() > 512;
    if (m_hasAudio) {
        setStatus(tr("✅ Gravação salva (%1s). Clique em Transcrever ou edite o texto.")
                      .arg(m_elapsed));
    } else {
        setStatus(tr("Nenhum áudio capturado."), "#c05621");
    }
    updateButtons();
}

void AudioCaptureDialog::onPlay()
{
    if (m_playProc && m_playProc->state() != QProcess::NotRunning) {
        m_playProc->kill();
        return;
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
                               .arg(m, 2, 10, QChar('0'))
                               .arg(s, 2, 10, QChar('0')));
    // Limite de 5 minutos para não gerar arquivo enorme
    if (m_elapsed >= 300) {
        setStatus(tr("⏱ Limite de 5 min atingido — gravação parada automaticamente."));
        onStop();
    }
}

void AudioCaptureDialog::onTranscribe()
{
    if (!m_hasAudio) return;

    const QString engine = whisperEngine();
    if (engine.isEmpty()) {
        setStatus(tr("Motor não encontrado."), "#c05621");
        ToastNotification::show(
            parentWidget() ? parentWidget() : this,
            tr("🎤 Instale o Whisper para transcrever"),
            tr("Execute o comando abaixo no terminal e reinicie o CfdmNote:"),
            "pip install openai-whisper",
            20000
        );
        return;
    }

    const QString model = m_modelCombo->currentText();
    const QString lang  = m_langCombo->currentText();

    m_btnTranscribe->setEnabled(false);
    setStatus(tr("⏳ Transcrevendo com %1 (modelo: %2)...").arg(engine, model));

    m_transcProc = new QProcess(this);
    m_transcProc->setWorkingDirectory(QDir::tempPath());

    QStringList args;
    if (engine.startsWith("whisper-cli")) {
        // whisper.cpp CLI
        args << "--model"    << QString("ggml-%1.bin").arg(model)
             << "--language" << (lang == "auto" ? QString("auto") : lang.left(2).toLower())
             << "--output-txt"
             << "--file"     << WAV_PATH;
        m_transcProc->start("whisper-cli", args);
    } else {
        // python3 -m whisper
        args << "-m"             << "whisper"
             << WAV_PATH
             << "--model"        << model
             << "--language"     << (lang == "auto" ? QString("auto") : lang)
             << "--output_format" << "txt"
             << "--output_dir"   << QDir::tempPath();
        m_transcProc->start("python3", args);
    }

    connect(m_transcProc,
            QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code) { onTranscribeFinished(code); });
}

void AudioCaptureDialog::onTranscribeFinished(int exitCode)
{
    m_btnTranscribe->setEnabled(true);

    if (exitCode != 0) {
        const QString err = QString::fromUtf8(m_transcProc->readAllStandardError()).trimmed();
        setStatus(tr("Erro na transcrição (código %1): %2").arg(exitCode).arg(err), "#c05621");
        return;
    }

    // Tenta ler o .txt gerado pelo whisper
    const QString txtPath = WAV_PATH.left(WAV_PATH.length() - 4) + ".txt";
    QString text;

    QFile f(txtPath);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        text = QString::fromUtf8(f.readAll()).trimmed();
        f.close();
    }

    // Fallback: lê a saída padrão do processo
    if (text.isEmpty())
        text = QString::fromUtf8(m_transcProc->readAllStandardOutput()).trimmed();

    if (text.isEmpty()) {
        setStatus(tr("Transcrição vazia — tente um modelo maior ou verifique o áudio."),
                  "#c05621");
        return;
    }

    m_transcription->setPlainText(text);
    setStatus(tr("✅ Transcrição concluída (%1 caracteres).").arg(text.length()));
    m_btnInsert->setEnabled(m_entry != nullptr);
}

void AudioCaptureDialog::onInsertInNote()
{
    if (!m_entry) return;
    const QString text = m_transcription->toPlainText().trimmed();
    if (text.isEmpty()) return;

    // Prefixo com data/hora
    const QString prefix = QString("\n\n---\n🎤 %1\n%2")
                               .arg(QDateTime::currentDateTime()
                                        .toString("dd/MM/yyyy HH:mm"))
                               .arg(text);

    // Tenta inserir via NoteManager (banco .ksdb)
    const QString uuid   = m_entry->uuidToHex();
    NoteManager&  nm     = NoteManager::instance();
    const QString noteId = nm.getNoteIdForEntry(uuid);
    if (!noteId.isEmpty()) {
        const QString current = nm.getNoteContent(noteId);
        nm.saveNoteContent(noteId, current + prefix);
    }

    // Também adiciona nas notes nativas da entrada como fallback
    m_entry->setNotes(m_entry->notes() + prefix);
    if (m_db) {
        QString err;
        m_db->save(Database::Atomic, {}, &err);
    }

    setStatus(tr("✅ Transcrição inserida na nota da entrada."), "#276749");
    m_btnInsert->setEnabled(false);
}
