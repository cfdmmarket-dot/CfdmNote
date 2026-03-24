#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QComboBox>
#include <QTimer>
#include <QProcess>
#include <QSharedPointer>
#include <QFrame>

class Database;
class Group;
class Entry;

/**
 * VoiceCommandDialog — Assistente de voz para o CfdmNote.
 *
 * Grava um comando de voz, transcreve com Whisper e interpreta a intenção:
 *   - "criar entrada <nome>"    → cria entrada no grupo atual
 *   - "novo grupo <nome>"       → cria grupo filho do grupo atual
 *   - "adicionar nota <texto>"  → acrescenta texto na nota da entrada atual
 *
 * Sempre mostra confirmação antes de executar.
 *
 * Uso:
 *   VoiceCommandDialog::run(db, currentGroup, currentEntry, parent);
 */
class VoiceCommandDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VoiceCommandDialog(QSharedPointer<Database> db,
                                 Group*  currentGroup,
                                 Entry*  currentEntry,
                                 QWidget* parent = nullptr);
    ~VoiceCommandDialog() override;

    static void run(QSharedPointer<Database> db,
                    Group*  currentGroup,
                    Entry*  currentEntry,
                    QWidget* parent = nullptr);

    // Intenções detectadas
    enum class Intent { Unknown, CreateEntry, CreateGroup, EditNote };

    struct ParseResult {
        Intent  intent  = Intent::Unknown;
        QString param;      // nome da entrada/grupo ou texto da nota
        QString summary;    // descrição legível do que será feito
    };

    // Parser público para testes
    static ParseResult parseIntent(const QString& text);

private slots:
    void onRecord();
    void onStop();
    void onTranscribe();
    void onExecute();
    void onTick();
    void onRecordFinished(int exitCode);
    void onTranscribeFinished(int exitCode);

private:
    void setStatus(const QString& msg, const QString& color = "#2c5282");
    void showConfirmStep(const ParseResult& result);
    void resetToRecord();
    QString whisperEngine() const;
    void executeAction();

    QSharedPointer<Database> m_db;
    Group*  m_currentGroup  = nullptr;
    Entry*  m_currentEntry  = nullptr;

    ParseResult m_pending;

    // ── UI ────────────────────────────────────────────────────────────────────
    QLabel*      m_timerLabel    = nullptr;
    QLabel*      m_statusLabel   = nullptr;
    QPushButton* m_btnRecord     = nullptr;
    QPushButton* m_btnStop       = nullptr;
    QPushButton* m_btnTranscribe = nullptr;
    QTextEdit*   m_transcription = nullptr;
    QComboBox*   m_langCombo     = nullptr;

    // Painel de confirmação (oculto até intent detectada)
    QFrame*      m_confirmFrame  = nullptr;
    QLabel*      m_confirmLabel  = nullptr;
    QPushButton* m_btnExecute    = nullptr;
    QPushButton* m_btnRetry      = nullptr;

    // ── Estado ────────────────────────────────────────────────────────────────
    QTimer*   m_tickTimer  = nullptr;
    QProcess* m_recProcess = nullptr;
    QProcess* m_transcProc = nullptr;
    int       m_elapsed    = 0;
    bool      m_hasAudio   = false;
    bool      m_recording  = false;
};
