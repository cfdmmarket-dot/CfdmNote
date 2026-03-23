#include "ExportNoteDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QCheckBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>

#include <botan/auto_rng.h>
#include <botan/aead.h>
#include <botan/pbkdf.h>

#include "core/Entry.h"
#include "core/Database.h"
#include "core/EntryAttributes.h"
#include "notes/NoteManager.h"

// ─── Construtor ───────────────────────────────────────────────────────────────

ExportNoteDialog::ExportNoteDialog(Entry* entry,
                                    const QSharedPointer<Database>& db,
                                    QWidget* parent)
    : QDialog(parent)
    , m_entry(entry)
    , m_db(db)
{
    setWindowTitle(tr("Exportar Nota — %1").arg(entry ? entry->title() : tr("entrada")));
    setMinimumWidth(500);
    setMinimumHeight(420);

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(14, 14, 14, 14);

    // Título
    auto* titleLabel = new QLabel(
        tr("<b>Exportar nota da entrada:</b> %1")
            .arg(entry ? entry->title().toHtmlEscaped() : ""));
    titleLabel->setStyleSheet("font-size: 12px;");
    layout->addWidget(titleLabel);

    // Preview do conteúdo da nota
    auto* previewLabel = new QLabel(tr("Conteúdo da nota:"));
    previewLabel->setStyleSheet("font-size: 11px; font-weight: bold;");
    layout->addWidget(previewLabel);

    m_preview = new QTextEdit();
    m_preview->setReadOnly(true);
    m_preview->setPlainText(getNoteContent());
    m_preview->setStyleSheet("font-size: 11px; border: 1px solid palette(mid); "
                              "border-radius: 3px;");
    layout->addWidget(m_preview, 1);

    // Checkbox criptografia
    m_chkEncrypt = new QCheckBox(tr("Exportar com criptografia (AES-256-GCM)"));
    m_chkEncrypt->setStyleSheet("font-size: 11px; font-weight: bold;");
    layout->addWidget(m_chkEncrypt);

    // Box de senha (oculta por padrão)
    m_encryptBox = new QGroupBox(tr("Senha de criptografia"));
    m_encryptBox->setStyleSheet(
        "QGroupBox { font-size: 11px; border: 1px solid palette(mid); "
        "border-radius: 4px; margin-top: 6px; padding-top: 4px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; }");
    m_encryptBox->setVisible(false);

    auto* encLayout = new QVBoxLayout(m_encryptBox);
    auto* passRow1  = new QHBoxLayout();
    auto* passRow2  = new QHBoxLayout();

    auto* lblPass    = new QLabel(tr("Senha:"));
    auto* lblConfirm = new QLabel(tr("Confirmar:"));
    lblPass->setFixedWidth(70);
    lblConfirm->setFixedWidth(70);

    m_passEdit = new QLineEdit();
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passEdit->setPlaceholderText(tr("Senha para criptografar"));

    m_passConfirm = new QLineEdit();
    m_passConfirm->setEchoMode(QLineEdit::Password);
    m_passConfirm->setPlaceholderText(tr("Repita a senha"));

    passRow1->addWidget(lblPass);
    passRow1->addWidget(m_passEdit);
    passRow2->addWidget(lblConfirm);
    passRow2->addWidget(m_passConfirm);

    auto* infoLabel = new QLabel(
        tr("ℹ  PBKDF2-SHA256 (200.000 iter.) + AES-256-GCM. "
           "Extensão do arquivo: .cfdmnote"));
    infoLabel->setStyleSheet("font-size: 10px; color: palette(mid);");
    infoLabel->setWordWrap(true);

    encLayout->addLayout(passRow1);
    encLayout->addLayout(passRow2);
    encLayout->addWidget(infoLabel);
    layout->addWidget(m_encryptBox);

    // Status
    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("font-size: 11px;");
    m_statusLabel->setWordWrap(true);
    layout->addWidget(m_statusLabel);

    // Botões
    auto* btnRow   = new QHBoxLayout();
    auto* btnExport = new QPushButton(QIcon::fromTheme("document-save-as"), tr("Exportar..."));
    auto* btnClose  = new QPushButton(tr("Fechar"));
    btnExport->setFixedHeight(30);
    btnClose->setFixedHeight(30);
    btnRow->addStretch();
    btnRow->addWidget(btnExport);
    btnRow->addWidget(btnClose);
    layout->addLayout(btnRow);

    // Conexões
    connect(m_chkEncrypt, &QCheckBox::toggled, this, &ExportNoteDialog::onEncryptToggled);
    connect(btnExport,    &QPushButton::clicked, this, &ExportNoteDialog::onExport);
    connect(btnClose,     &QPushButton::clicked, this, &QDialog::accept);
}

// ─── Estático ─────────────────────────────────────────────────────────────────

void ExportNoteDialog::exportNote(Entry* entry,
                                   const QSharedPointer<Database>& db,
                                   QWidget* parent)
{
    if (!entry) return;
    ExportNoteDialog dlg(entry, db, parent);
    dlg.exec();
}

// ─── Conteúdo da nota ─────────────────────────────────────────────────────────

QString ExportNoteDialog::getNoteContent() const
{
    if (!m_entry) return {};

    // Tenta NoteManager primeiro (banco .ksdb)
    auto& nm = NoteManager::instance();
    if (nm.isOpen()) {
        const QString noteId = nm.getNoteIdForEntry(m_entry->uuidToHex());
        if (!noteId.isEmpty()) {
            const QString content = nm.getNoteContent(noteId);
            if (!content.isEmpty()) return content;
        }
    }

    // Fallback: campo Notes padrão da entrada
    const QString stdNotes = m_entry->notes();
    if (!stdNotes.isEmpty()) return stdNotes;

    // Fallback: campo CfdmNote::Comment
    return m_entry->attributes()->value("CfdmNote::Comment");
}

