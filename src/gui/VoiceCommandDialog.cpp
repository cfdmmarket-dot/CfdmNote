#include "VoiceCommandDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QUuid>
#include <QRegularExpression>

#include "core/Entry.h"
#include "core/Group.h"
#include "core/Database.h"
#include "core/EntryAttributes.h"
#include "notes/NoteManager.h"

static const QString WAV_CMD = QDir::tempPath() + "/cfdmnote_voice_cmd.wav";

// ─────────────────────────────────────────────────────────────────────────────
// Parser de intenção
// ─────────────────────────────────────────────────────────────────────────────

VoiceCommandDialog::ParseResult VoiceCommandDialog::parseIntent(const QString& rawText)
{
    const QString text = rawText.trimmed().toLower();
    ParseResult r;

    // ── Criar entrada ─────────────────────────────────────────────────────────
    static const QRegularExpression reEntry(
        R"((?:criar|nova?|adicionar)\s+entrada\s+(.+))",
        QRegularExpression::CaseInsensitiveOption);
    auto m = reEntry.match(text);
    if (m.hasMatch()) {
        r.intent  = Intent::CreateEntry;
        r.param   = m.captured(1).trimmed();
        // Capitaliza primeira letra
        if (!r.param.isEmpty())
            r.param[0] = r.param[0].toUpper();
        r.summary = QString("Criar entrada: \"%1\"").arg(r.param);
        return r;
    }

    // ── Criar grupo ───────────────────────────────────────────────────────────
    static const QRegularExpression reGroup(
        R"((?:criar|novo?|adicionar)\s+grupo\s+(.+))",
        QRegularExpression::CaseInsensitiveOption);
    m = reGroup.match(text);
    if (m.hasMatch()) {
        r.intent  = Intent::CreateGroup;
        r.param   = m.captured(1).trimmed();
        if (!r.param.isEmpty())
            r.param[0] = r.param[0].toUpper();
        r.summary = QString("Criar grupo: \"%1\"").arg(r.param);
        return r;
    }

    // ── Editar / adicionar nota ───────────────────────────────────────────────
    static const QRegularExpression reNote(
        R"((?:adicionar?|editar?|escrever?|inserir?|incluir?)\s+(?:na\s+)?nota\s+(.+))",
        QRegularExpression::CaseInsensitiveOption);
    m = reNote.match(text);
    if (m.hasMatch()) {
        r.intent  = Intent::EditNote;
        r.param   = m.captured(1).trimmed();
        if (!r.param.isEmpty())
            r.param[0] = r.param[0].toUpper();
        r.summary = QString("Adicionar à nota: \"%1\"").arg(r.param);
        return r;
    }

    // Fallback: tudo que começa com "nota " vira adição de nota
    static const QRegularExpression reNoteFallback(
        R"(^nota\s+(.+))",
        QRegularExpression::CaseInsensitiveOption);
    m = reNoteFallback.match(text);
    if (m.hasMatch()) {
        r.intent  = Intent::EditNote;
        r.param   = m.captured(1).trimmed();
        if (!r.param.isEmpty())
            r.param[0] = r.param[0].toUpper();
        r.summary = QString("Adicionar à nota: \"%1\"").arg(r.param);
        return r;
    }

    r.summary = "Comando não reconhecido.";
    return r;
}

// ─────────────────────────────────────────────────────────────────────────────
// Construtor
// ─────────────────────────────────────────────────────────────────────────────

