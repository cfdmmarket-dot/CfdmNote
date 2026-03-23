#include "TranslateDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFrame>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QDesktopServices>
#include <QApplication>
#include <QClipboard>
#include <QShortcut>
#include <QKeySequence>
#include <QToolButton>

#include "core/Entry.h"
#include "core/Database.h"
#include "notes/NoteManager.h"

// ─── Endpoints LibreTranslate gratuitos (tentados em ordem) ──────────────────
const QStringList TranslateDialog::ENDPOINTS = {
    "https://translate.argosopentech.com/translate",
    "https://libretranslate.de/translate",
    "https://translate.terraprint.co/translate",
};

// ─── Idiomas suportados pelo LibreTranslate ───────────────────────────────────
QStringList TranslateDialog::langCodes()
{
    return {"auto","pt","en","es","fr","de","it","nl","ru","zh","ja",
            "ar","ko","pl","tr","uk","hi","sv","da","fi","no","el"};
}

QStringList TranslateDialog::langNames()
{
    return {
        QObject::tr("Detectar automaticamente"),
        QObject::tr("Português"),
        QObject::tr("Inglês"),
        QObject::tr("Espanhol"),
        QObject::tr("Francês"),
        QObject::tr("Alemão"),
        QObject::tr("Italiano"),
        QObject::tr("Holandês"),
        QObject::tr("Russo"),
        QObject::tr("Chinês"),
        QObject::tr("Japonês"),
        QObject::tr("Árabe"),
        QObject::tr("Coreano"),
        QObject::tr("Polonês"),
        QObject::tr("Turco"),
        QObject::tr("Ucraniano"),
        QObject::tr("Hindi"),
        QObject::tr("Sueco"),
        QObject::tr("Dinamarquês"),
        QObject::tr("Finlandês"),
        QObject::tr("Norueguês"),
        QObject::tr("Grego"),
    };
}

// ─────────────────────────────────────────────────────────────────────────────

