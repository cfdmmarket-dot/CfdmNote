#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QComboBox>
#include <QTimer>
#include <QProcess>
#include <QSharedPointer>

class Entry;
class Database;

/**
 * AudioCaptureDialog — Captura de áudio + transcrição para a nota da entrada.
 *
 * Gravação: arecord (ALSA, 16 kHz mono WAV — formato ideal para whisper).
 * Transcrição: tenta em ordem:
 *   1. whisper-cli (whisper.cpp instalado)
 *   2. python3 -m whisper (OpenAI Whisper via pip)
 *   3. Exibe guia de instalação
 *
 * Uso:
 *   AudioCaptureDialog::captureAudio(entry, db, parentWidget);
 */
class AudioCaptureDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AudioCaptureDialog(Entry* entry,
                                 QSharedPointer<Database> db,
                                 QWidget* parent = nullptr);
    ~AudioCaptureDialog();

    static void captureAudio(Entry* entry,
                              QSharedPointer<Database> db,
                              QWidget* parent = nullptr);

private slots:
    void onRecord();
    void onStop();
    void onPlay();
    void onTranscribe();
    void onInsertInNote();
    void onTick();
    void onRecordFinished(int exitCode);
    void onTranscribeFinished(int exitCode);

private:
    void updateButtons();
    void setStatus(const QString& msg, const QString& color = "#2c5282");
    QString whisperEngine() const;
    QString tmpWavPath() const;

    Entry*                   m_entry    = nullptr;
    QSharedPointer<Database> m_db;

    // UI
    QLabel*      m_timerLabel   = nullptr;
    QLabel*      m_statusLabel  = nullptr;
    QPushButton* m_btnRecord    = nullptr;
    QPushButton* m_btnStop      = nullptr;
    QPushButton* m_btnPlay      = nullptr;
    QPushButton* m_btnTranscribe= nullptr;
    QPushButton* m_btnInsert    = nullptr;
    QTextEdit*   m_transcription= nullptr;
    QComboBox*   m_modelCombo   = nullptr;
    QComboBox*   m_langCombo    = nullptr;

    // Estado
    QTimer*   m_tickTimer  = nullptr;
    QProcess* m_recProcess = nullptr;
    QProcess* m_transcProc = nullptr;
    QProcess* m_playProc   = nullptr;
    int       m_elapsed    = 0;
    bool      m_hasAudio   = false;
    bool      m_recording  = false;
};