VoiceCommandDialog::VoiceCommandDialog(QSharedPointer<Database> db,
                                        Group*  currentGroup,
                                        Entry*  currentEntry,
                                        QWidget* parent)
    : QDialog(parent)
    , m_db(db)
    , m_currentGroup(currentGroup)
    , m_currentEntry(currentEntry)
{
    setWindowTitle(tr("🎙 Assistente de Voz — CfdmNote"));
    setMinimumWidth(520);

    auto* lay = new QVBoxLayout(this);
    lay->setSpacing(10);
    lay->setContentsMargins(14, 14, 14, 14);

    // ── Cabeçalho ─────────────────────────────────────────────────────────────
    auto* hdr = new QLabel(tr(
        "<b>🎙 Assistente de Voz</b><br>"
        "<small style='color:#4a5568'>Diga um comando e pressione Transcrever</small>"));
    hdr->setStyleSheet("font-size:12px; color:#2c5282;");
    lay->addWidget(hdr);

    // Contexto atual
    QString ctx;
    if (currentGroup) ctx += tr("Grupo atual: <b>%1</b>").arg(currentGroup->name().toHtmlEscaped());
    if (currentEntry) {
        if (!ctx.isEmpty()) ctx += "  ·  ";
        ctx += tr("Entrada: <b>%1</b>").arg(currentEntry->title().toHtmlEscaped());
    }
    if (!ctx.isEmpty()) {
        auto* ctxLbl = new QLabel(ctx);
        ctxLbl->setStyleSheet("font-size:11px; color:#718096; padding:2px 0;");
        lay->addWidget(ctxLbl);
    }

    // ── Exemplos de comandos ──────────────────────────────────────────────────
    auto* exBox = new QGroupBox(tr("Exemplos de comandos"));
    auto* exLay = new QVBoxLayout(exBox);
    exLay->setSpacing(2);
    for (const QString& ex : {
            tr("\"Criar entrada GitHub\""),
            tr("\"Nova entrada Banco Nubank\""),
            tr("\"Criar grupo Finanças\""),
            tr("\"Novo grupo Projetos\""),
            tr("\"Adicionar nota Lembrar de renovar certificado\""),
            tr("\"Nota Reunião amanhã às 14h\"")}) {
        auto* l = new QLabel("  · " + ex);
        l->setStyleSheet("font-size:11px; color:#4a5568; font-style:italic;");
        exLay->addWidget(l);
    }
    lay->addWidget(exBox);

    // ── Gravação ──────────────────────────────────────────────────────────────
    auto* grpRec = new QGroupBox(tr("Gravação"));
    auto* recLay = new QVBoxLayout(grpRec);

    m_timerLabel = new QLabel("00:00");
    m_timerLabel->setAlignment(Qt::AlignCenter);
    m_timerLabel->setStyleSheet(
        "font-size:28px; font-family:monospace; font-weight:bold; color:#1a365d; padding:4px;");
    recLay->addWidget(m_timerLabel);

    auto* btnRow = new QHBoxLayout();
    m_btnRecord = new QPushButton(tr("● Gravar"));
    m_btnRecord->setFixedHeight(34);
    m_btnRecord->setStyleSheet(
        "QPushButton{background:#e53e3e;color:white;font-weight:bold;border-radius:6px;padding:0 18px;}"
        "QPushButton:hover{background:#c53030;}"
        "QPushButton:disabled{background:#fed7d7;color:#fc8181;}");

    m_btnStop = new QPushButton(tr("■ Parar"));
    m_btnStop->setFixedHeight(34);
    m_btnStop->setEnabled(false);
    m_btnStop->setStyleSheet(
        "QPushButton{background:#718096;color:white;font-weight:bold;border-radius:6px;padding:0 18px;}"
        "QPushButton:hover{background:#4a5568;}"
        "QPushButton:disabled{background:#e2e8f0;color:#a0aec0;}");

    btnRow->addWidget(m_btnRecord);
    btnRow->addWidget(m_btnStop);

    auto* optsRow = new QHBoxLayout();
    optsRow->addWidget(new QLabel(tr("Idioma:")));
    m_langCombo = new QComboBox();
    m_langCombo->addItems({"Portuguese", "English", "Spanish", "auto"});
    optsRow->addWidget(m_langCombo);

    m_btnTranscribe = new QPushButton(tr("  Transcrever"));
    m_btnTranscribe->setIcon(QIcon::fromTheme("system-run"));
    m_btnTranscribe->setFixedHeight(30);
    m_btnTranscribe->setEnabled(false);
    optsRow->addStretch();
    optsRow->addWidget(m_btnTranscribe);

    recLay->addLayout(btnRow);
    recLay->addLayout(optsRow);
    lay->addWidget(grpRec);

    // ── Transcrição ───────────────────────────────────────────────────────────
    m_transcription = new QTextEdit();
    m_transcription->setPlaceholderText(tr("A transcrição do seu comando aparecerá aqui.\n"
                                           "Você pode editá-la antes de confirmar."));
    m_transcription->setFixedHeight(72);
    lay->addWidget(m_transcription);

    // ── Painel de confirmação (oculto) ────────────────────────────────────────
    m_confirmFrame = new QFrame();
    m_confirmFrame->setFrameShape(QFrame::StyledPanel);
    m_confirmFrame->setStyleSheet(
        "QFrame{background:#ebf8ff; border:2px solid #3182ce; border-radius:6px; padding:4px;}");
    m_confirmFrame->setVisible(false);
    auto* cfmLay = new QVBoxLayout(m_confirmFrame);
    cfmLay->setSpacing(6);

    auto* cfmTitle = new QLabel(tr("✅ Comando detectado — confirmar ação?"));
    cfmTitle->setStyleSheet("font-weight:bold; color:#2b6cb0; font-size:12px;");
    cfmLay->addWidget(cfmTitle);

    m_confirmLabel = new QLabel();
    m_confirmLabel->setWordWrap(true);
    m_confirmLabel->setStyleSheet("color:#1a365d; font-size:12px; padding:2px 0;");
    cfmLay->addWidget(m_confirmLabel);

    auto* cfmBtnRow = new QHBoxLayout();
    m_btnExecute = new QPushButton(tr("✔ Executar"));
    m_btnExecute->setStyleSheet(
        "QPushButton{background:#276749;color:white;font-weight:bold;border-radius:5px;padding:5px 18px;}"
        "QPushButton:hover{background:#1c4532;}");
    m_btnRetry = new QPushButton(tr("✎ Corrigir"));
    m_btnRetry->setStyleSheet(
        "QPushButton{background:#c05621;color:white;border-radius:5px;padding:5px 14px;}"
        "QPushButton:hover{background:#7b341e;}");
    cfmBtnRow->addWidget(m_btnExecute);
    cfmBtnRow->addWidget(m_btnRetry);
    cfmBtnRow->addStretch();
    cfmLay->addLayout(cfmBtnRow);
    lay->addWidget(m_confirmFrame);

    // ── Status ────────────────────────────────────────────────────────────────
    m_statusLabel = new QLabel(tr("Clique em Gravar e diga o comando."));
    m_statusLabel->setStyleSheet("font-size:11px; color:#4a5568;");
    m_statusLabel->setWordWrap(true);
    lay->addWidget(m_statusLabel);

    // ── Fechar ────────────────────────────────────────────────────────────────
    auto* btnClose = new QPushButton(tr("Fechar"));
    btnClose->setFixedHeight(28);
    auto* closeRow = new QHBoxLayout();
    closeRow->addStretch();
    closeRow->addWidget(btnClose);
    lay->addLayout(closeRow);

    // ── Timer ─────────────────────────────────────────────────────────────────
    m_tickTimer = new QTimer(this);
    m_tickTimer->setInterval(1000);

    // ── Conexões ─────────────────────────────────────────────────────────────
    connect(m_btnRecord,     &QPushButton::clicked, this, &VoiceCommandDialog::onRecord);
    connect(m_btnStop,       &QPushButton::clicked, this, &VoiceCommandDialog::onStop);
    connect(m_btnTranscribe, &QPushButton::clicked, this, &VoiceCommandDialog::onTranscribe);
    connect(m_btnExecute,    &QPushButton::clicked, this, &VoiceCommandDialog::onExecute);
    connect(m_btnRetry,      &QPushButton::clicked, this, [this]() { resetToRecord(); });
    connect(btnClose,        &QPushButton::clicked, this, &QDialog::accept);
    connect(m_tickTimer,     &QTimer::timeout,      this, &VoiceCommandDialog::onTick);

    // Re-analisa ao editar manualmente
    connect(m_transcription, &QTextEdit::textChanged, this, [this]() {
        const QString t = m_transcription->toPlainText().trimmed();
        if (t.isEmpty()) { m_confirmFrame->setVisible(false); return; }
        const ParseResult r = parseIntent(t);
        if (r.intent != Intent::Unknown) showConfirmStep(r);
        else m_confirmFrame->setVisible(false);
    });

    // Aviso se Whisper não disponível
    if (whisperEngine().isEmpty())
        setStatus(tr("⚠ Whisper não encontrado — instale: pip install openai-whisper"), "#c05621");
}