TranslateDialog::TranslateDialog(Entry* entry,
                                  QSharedPointer<Database> db,
                                  QWidget* parent)
    : QDialog(parent)
    , m_entry(entry)
    , m_db(db)
{
    setWindowTitle(tr("Tradução de Notas"));
    setMinimumSize(680, 520);
    resize(760, 580);

    m_nam = new QNetworkAccessManager(this);
    connect(m_nam, &QNetworkAccessManager::finished,
            this,  &TranslateDialog::onReplyFinished);

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    layout->setContentsMargins(12, 12, 12, 12);

    // ── Cabeçalho ─────────────────────────────────────────────────────────────
    auto* header = new QLabel(
        tr("<b>🌐 Tradução</b>  —  powered by "
           "<a href='https://libretranslate.com'>LibreTranslate</a> (gratuito, sem conta)"));
    header->setOpenExternalLinks(true);
    header->setStyleSheet("font-size: 12px; color: #2c5282; padding: 2px 0;");
    layout->addWidget(header);

    // ── Seleção de idiomas + botão traduzir ──────────────────────────────────
    auto* langRow = new QHBoxLayout();

    langRow->addWidget(new QLabel(tr("De:")));
    m_srcLang = new QComboBox();
    const QStringList codes = langCodes();
    const QStringList names = langNames();
    for (int i = 0; i < codes.size(); ++i)
        m_srcLang->addItem(names[i], codes[i]);
    m_srcLang->setCurrentIndex(0);  // auto-detect
    langRow->addWidget(m_srcLang);

    // Botão inverter ↔
    auto* btnSwap = new QToolButton();
    btnSwap->setText("⇄");
    btnSwap->setFixedWidth(32);
    btnSwap->setToolTip(tr("Inverter idiomas"));
    langRow->addWidget(btnSwap);

    langRow->addWidget(new QLabel(tr("Para:")));
    m_dstLang = new QComboBox();
    for (int i = 1; i < codes.size(); ++i)  // sem "auto" no destino
        m_dstLang->addItem(names[i], codes[i]);
    m_dstLang->setCurrentIndex(0);  // Português
    langRow->addWidget(m_dstLang);

    langRow->addStretch();

    m_btnTrans = new QPushButton(tr("🌐  Traduzir"));
    m_btnTrans->setFixedHeight(32);
    m_btnTrans->setStyleSheet(
        "QPushButton { background: #2b6cb0; color: white; font-weight: bold;"
        "  border-radius: 5px; padding: 0 18px; font-size: 13px; }"
        "QPushButton:hover { background: #1a365d; }"
        "QPushButton:disabled { background: #bee3f8; color: #90cdf4; }");
    langRow->addWidget(m_btnTrans);
    layout->addLayout(langRow);

    // ── Área de texto: origem | destino ──────────────────────────────────────
    auto* textRow = new QHBoxLayout();
    textRow->setSpacing(8);

    // Origem
    auto* srcBox = new QGroupBox(tr("Texto original"));
    auto* srcLyt = new QVBoxLayout(srcBox);
    m_srcText = new QTextEdit();
    m_srcText->setPlaceholderText(tr("Cole ou escreva o texto a traduzir..."));
    srcLyt->addWidget(m_srcText);

    // Botões da área de origem
    auto* srcBtnRow = new QHBoxLayout();
    auto* btnClearSrc = new QPushButton(tr("Limpar"));
    btnClearSrc->setFixedHeight(24);
    auto* btnFromNote = new QPushButton(tr("← Da nota"));
    btnFromNote->setFixedHeight(24);
    btnFromNote->setEnabled(entry != nullptr);
    btnFromNote->setToolTip(tr("Carrega o texto da nota da entrada selecionada"));
    srcBtnRow->addWidget(btnFromNote);
    srcBtnRow->addStretch();
    srcBtnRow->addWidget(btnClearSrc);
    srcLyt->addLayout(srcBtnRow);
    textRow->addWidget(srcBox);

    // Destino
    auto* dstBox = new QGroupBox(tr("Tradução"));
    auto* dstLyt = new QVBoxLayout(dstBox);
    m_dstText = new QTextEdit();
    m_dstText->setPlaceholderText(tr("A tradução aparecerá aqui..."));
    dstLyt->addWidget(m_dstText);

    // Botões da área de destino
    auto* dstBtnRow = new QHBoxLayout();
    auto* btnCopy = new QPushButton(tr("Copiar"));
    btnCopy->setFixedHeight(24);
    m_btnInsert = new QPushButton(tr("→ Inserir na Nota"));
    m_btnInsert->setFixedHeight(24);
    m_btnInsert->setEnabled(false);
    m_btnInsert->setToolTip(tr("Insere a tradução no final da nota da entrada"));
    dstBtnRow->addWidget(m_btnInsert);
    dstBtnRow->addStretch();
    dstBtnRow->addWidget(btnCopy);
    dstLyt->addLayout(dstBtnRow);
    textRow->addWidget(dstBox);

    layout->addLayout(textRow, 1);

    // ── Status ────────────────────────────────────────────────────────────────
    m_statusLbl = new QLabel(tr("Pronto."));
    m_statusLbl->setStyleSheet("font-size: 11px; color: #4a5568;");
    m_statusLbl->setWordWrap(true);
    layout->addWidget(m_statusLbl);

    // ── Rodapé: fallback browser ──────────────────────────────────────────────
    auto* footRow = new QHBoxLayout();
    footRow->addWidget(new QLabel(tr("Abrir no navegador:")));

    auto* btnGoogle = new QPushButton(tr("Google Translate"));
    btnGoogle->setFixedHeight(26);
    btnGoogle->setStyleSheet(
        "QPushButton { border: 1px solid #bee3f8; border-radius: 4px; "
        "  color: #2b6cb0; padding: 0 10px; }"
        "QPushButton:hover { background: #ebf8ff; }");

    auto* btnDeepl = new QPushButton(tr("DeepL"));
    btnDeepl->setFixedHeight(26);
    btnDeepl->setStyleSheet(btnGoogle->styleSheet());

    footRow->addWidget(btnGoogle);
    footRow->addWidget(btnDeepl);
    footRow->addStretch();

    auto* btnClose = new QPushButton(tr("Fechar"));
    btnClose->setFixedHeight(26);
    footRow->addWidget(btnClose);
    layout->addLayout(footRow);

    // ── Pré-carrega nota da entrada ───────────────────────────────────────────
    if (entry) {
        const QString noteId =
            NoteManager::instance().getNoteIdForEntry(entry->uuidToHex());
        QString noteText;
        if (!noteId.isEmpty())
            noteText = NoteManager::instance().getNoteContent(noteId);
        if (noteText.isEmpty())
            noteText = entry->notes();
        if (!noteText.isEmpty())
            m_srcText->setPlainText(noteText);
    }

    // ── Conexões ─────────────────────────────────────────────────────────────
    connect(m_btnTrans,  &QPushButton::clicked, this, &TranslateDialog::onTranslate);
    connect(m_btnInsert, &QPushButton::clicked, this, &TranslateDialog::onInsertInNote);
    connect(btnClose,    &QPushButton::clicked, this, &QDialog::accept);

    connect(btnClearSrc, &QPushButton::clicked, m_srcText, &QTextEdit::clear);
    connect(btnCopy, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_dstText->toPlainText());
        setStatus(tr("✅ Tradução copiada para a área de transferência."), "#276749");
    });
    connect(btnFromNote, &QPushButton::clicked, this, [this]() {
        if (!m_entry) return;
        const QString noteId =
            NoteManager::instance().getNoteIdForEntry(m_entry->uuidToHex());
        QString text;
        if (!noteId.isEmpty())
            text = NoteManager::instance().getNoteContent(noteId);
        if (text.isEmpty())
            text = m_entry->notes();
        m_srcText->setPlainText(text);
    });
    connect(btnSwap, &QToolButton::clicked, this, [this]() {
        // Troca texto e idiomas (exceto auto)
        const QString tmp = m_srcText->toPlainText();
        m_srcText->setPlainText(m_dstText->toPlainText());
        m_dstText->setPlainText(tmp);

        const int srcIdx = m_srcLang->currentIndex();
        // src tem "auto" no índice 0, dst não tem "auto"
        // Se srcIdx == 0 (auto), define src = pt após swap
        const int dstIdx = m_dstLang->currentIndex();
        if (srcIdx > 0)
            m_dstLang->setCurrentIndex(srcIdx - 1);  // -1 por causa do "auto"
        m_srcLang->setCurrentIndex(dstIdx + 1);       // +1 para compensar
    });
    connect(m_dstText, &QTextEdit::textChanged, this, [this]() {
        m_btnInsert->setEnabled(m_entry != nullptr &&
                                !m_dstText->toPlainText().trimmed().isEmpty());
    });
    connect(btnGoogle, &QPushButton::clicked, this,
            [this]() { onOpenBrowser("google"); });
    connect(btnDeepl,  &QPushButton::clicked, this,
            [this]() { onOpenBrowser("deepl"); });

    connect(new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), this),
            &QShortcut::activated, this, &TranslateDialog::onTranslate);
}

