#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QComboBox>
#include <QTimer>
#include <QProcess>
#include <QSharedPointer>
#include <QWidget>
#include <QVector>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QCheckBox>

class Entry;
class Database;

// ─── WaveformWidget ───────────────────────────────────────────────────────────
/**
 * Exibe barras de nível de áudio em tempo real.
 * Lê o arquivo WAV enquanto cresce (arecord escreve progressivamente).
 * Cores: verde (normal) | amarelo (alto) | vermelho (clipping/silêncio)
 */
class WaveformWidget : public QWidget
{
    Q_OBJECT
public:
    explicit WaveformWidget(QWidget* parent = nullptr);
    void startMonitoring(const QString& wavPath);
    void stopMonitoring();
    void clear();

protected:
    void paintEvent(QPaintEvent*) override;

private slots:
    void tick();

private:
    float readLastRms() const;

    QTimer   m_timer;
    QString  m_wavPath;
    QVector<float> m_levels;   // histórico de níveis (0.0–1.0)
    bool     m_active = false;
    static constexpr int MAX_BARS = 120;
};

// ─── SpellHighlighter ─────────────────────────────────────────────────────────
/**
 * Sublinha palavras desconhecidas usando aspell em modo pipe.
 * Sugestões disponíveis via suggestionsFor(word).
 */
class SpellHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit SpellHighlighter(QTextDocument* parent);
    QStringList suggestionsFor(const QString& word) const;
    bool isAvailable() const { return m_available; }

protected:
    void highlightBlock(const QString& text) override;

private:
    bool checkWord(const QString& word) const;

    mutable QProcess m_proc;
    bool m_available = false;
    QTextCharFormat m_errorFmt;
};

// ─── AudioCaptureDialog ───────────────────────────────────────────────────────
class AudioCaptureDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AudioCaptureDialog(Entry* entry,
                                 QSharedPointer<Database> db,
                                 QWidget* parent = nullptr);
    ~AudioCaptureDialog() override;

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
    void onSpellCheck();
    void onAiAction(const QString& action);

private:
    void updateButtons();
    void setStatus(const QString& msg, const QString& color = "#2c5282");
    void updateFileInfo();
    void showSuggestions(const QPoint& pos);
    QString whisperEngine() const;
    QString tmpWavPath() const;

    Entry*                   m_entry    = nullptr;
    QSharedPointer<Database> m_db;

    // UI
    WaveformWidget* m_waveform      = nullptr;
    QLabel*      m_timerLabel       = nullptr;
    QLabel*      m_fileInfoLabel    = nullptr;
    QLabel*      m_statusLabel      = nullptr;
    QPushButton* m_btnRecord        = nullptr;
    QPushButton* m_btnStop          = nullptr;
    QPushButton* m_btnPlay          = nullptr;
    QPushButton* m_btnTranscribe    = nullptr;
    QPushButton* m_btnInsert        = nullptr;
    QPushButton* m_btnSpell         = nullptr;
    QTextEdit*   m_transcription    = nullptr;
    QComboBox*   m_modelCombo        = nullptr;
    QComboBox*   m_langCombo         = nullptr;
    QCheckBox*   m_chkAutoTranscribe = nullptr;
    SpellHighlighter* m_spell        = nullptr;

    // Estado
    QTimer*   m_tickTimer    = nullptr;
    QProcess* m_recProcess   = nullptr;
    QProcess* m_transcProc   = nullptr;
    QProcess* m_playProc     = nullptr;
    int       m_elapsed      = 0;
    bool      m_hasAudio     = false;
    bool      m_recording    = false;
    QString   m_transcStderr;   // acumula stderr completo da transcrição
};