VoiceCommandDialog::~VoiceCommandDialog()
{
    if (m_recProcess && m_recProcess->state() != QProcess::NotRunning) {
        m_recProcess->kill();
        m_recProcess->waitForFinished(1000);
    }
    if (m_transcProc && m_transcProc->state() != QProcess::NotRunning) {
        m_transcProc->kill();
        m_transcProc->waitForFinished(1000);
    }
}

void VoiceCommandDialog::run(QSharedPointer<Database> db,
                              Group*  currentGroup,
                              Entry*  currentEntry,
                              QWidget* parent)
{
    VoiceCommandDialog dlg(db, currentGroup, currentEntry, parent);
    dlg.exec();
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

QString VoiceCommandDialog::whisperEngine() const
{
    QProcess p;
    p.start("whisper-cli", {"--help"});
    if (p.waitForFinished(1500) && p.exitCode() == 0) return "whisper-cli";
    QProcess p2;
    p2.start("python3", {"-m", "whisper", "--help"});
    if (p2.waitForFinished(3000) && p2.exitCode() == 0) return "python3";
    return {};
}

void VoiceCommandDialog::setStatus(const QString& msg, const QString& color)
{
    m_statusLabel->setText(msg);
    m_statusLabel->setStyleSheet(QString("font-size:11px; color:%1;").arg(color));
}

void VoiceCommandDialog::showConfirmStep(const ParseResult& result)
{
    m_pending = result;

    QString detail;
    switch (result.intent) {
    case Intent::CreateEntry:
        detail = tr("→ Criar entrada <b>%1</b> no grupo <b>%2</b>")
                     .arg(result.param.toHtmlEscaped(),
                          m_currentGroup ? m_currentGroup->name().toHtmlEscaped() : "raiz");
        break;
    case Intent::CreateGroup:
        detail = tr("→ Criar grupo <b>%1</b> dentro de <b>%2</b>")
                     .arg(result.param.toHtmlEscaped(),
                          m_currentGroup ? m_currentGroup->name().toHtmlEscaped() : "raiz");
        break;
    case Intent::EditNote:
        detail = tr("→ Adicionar à nota de <b>%1</b>:<br><i>%2</i>")
                     .arg(m_currentEntry ? m_currentEntry->title().toHtmlEscaped()
                                         : tr("(nenhuma entrada selecionada)"),
                          result.param.toHtmlEscaped());
        break;
    default:
        break;
    }
    m_confirmLabel->setText(detail);
    m_confirmFrame->setVisible(true);
}

void VoiceCommandDialog::resetToRecord()
{
    m_confirmFrame->setVisible(false);
    m_transcription->clear();
    m_btnRecord->setEnabled(true);
    m_btnTranscribe->setEnabled(false);
    setStatus(tr("Clique em Gravar para tentar novamente."));
}

// ─────────────────────────────────────────────────────────────────────────────
// Slots
// ─────────────────────────────────────────────────────────────────────────────

void VoiceCommandDialog::onRecord()
{
    QFile::remove(WAV_CMD);
    m_recProcess = new QProcess(this);
    m_recProcess->start("arecord",
        {"-f", "S16_LE", "-r", "16000", "-c", "1", "-t", "wav", WAV_CMD});
    if (!m_recProcess->waitForStarted(2000)) {
        setStatus(tr("Erro: arecord não encontrado (sudo apt install alsa-utils)"), "#c05621");
        delete m_recProcess; m_recProcess = nullptr;
        return;
    }
    connect(m_recProcess, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code) { onRecordFinished(code); });
    m_elapsed = 0; m_recording = true;
    m_tickTimer->start();
    m_btnRecord->setEnabled(false);
    m_btnStop->setEnabled(true);
    m_btnTranscribe->setEnabled(false);
    m_confirmFrame->setVisible(false);
    m_transcription->clear();
    m_timerLabel->setStyleSheet(
        "font-size:28px; font-family:monospace; font-weight:bold; color:#e53e3e; padding:4px;");
    setStatus(tr("⏺ Gravando… diga o comando. Clique em Parar quando terminar."), "#c53030");
}