// ─────────────────────────────────────────────────────────────────────────────

void TranslateDialog::translateEntry(Entry* entry,
                                      QSharedPointer<Database> db,
                                      QWidget* parent)
{
    TranslateDialog dlg(entry, db, parent);
    dlg.exec();
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

void TranslateDialog::setStatus(const QString& msg, const QString& color)
{
    m_statusLbl->setText(msg);
    m_statusLbl->setStyleSheet(
        QString("font-size: 11px; color: %1;").arg(color));
}

void TranslateDialog::doRequest(const QString& endpoint,
                                 const QString& text,
                                 const QString& src,
                                 const QString& tgt)
{
    QUrl reqUrl(endpoint);
    QNetworkRequest req(reqUrl);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Accept", "application/json");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    QJsonObject body;
    body["q"]       = text;
    body["source"]  = src;
    body["target"]  = tgt;
    body["format"]  = "text";

    m_reply = m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
}

// ─────────────────────────────────────────────────────────────────────────────
// Slots
// ─────────────────────────────────────────────────────────────────────────────

void TranslateDialog::onTranslate()
{
    const QString text = m_srcText->toPlainText().trimmed();
    if (text.isEmpty()) {
        setStatus(tr("Digite ou cole o texto de origem primeiro."), "#c05621");
        return;
    }

    // Limite de caracteres para não sobrecarregar a API gratuita
    if (text.length() > 5000) {
        setStatus(tr("⚠ Texto muito longo (%1 chars). A API gratuita suporta até 5000 chars. "
                     "Use o fallback no navegador para textos maiores.").arg(text.length()),
                  "#c05621");
        return;
    }

    const QString srcCode = m_srcLang->currentData().toString();
    const QString dstCode = m_dstLang->currentData().toString();

    m_tryIdx = 0;
    m_btnTrans->setEnabled(false);
    setStatus(tr("⏳ Traduzindo via LibreTranslate..."));

    doRequest(ENDPOINTS[m_tryIdx], text, srcCode, dstCode);
}

void TranslateDialog::onReplyFinished(QNetworkReply* reply)
{
    reply->deleteLater();
    m_reply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        // Tenta próximo endpoint
        ++m_tryIdx;
        if (m_tryIdx < ENDPOINTS.size()) {
            setStatus(tr("⏳ Tentando endpoint alternativo (%1/%2)...")
                          .arg(m_tryIdx + 1).arg(ENDPOINTS.size()));
            doRequest(ENDPOINTS[m_tryIdx],
                      m_srcText->toPlainText().trimmed(),
                      m_srcLang->currentData().toString(),
                      m_dstLang->currentData().toString());
            return;
        }

        // Todos falharam
        m_btnTrans->setEnabled(true);
        setStatus(tr("❌ Sem conexão com os servidores LibreTranslate. "
                     "Verifique a internet ou use o Google Translate / DeepL abaixo."),
                  "#c05621");
        return;
    }

    const QByteArray data = reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isNull() || !doc.object().contains("translatedText")) {
        // Pode ser rate-limit ou erro da API — tenta próximo
        ++m_tryIdx;
        if (m_tryIdx < ENDPOINTS.size()) {
            setStatus(tr("⏳ Servidor ocupado, tentando alternativa..."));
            doRequest(ENDPOINTS[m_tryIdx],
                      m_srcText->toPlainText().trimmed(),
                      m_srcLang->currentData().toString(),
                      m_dstLang->currentData().toString());
            return;
        }

        m_btnTrans->setEnabled(true);
        setStatus(tr("❌ Resposta inválida dos servidores. Use o fallback no navegador."),
                  "#c05621");
        return;
    }

    const QString translated = doc.object()["translatedText"].toString();
    m_dstText->setPlainText(translated);
    m_btnTrans->setEnabled(true);

    const QString srcName = m_srcLang->currentText();
    const QString dstName = m_dstLang->currentText();
    setStatus(tr("✅ Tradução concluída: %1 → %2  (%3 chars → %4 chars)")
                  .arg(srcName, dstName)
                  .arg(m_srcText->toPlainText().trimmed().length())
                  .arg(translated.length()),
              "#276749");
}

