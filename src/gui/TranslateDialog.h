#pragma once

#include <QDialog>
#include <QTextEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSharedPointer>

class Entry;
class Database;

/**
 * TranslateDialog — Tradução de notas/entradas para qualquer idioma.
 *
 * Tradução in-app: LibreTranslate (API gratuita, sem chave de API).
 *   Endpoint primário : translate.argosopentech.com
 *   Endpoint secundário: libretranslate.de
 * Fallback no browser : Google Translate / DeepL (URL com texto).
 *
 * Uso:
 *   TranslateDialog::translateEntry(entry, db, parentWidget);
 */
class TranslateDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TranslateDialog(Entry* entry,
                              QSharedPointer<Database> db,
                              QWidget* parent = nullptr);

    static void translateEntry(Entry* entry,
                                QSharedPointer<Database> db,
                                QWidget* parent = nullptr);

private slots:
    void onTranslate();
    void onReplyFinished(QNetworkReply* reply);
    void onOpenBrowser(const QString& service);
    void onInsertInNote();
    void onSourceChanged();

private:
    void setStatus(const QString& msg, const QString& color = "#2c5282");
    void doRequest(const QString& endpoint, const QString& text,
                   const QString& src, const QString& tgt);

    static QStringList langCodes();
    static QStringList langNames();

    Entry*                   m_entry = nullptr;
    QSharedPointer<Database> m_db;

    QTextEdit*   m_srcText    = nullptr;
    QTextEdit*   m_dstText    = nullptr;
    QComboBox*   m_srcLang    = nullptr;
    QComboBox*   m_dstLang    = nullptr;
    QLabel*      m_statusLbl  = nullptr;
    QPushButton* m_btnTrans   = nullptr;
    QPushButton* m_btnInsert  = nullptr;

    QNetworkAccessManager* m_nam    = nullptr;
    QNetworkReply*         m_reply  = nullptr;
    int                    m_tryIdx = 0;   // 0 = primário, 1 = secundário

    static const QStringList ENDPOINTS;
};