// ─── Slots ────────────────────────────────────────────────────────────────────

void ExportNoteDialog::onEncryptToggled(bool checked)
{
    m_encryptBox->setVisible(checked);
    adjustSize();
}

void ExportNoteDialog::onExport()
{
    const QString content = m_preview->toPlainText().trimmed();
    if (content.isEmpty()) {
        m_statusLabel->setText(tr("⚠  Nota vazia — nada a exportar."));
        m_statusLabel->setStyleSheet("color: #c08000; font-size: 11px;");
        return;
    }

    const bool encrypt = m_chkEncrypt->isChecked();

    if (encrypt) {
        const QString pw = m_passEdit->text();
        if (pw.isEmpty()) {
            m_statusLabel->setText(tr("⚠  Digite uma senha para criptografar."));
            m_statusLabel->setStyleSheet("color: #c00000; font-size: 11px;");
            return;
        }
        if (pw != m_passConfirm->text()) {
            m_statusLabel->setText(tr("⚠  As senhas não coincidem."));
            m_statusLabel->setStyleSheet("color: #c00000; font-size: 11px;");
            return;
        }
    }

    const QString entryTitle = m_entry ? m_entry->title() : tr("nota");
    const QString defaultName = encrypt
        ? QString("%1.cfdmnote").arg(entryTitle)
        : QString("%1.txt").arg(entryTitle);

    const QString filter = encrypt
        ? tr("Nota CfdmNote criptografada (*.cfdmnote);;Todos os arquivos (*)")
        : tr("Arquivo de texto (*.txt);;Todos os arquivos (*)");

    const QString path = QFileDialog::getSaveFileName(
        this, tr("Exportar nota"), QDir::homePath() + "/" + defaultName, filter);

    if (path.isEmpty()) return;

    bool ok = encrypt
        ? exportEncrypted(path, content, m_passEdit->text())
        : exportPlain(path, content);

    if (ok) {
        m_statusLabel->setText(tr("✅  Nota exportada: %1").arg(QFileInfo(path).fileName()));
        m_statusLabel->setStyleSheet("color: #007700; font-size: 11px; font-weight: bold;");
    }
}

// ─── Exportar texto simples ───────────────────────────────────────────────────

bool ExportNoteDialog::exportPlain(const QString& path, const QString& content)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_statusLabel->setText(tr("⚠  Não foi possível abrir o arquivo para escrita."));
        m_statusLabel->setStyleSheet("color: #c00000; font-size: 11px;");
        return false;
    }
    f.write(content.toUtf8());
    f.close();
    return true;
}

// ─── Exportar criptografado (AES-256-GCM + PBKDF2-SHA256) ────────────────────

bool ExportNoteDialog::exportEncrypted(const QString& path,
                                        const QString& content,
                                        const QString& password)
{
    try {
        Botan::AutoSeeded_RNG rng;

        // Gera salt (32 bytes) e nonce (12 bytes)
        std::vector<uint8_t> salt(32);
        std::vector<uint8_t> nonce(12);
        rng.randomize(salt.data(), salt.size());
        rng.randomize(nonce.data(), nonce.size());

        // Deriva chave de 32 bytes com PBKDF2-SHA256 (200.000 iterações)
        auto pbkdf = Botan::PBKDF::create("PBKDF2(SHA-256)");
        if (!pbkdf) throw std::runtime_error("PBKDF2 não disponível");

        Botan::secure_vector<uint8_t> key =
            pbkdf->derive_key(32, password.toStdString(),
                              salt.data(), salt.size(), 200000).bits_of();

        // Criptografa com AES-256-GCM
        auto enc = Botan::AEAD_Mode::create("AES-256/GCM", Botan::ENCRYPTION);
        if (!enc) throw std::runtime_error("AES-256/GCM não disponível");

        enc->set_key(key);
        enc->start(nonce);

        const QByteArray utf8 = content.toUtf8();
        Botan::secure_vector<uint8_t> buf(
            reinterpret_cast<const uint8_t*>(utf8.constData()),
            reinterpret_cast<const uint8_t*>(utf8.constData()) + utf8.size());
        enc->finish(buf);  // buf = ciphertext + 16-byte GCM tag

        // Grava arquivo
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly)) {
            throw std::runtime_error("Não foi possível criar o arquivo");
        }

        // Magic + version
        f.write("CFDMNOTE", 8);
        const uint8_t version = 1;
        f.write(reinterpret_cast<const char*>(&version), 1);

        // Salt + nonce
        f.write(reinterpret_cast<const char*>(salt.data()),  32);
        f.write(reinterpret_cast<const char*>(nonce.data()), 12);

        // Tamanho do ciphertext (4 bytes LE)
        const uint32_t cipherLen = static_cast<uint32_t>(buf.size());
        f.write(reinterpret_cast<const char*>(&cipherLen), 4);

        // Ciphertext + tag
        f.write(reinterpret_cast<const char*>(buf.data()),
                static_cast<qint64>(buf.size()));
        f.close();
        return true;

    } catch (const std::exception& e) {
        m_statusLabel->setText(tr("⚠  Erro ao criptografar: %1").arg(e.what()));
        m_statusLabel->setStyleSheet("color: #c00000; font-size: 11px;");
        return false;
    }
}
