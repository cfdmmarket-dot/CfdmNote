#include "ShareDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QClipboard>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QIcon>
#include <QFrame>

#include "core/Entry.h"
#include "core/EntryAttributes.h"

// ─── Construtor ───────────────────────────────────────────────────────────────

ShareDialog::ShareDialog(const QString& entryTitle,
                         const QString& entryUrl,
                         const QString& entryNotes,
                         const QString& entryComment,
                         QWidget* parent)
    : QDialog(parent)
    , m_entryUrl(entryUrl)
{
    setWindowTitle(tr("Compartilhar entrada"));
    setMinimumWidth(480);
    setMinimumHeight(360);

    buildShareText(entryTitle, entryUrl, entryNotes, entryComment);

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(14, 14, 14, 14);

    // Aviso de segurança
    auto* warnLabel = new QLabel(
        tr("⚠  <b>Senha NÃO incluída.</b>  Verifique o conteúdo antes de enviar."));
    warnLabel->setStyleSheet(
        "QLabel { background: #fff3cd; color: #856404; border: 1px solid #ffc107; "
        "border-radius: 4px; padding: 6px 10px; font-size: 11px; }");
    warnLabel->setWordWrap(true);
    layout->addWidget(warnLabel);

    // Preview do texto a ser compartilhado
    auto* previewLabel = new QLabel(tr("Texto que será compartilhado:"));
    previewLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: palette(text);");
    layout->addWidget(previewLabel);

    m_preview = new QTextEdit();
    m_preview->setReadOnly(false);
    m_preview->setPlainText(m_shareText);
    m_preview->setStyleSheet("font-size: 12px; border: 1px solid palette(mid); "
                              "border-radius: 3px;");
    layout->addWidget(m_preview, 1);

    auto* editNote = new QLabel(tr("Você pode editar o texto acima antes de compartilhar."));
    editNote->setStyleSheet("font-size: 10px; color: palette(mid);");
    layout->addWidget(editNote);

    // Separador
    auto* sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    layout->addWidget(sep);

    // Botões de plataforma
    auto* platformLabel = new QLabel(tr("Compartilhar via:"));
    platformLabel->setStyleSheet("font-size: 11px; font-weight: bold;");
    layout->addWidget(platformLabel);

    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(6);

    auto makePlatformBtn = [&](const QString& text, const QString& icon) -> QPushButton* {
        auto* btn = new QPushButton(QIcon::fromTheme(icon), text);
        btn->setFixedHeight(34);
        btn->setStyleSheet("font-size: 11px; font-weight: bold;");
        return btn;
    };

    auto* btnWhatsApp  = makePlatformBtn(tr("WhatsApp"),  "phone");
    auto* btnTelegram  = makePlatformBtn(tr("Telegram"),  "mail-message");
    auto* btnDiscord   = makePlatformBtn(tr("Discord\n(copiar)"), "edit-copy");
    auto* btnFacebook  = makePlatformBtn(tr("Facebook"),  "internet-web-browser");
    auto* btnCopy      = makePlatformBtn(tr("Copiar"),    "edit-copy");

    btnRow->addWidget(btnWhatsApp);
    btnRow->addWidget(btnTelegram);
    btnRow->addWidget(btnDiscord);
    btnRow->addWidget(btnFacebook);
    btnRow->addStretch();
    btnRow->addWidget(btnCopy);
    layout->addLayout(btnRow);

    // Botão Fechar
    auto* closeRow = new QHBoxLayout();
    auto* btnClose = new QPushButton(tr("Fechar"));
    btnClose->setFixedHeight(30);
    closeRow->addStretch();
    closeRow->addWidget(btnClose);
    layout->addLayout(closeRow);

    // Conexões
    connect(btnWhatsApp, &QPushButton::clicked, this, &ShareDialog::onWhatsApp);
    connect(btnTelegram, &QPushButton::clicked, this, &ShareDialog::onTelegram);
    connect(btnDiscord,  &QPushButton::clicked, this, &ShareDialog::onDiscord);
    connect(btnFacebook, &QPushButton::clicked, this, &ShareDialog::onFacebook);
    connect(btnCopy,     &QPushButton::clicked, this, &ShareDialog::onCopy);
    connect(btnClose,    &QPushButton::clicked, this, &QDialog::accept);
}