void VoiceCommandDialog::onStop()
{
    if (m_recProcess && m_recProcess->state() != QProcess::NotRunning) {
        m_recProcess->terminate();
        m_recProcess->waitForFinished(2000);
    }
    m_tickTimer->stop();
    m_recording = false;
    m_timerLabel->setStyleSheet(
        "font-size:28px; font-family:monospace; font-weight:bold; color:#1a365d; padding:4px;");
    m_btnStop->setEnabled(false);
}

void VoiceCommandDialog::onRecordFinished(int)
{
    m_recording = false;
    m_tickTimer->stop();
    m_timerLabel->setStyleSheet(
        "font-size:28px; font-family:monospace; font-weight:bold; color:#1a365d; padding:4px;");
    m_hasAudio = QFileInfo(WAV_CMD).size() > 512;
    m_btnRecord->setEnabled(true);
    m_btnStop->setEnabled(false);
    m_btnTranscribe->setEnabled(m_hasAudio);
    if (m_hasAudio)
        setStatus(tr("✅ Áudio capturado (%1s). Clique em Transcrever.").arg(m_elapsed));
    else
        setStatus(tr("Nenhum áudio detectado. Tente novamente."), "#c05621");
}

void VoiceCommandDialog::onTick()
{
    ++m_elapsed;
    m_timerLabel->setText(QString("%1:%2")
        .arg(m_elapsed/60, 2, 10, QChar('0'))
        .arg(m_elapsed%60, 2, 10, QChar('0')));
    if (m_elapsed >= 60) {   // limite de 1 min para comandos de voz
        setStatus(tr("⏱ Limite de 1 min atingido."));
        onStop();
    }
}

