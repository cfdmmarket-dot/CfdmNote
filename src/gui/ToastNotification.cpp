#include "ToastNotification.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QApplication>
#include <QClipboard>
#include <QScreen>
#include <QFrame>
#include <QFont>
#include <QGraphicsDropShadowEffect>

// ─────────────────────────────────────────────────────────────────────────────

/*static*/
void ToastNotification::show(QWidget*       parent,
                               const QString& title,
                               const QString& message,
                               const QString& command,
                               int            durationMs)
{
    // Cada toast é auto-deletável — sem necessidade de gerenciar o ponteiro
    auto* toast = new ToastNotification(parent, title, message, command, durationMs);
    toast->reposition();
    toast->QWidget::show();

    // Fade-in
    toast->setWindowOpacity(0.0);
    auto* fadeIn = new QPropertyAnimation(toast, "windowOpacity", toast);
    fadeIn->setDuration(250);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
}

// ─────────────────────────────────────────────────────────────────────────────

ToastNotification::ToastNotification(QWidget*       parent,
                                      const QString& title,
                                      const QString& message,
                                      const QString& command,
                                      int            durationMs)
    : QWidget(parent,
              Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    , m_totalMs(durationMs)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFixedWidth(360);

    // ── Sombra ────────────────────────────────────────────────────────────────
    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(18);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 80));
    setGraphicsEffect(shadow);

    // ── Layout ────────────────────────────────────────────────────────────────
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto* card = new QFrame();
    card->setObjectName("toastCard");
    card->setStyleSheet(
        "#toastCard {"
        "  background: #1a202c;"
        "  border-radius: 10px;"
        "  border: 1px solid #2d3748;"
        "}"
    );
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(8);

    // ── Linha de título + fechar ──────────────────────────────────────────────
    auto* topRow = new QHBoxLayout();
    auto* lblTitle = new QLabel(title);
    lblTitle->setStyleSheet(
        "font-size: 13px; font-weight: bold; color: #f6e05e;");
    topRow->addWidget(lblTitle);
    topRow->addStretch();

    auto* btnClose = new QPushButton("✕");
    btnClose->setFixedSize(20, 20);
    btnClose->setFlat(true);
    btnClose->setStyleSheet(
        "QPushButton { color: #718096; font-size: 11px; border: none; }"
        "QPushButton:hover { color: #e2e8f0; }");
    topRow->addWidget(btnClose);
    layout->addLayout(topRow);

    // ── Mensagem ──────────────────────────────────────────────────────────────
    auto* lblMsg = new QLabel(message);
    lblMsg->setWordWrap(true);
    lblMsg->setStyleSheet("font-size: 12px; color: #cbd5e0; line-height: 1.5;");
    layout->addWidget(lblMsg);

    // ── Bloco de comando (se fornecido) ──────────────────────────────────────
    if (!command.isEmpty()) {
        auto* cmdFrame = new QFrame();
        cmdFrame->setStyleSheet(
            "background: #2d3748; border-radius: 6px; padding: 6px 10px;");
        auto* cmdLayout = new QHBoxLayout(cmdFrame);
        cmdLayout->setContentsMargins(8, 6, 8, 6);
        cmdLayout->setSpacing(8);

        auto* lblCmd = new QLabel(command);
        lblCmd->setStyleSheet(
            "font-family: monospace; font-size: 12px; color: #68d391;"
            "background: transparent;");
        cmdLayout->addWidget(lblCmd, 1);

        auto* btnCopy = new QPushButton("📋 Copiar");
        btnCopy->setFixedHeight(24);
        btnCopy->setStyleSheet(
            "QPushButton { background: #4a5568; color: #e2e8f0; border-radius: 4px;"
            "  font-size: 11px; padding: 0 8px; border: none; }"
            "QPushButton:hover { background: #2b6cb0; color: white; }");
        cmdLayout->addWidget(btnCopy);

        layout->addWidget(cmdFrame);

        const QString cmd = command;
        connect(btnCopy, &QPushButton::clicked, this, [this, btnCopy, cmd]() {
            QApplication::clipboard()->setText(cmd);
            btnCopy->setText("✅ Copiado!");
            QTimer::singleShot(2000, btnCopy, [btnCopy]() {
                if (btnCopy) btnCopy->setText("📋 Copiar");
            });
        });
    }

    // ── Barra de progresso regressiva ────────────────────────────────────────
    auto* sepLine = new QFrame();
    sepLine->setFrameShape(QFrame::HLine);
    sepLine->setStyleSheet("color: #2d3748;");
    layout->addWidget(sepLine);

    m_progressBar = new QLabel();
    m_progressBar->setFixedHeight(4);
    m_progressBar->setStyleSheet(
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "  stop:0 #3182ce, stop:1 #63b3ed);"
        "border-radius: 2px;");
    layout->addWidget(m_progressBar);

    outer->addWidget(card);

    // ── Timer de progresso + auto-close ──────────────────────────────────────
    m_tickTimer = new QTimer(this);
    m_tickTimer->setInterval(50);   // atualiza a cada 50ms (suave)
    connect(m_tickTimer, &QTimer::timeout, this, [this]() {
        m_elapsedMs += 50;
        const double ratio = 1.0 - (double(m_elapsedMs) / m_totalMs);
        const int    w     = qMax(0, int(width() * ratio));
        m_progressBar->setFixedWidth(w);

        if (m_elapsedMs >= m_totalMs) {
            m_tickTimer->stop();
            // Fade-out antes de fechar
            m_fadeAnim = new QPropertyAnimation(this, "windowOpacity", this);
            m_fadeAnim->setDuration(400);
            m_fadeAnim->setStartValue(1.0);
            m_fadeAnim->setEndValue(0.0);
            connect(m_fadeAnim, &QPropertyAnimation::finished, this, &QWidget::close);
            m_fadeAnim->start(QAbstractAnimation::DeleteWhenStopped);
        }
    });
    m_tickTimer->start();

    connect(btnClose, &QPushButton::clicked, this, [this]() {
        m_tickTimer->stop();
        close();
    });

    adjustSize();
}

void ToastNotification::reposition()
{
    // Posiciona no canto inferior direito do widget pai
    QWidget* par = parentWidget();
    QPoint   base;

    if (par) {
        const QRect pg = par->geometry();
        // mapToGlobal para janelas que podem estar em monitores diferentes
        base = par->mapToGlobal(QPoint(pg.width()  - width()  - 20,
                                       pg.height() - height() - 40));
    } else {
        const QRect screen = QApplication::primaryScreen()->availableGeometry();
        base = QPoint(screen.right()  - width()  - 20,
                      screen.bottom() - height() - 40);
    }
    move(base);
}