// ─── Métodos estáticos ────────────────────────────────────────────────────────

void ShareDialog::shareEntry(Entry* entry, QWidget* parent)
{
    if (!entry) return;

    const QString title   = entry->title();
    const QString url     = entry->url();
    const QString notes   = entry->notes();
    const QString comment = entry->attributes()->value("CfdmNote::Comment");

    ShareDialog dlg(title, url, notes, comment, parent);
    dlg.exec();
}

// ─── Construção do texto ──────────────────────────────────────────────────────

void ShareDialog::buildShareText(const QString& title,
                                  const QString& url,
                                  const QString& notes,
                                  const QString& comment)
{
    QStringList parts;

    if (!title.isEmpty())
        parts << QString("*%1*").arg(title);

    if (!comment.isEmpty())
        parts << comment;

    if (!url.isEmpty())
        parts << QString("🔗 %1").arg(url);

    if (!notes.isEmpty()) {
        // Extrato (máx. 300 caracteres)
        QString excerpt = notes.left(300).trimmed();
        if (notes.length() > 300) excerpt += "...";
        parts << QString("\n%1").arg(excerpt);
    }

    parts << QString("\n— %1").arg(tr("Enviado via CfdmNote"));

    m_shareText = parts.join("\n");
}

// ─── Slots ────────────────────────────────────────────────────────────────────

void ShareDialog::onWhatsApp()
{
    const QString text = m_preview->toPlainText();
    const QString encoded = QUrl::toPercentEncoding(text);
    openUrl(QString("https://api.whatsapp.com/send?text=%1").arg(encoded));
}

void ShareDialog::onTelegram()
{
    const QString text = m_preview->toPlainText();
    if (!m_entryUrl.isEmpty()) {
        openUrl(QString("https://t.me/share/url?url=%1&text=%2")
                    .arg(QString(QUrl::toPercentEncoding(m_entryUrl)),
                         QString(QUrl::toPercentEncoding(text))));
    } else {
        // Sem URL de entrada: compartilha só o texto via URI tg://
        const QString encoded = QUrl::toPercentEncoding(text);
        openUrl(QString("https://t.me/share/url?url=%1&text=").arg(encoded));
    }
}

void ShareDialog::onDiscord()
{
    QApplication::clipboard()->setText(m_preview->toPlainText());
    // Discord não tem URI scheme padrão para mobile/desktop — copia para colar
    auto* info = new QLabel(tr("✅  Copiado para a área de transferência!\n"
                                "Cole no Discord com Ctrl+V."), this);
    info->setStyleSheet("QLabel { color: #5865F2; font-weight: bold; font-size: 11px; }");
    info->setAlignment(Qt::AlignCenter);
    // Mostra por 2 segundos via tooltip
    QApplication::clipboard()->setText(m_preview->toPlainText());
    setToolTip(tr("Copiado para a área de transferência — Cole no Discord (Ctrl+V)"));
    info->deleteLater();
}

void ShareDialog::onFacebook()
{
    const QString text = m_preview->toPlainText();
    const QString encodedText = QUrl::toPercentEncoding(text);
    if (!m_entryUrl.isEmpty()) {
        openUrl(QString("https://www.facebook.com/sharer/sharer.php?u=%1&quote=%2")
                    .arg(QString(QUrl::toPercentEncoding(m_entryUrl)), encodedText));
    } else {
        openUrl(QString("https://www.facebook.com/sharer/sharer.php?quote=%1")
                    .arg(encodedText));
    }
}

void ShareDialog::onCopy()
{
    QApplication::clipboard()->setText(m_preview->toPlainText());
    setWindowTitle(tr("Compartilhar entrada — ✅ Copiado!"));
}

void ShareDialog::openUrl(const QString& url)
{
    QDesktopServices::openUrl(QUrl(url));
}