void TranslateDialog::onOpenBrowser(const QString& service)
{
    const QString text = m_srcText->toPlainText().trimmed();
    const QString src  = m_srcLang->currentData().toString();
    const QString dst  = m_dstLang->currentData().toString();
    const QString enc  = QString::fromUtf8(QUrl::toPercentEncoding(text.left(3000)));

    QString urlStr;
    if (service == "google") {
        urlStr = QString("https://translate.google.com/?sl=%1&tl=%2&text=%3&op=translate")
                     .arg(src == "auto" ? "auto" : src, dst, enc);
    } else if (service == "deepl") {
        // DeepL usa formato: /translator#idioma_src/idioma_dst/texto
        const QString srcDl = src == "auto" ? "auto" : src.toUpper();
        urlStr = QString("https://www.deepl.com/translator#%1/%2/%3")
                     .arg(srcDl, dst.toUpper(), enc);
    }

    if (!urlStr.isEmpty())
        QDesktopServices::openUrl(QUrl(urlStr));
}

void TranslateDialog::onInsertInNote()
{
    if (!m_entry) return;
    const QString translated = m_dstText->toPlainText().trimmed();
    if (translated.isEmpty()) return;

    const QString srcName = m_srcLang->currentText();
    const QString dstName = m_dstLang->currentText();
    const QString block   = QString("\n\n---\n🌐 Tradução (%1 → %2):\n%3")
                                .arg(srcName, dstName, translated);

    // Salva via NoteManager
    const QString uuid   = m_entry->uuidToHex();
    const QString noteId = NoteManager::instance().getNoteIdForEntry(uuid);
    if (!noteId.isEmpty()) {
        const QString current = NoteManager::instance().getNoteContent(noteId);
        NoteManager::instance().saveNoteContent(noteId, current + block);
    }

    // Também nas notes nativas
    m_entry->setNotes(m_entry->notes() + block);
    if (m_db) {
        QString err;
        m_db->save(Database::Atomic, {}, &err);
    }

    setStatus(tr("✅ Tradução inserida na nota da entrada."), "#276749");
    m_btnInsert->setEnabled(false);
}

void TranslateDialog::onSourceChanged()
{
    m_dstText->clear();
}
