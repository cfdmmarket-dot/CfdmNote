#pragma once

#include <QDialog>
#include <QTextEdit>
#include <QLabel>
#include <QString>

class Entry;

/**
 * ShareDialog — Compartilhar informações da entrada via apps de mensagens.
 *
 * Compartilha: Título, URL, Comentário e extrato das Notas.
 * NUNCA inclui senha ou dados sensíveis.
 *
 * Plataformas suportadas:
 *   WhatsApp, Telegram, Discord (copia), Facebook
 */
class ShareDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ShareDialog(const QString& entryTitle,
                         const QString& entryUrl,
                         const QString& entryNotes,
                         const QString& entryComment,
                         QWidget* parent = nullptr);

    static void shareEntry(Entry* entry, QWidget* parent = nullptr);

private slots:
    void onWhatsApp();
    void onTelegram();
    void onDiscord();
    void onFacebook();
    void onCopy();

private:
    void buildShareText(const QString& title,
                        const QString& url,
                        const QString& notes,
                        const QString& comment);
    void openUrl(const QString& url);

    QString    m_shareText;
    QString    m_entryUrl;
    QTextEdit* m_preview = nullptr;
};