void VoiceCommandDialog::onTranscribe()
{
    if (!m_hasAudio) return;
    const QString engine = whisperEngine();
    if (engine.isEmpty()) {
        setStatus(tr("Whisper não instalado. Execute: pip install openai-whisper"), "#c05621");
        return;
    }
    m_btnTranscribe->setEnabled(false);
    setStatus(tr("⏳ Transcrevendo…"));

    m_transcProc = new QProcess(this);
    m_transcProc->setWorkingDirectory(QDir::tempPath());
    QStringList args;
    const QString lang = m_langCombo->currentText();
    if (engine == "whisper-cli") {
        args << "--model" << "base"
             << "--language" << (lang == "auto" ? "auto" : lang.left(2).toLower())
             << "--output-txt" << "--file" << WAV_CMD;
        m_transcProc->start("whisper-cli", args);
    } else {
        args << "-m" << "whisper" << WAV_CMD
             << "--model" << "base"
             << "--language" << (lang == "auto" ? "auto" : lang)
             << "--output_format" << "txt"
             << "--output_dir" << QDir::tempPath();
        m_transcProc->start("python3", args);
    }
    connect(m_transcProc, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code) { onTranscribeFinished(code); });
}

void VoiceCommandDialog::onTranscribeFinished(int exitCode)
{
    m_btnTranscribe->setEnabled(true);
    if (exitCode != 0) {
        setStatus(tr("Erro na transcrição."), "#c05621");
        return;
    }
    const QString txtPath = WAV_CMD.left(WAV_CMD.length() - 4) + ".txt";
    QString text;
    QFile f(txtPath);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        text = QString::fromUtf8(f.readAll()).trimmed();
        f.close();
    }
    if (text.isEmpty())
        text = QString::fromUtf8(m_transcProc->readAllStandardOutput()).trimmed();
    if (text.isEmpty()) {
        setStatus(tr("Transcrição vazia — fale mais alto ou use modelo maior."), "#c05621");
        return;
    }
    m_transcription->setPlainText(text);
    setStatus(tr("✅ Transcrito. Verifique o comando detectado abaixo."), "#276749");

    const ParseResult r = parseIntent(text);
    if (r.intent != Intent::Unknown)
        showConfirmStep(r);
    else
        setStatus(tr("⚠ Comando não reconhecido: \"%1\"").arg(text), "#c05621");
}

