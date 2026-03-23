#pragma once

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPropertyAnimation>

/**
 * ToastNotification — popup flutuante não-bloqueante.
 *
 * Aparece no canto inferior direito do widget pai, com barra de progresso
 * regressiva e botão de fechar. Suporta um "comando" copiável (pip install...).
 *
 * Uso:
 *   ToastNotification::show(parent,
 *       "🎤 Whisper não encontrado",
 *       "Para transcrever automaticamente, instale:",
 *       "pip install openai-whisper");
 */
class ToastNotification : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal windowOpacity READ windowOpacity WRITE setWindowOpacity)

public:
    static void show(QWidget*       parent,
                     const QString& title,
                     const QString& message,
                     const QString& command   = {},
                     int            durationMs = 18000);

private:
    explicit ToastNotification(QWidget*       parent,
                                const QString& title,
                                const QString& message,
                                const QString& command,
                                int            durationMs);

    void reposition();

    QLabel*            m_progressBar = nullptr;
    QTimer*            m_tickTimer   = nullptr;
    QPropertyAnimation* m_fadeAnim   = nullptr;
    int                m_totalMs     = 0;
    int                m_elapsedMs   = 0;
};