void VoiceCommandDialog::onExecute()
{
    if (!m_db) return;

    switch (m_pending.intent) {

    case Intent::CreateEntry: {
        if (!m_currentGroup) {
            setStatus(tr("Nenhum grupo selecionado — selecione um grupo primeiro."), "#c05621");
            return;
        }
        auto* entry = new Entry();
        entry->setUuid(QUuid::createUuid());
        entry->setTitle(m_pending.param);
        entry->setGroup(m_currentGroup);
        QString err;
        m_db->save(Database::Atomic, {}, &err);
        setStatus(tr("✅ Entrada \"%1\" criada em \"%2\".")
                      .arg(m_pending.param, m_currentGroup->name()), "#276749");
        break;
    }

    case Intent::CreateGroup: {
        if (!m_currentGroup) {
            setStatus(tr("Nenhum grupo selecionado — selecione um grupo primeiro."), "#c05621");
            return;
        }
        auto* grp = new Group();
        grp->setUuid(QUuid::createUuid());
        grp->setName(m_pending.param);
        grp->setParent(m_currentGroup);
        QString err;
        m_db->save(Database::Atomic, {}, &err);
        setStatus(tr("✅ Grupo \"%1\" criado em \"%2\".")
                      .arg(m_pending.param, m_currentGroup->name()), "#276749");
        break;
    }

    case Intent::EditNote: {
        if (!m_currentEntry) {
            setStatus(tr("Nenhuma entrada selecionada — selecione uma entrada primeiro."), "#c05621");
            return;
        }
        const QString prefix = QString("\n\n---\n🎙 %1\n%2")
            .arg(QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm"))
            .arg(m_pending.param);
        // NoteManager (banco .ksdb)
        const QString uuid   = m_currentEntry->uuidToHex();
        NoteManager&  nm     = NoteManager::instance();
        const QString noteId = nm.getNoteIdForEntry(uuid);
        if (!noteId.isEmpty())
            nm.saveNoteContent(noteId, nm.getNoteContent(noteId) + prefix);
        // Entry nativa
        m_currentEntry->setNotes(m_currentEntry->notes() + prefix);
        QString err;
        m_db->save(Database::Atomic, {}, &err);
        setStatus(tr("✅ Nota adicionada em \"%1\".").arg(m_currentEntry->title()), "#276749");
        break;
    }

    default:
        setStatus(tr("Ação desconhecida — tente novamente."), "#c05621");
        return;
    }

    m_confirmFrame->setVisible(false);
    m_transcription->clear();
    m_hasAudio = false;
    m_btnTranscribe->setEnabled(false);
}
