/**
 * ImportDocDialog.cpp
 * Importa documentos como entradas no banco CfdmNote.
 *
 * Formatos:
 *   .txt .md .log .csv     → QFile direto
 *   .pdf                   → pdftotext -layout - (poppler-utils)
 *   .docx                  → unzip -p word/document.xml + stripXml
 *   .odt                   → unzip -p content.xml + stripXml
 *   .rtf                   → unrtf --text
 *   .html .htm             → stripHtml (regex Qt)
 *   .png .jpg .jpeg .bmp   → tesseract (OCR)
 *   .tif .tiff .webp .gif  → tesseract (OCR)
 */

#include "ImportDocDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QApplication>
#include <QRegularExpression>
#include <QTemporaryFile>
#include <QUuid>
#include <QDialogButtonBox>

#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"

// ─── Static factory ───────────────────────────────────────────────────────────

void ImportDocDialog::importDocs(QSharedPointer<Database> db, QWidget* parent)
{
    ImportDocDialog dlg(db, parent);
    dlg.exec();
}

// ─── Constructor ─────────────────────────────────────────────────────────────

ImportDocDialog::ImportDocDialog(QSharedPointer<Database> db, QWidget* parent)
    : QDialog(parent)
    , m_db(db)
{
    setWindowTitle(tr("Importar Documentos como Notas"));
    setMinimumSize(600, 460);

    auto* lay = new QVBoxLayout(this);

    // ── Seletor de grupo destino ──────────────────────────────────────────────
    auto* grpBox = new QGroupBox(tr("Grupo de destino"), this);
    auto* grpLay = new QHBoxLayout(grpBox);
    m_groupCombo = new QComboBox(this);
    populateGroupCombo(db->rootGroup());
    grpLay->addWidget(m_groupCombo);
    lay->addWidget(grpBox);

    // ── Lista de arquivos ─────────────────────────────────────────────────────
    auto* fileBox = new QGroupBox(tr("Arquivos a importar"), this);
    auto* fileLay = new QVBoxLayout(fileBox);
    m_fileList = new QListWidget(this);
    m_fileList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    fileLay->addWidget(m_fileList);

    auto* btnRow = new QHBoxLayout;
    m_btnAdd    = new QPushButton(tr("➕ Adicionar..."), this);
    m_btnRemove = new QPushButton(tr("🗑 Remover"), this);
    btnRow->addWidget(m_btnAdd);
    btnRow->addWidget(m_btnRemove);
    btnRow->addStretch();
    fileLay->addLayout(btnRow);
    lay->addWidget(fileBox);

    // ── Progresso ────────────────────────────────────────────────────────────
    m_progress = new QProgressBar(this);
    m_progress->setVisible(false);
    lay->addWidget(m_progress);

    m_statusLabel = new QLabel(
        tr("Formatos: txt, md, pdf, docx, odt, rtf, html — e imagens (PNG/JPG/BMP/TIFF) via OCR"),
        this);
    m_statusLabel->setWordWrap(true);
    lay->addWidget(m_statusLabel);

    // ── Botões OK/Cancel ──────────────────────────────────────────────────────
    auto* bb = new QDialogButtonBox(this);
    m_btnImport = bb->addButton(tr("Importar"), QDialogButtonBox::AcceptRole);
    bb->addButton(QDialogButtonBox::Cancel);
    lay->addWidget(bb);

    connect(m_btnAdd,    &QPushButton::clicked, this, &ImportDocDialog::onAddFiles);
    connect(m_btnRemove, &QPushButton::clicked, this, &ImportDocDialog::onRemoveSelected);
    connect(m_btnImport, &QPushButton::clicked, this, &ImportDocDialog::onImport);
    connect(bb, &QDialogButtonBox::rejected,    this, &QDialog::reject);
}

// ─── Slots ────────────────────────────────────────────────────────────────────

void ImportDocDialog::onAddFiles()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this,
        tr("Selecionar documentos ou imagens"),
        QDir::homePath(),
        tr("Documentos e Imagens ("
           "*.txt *.md *.log *.csv "
           "*.pdf *.docx *.odt *.rtf "
           "*.html *.htm "
           "*.png *.jpg *.jpeg *.bmp *.tif *.tiff *.webp *.gif"
           ");;Todos os arquivos (*)")
    );
    for (const QString& f : files) {
        // evitar duplicatas
        bool dup = false;
        for (int i = 0; i < m_fileList->count(); ++i) {
            if (m_fileList->item(i)->data(Qt::UserRole).toString() == f) {
                dup = true; break;
            }
        }
        if (!dup) {
            auto* item = new QListWidgetItem(QFileInfo(f).fileName());
            item->setData(Qt::UserRole, f);
            item->setToolTip(f);
            m_fileList->addItem(item);
        }
    }
}

void ImportDocDialog::onRemoveSelected()
{
    for (auto* item : m_fileList->selectedItems())
        delete item;
}

void ImportDocDialog::onImport()
{
    if (m_fileList->count() == 0) {
        QMessageBox::information(this, tr("Importar"), tr("Adicione ao menos um arquivo."));
        return;
    }
    if (!m_db) {
        QMessageBox::warning(this, tr("Importar"), tr("Nenhum banco aberto."));
        return;
    }

    // Grupo destino
    Group* destGroup = m_db->rootGroup();
    int idx = m_groupCombo->currentIndex();
    if (idx >= 0 && idx < m_groups.size())
        destGroup = m_groups.at(idx);

    m_progress->setVisible(true);
    m_progress->setRange(0, m_fileList->count());
    m_progress->setValue(0);
    m_btnImport->setEnabled(false);
    m_btnAdd->setEnabled(false);
    m_btnRemove->setEnabled(false);

    int ok = 0, fail = 0;
    QStringList failedFiles;

    for (int i = 0; i < m_fileList->count(); ++i) {
        const QString path = m_fileList->item(i)->data(Qt::UserRole).toString();
        const QString name = QFileInfo(path).completeBaseName();
        m_statusLabel->setText(tr("Processando: %1").arg(QFileInfo(path).fileName()));
        QApplication::processEvents();

        QString errOut;
        const QString text = extractText(path, errOut);

        auto* entry = new Entry();
        entry->setTitle(name);
        if (!text.isEmpty()) {
            entry->setNotes(text);
            ++ok;
        } else {
            entry->setNotes(
                tr("[Falha ao extrair texto]\n%1").arg(errOut));
            ++fail;
            failedFiles << QFileInfo(path).fileName();
        }
        entry->setGroup(destGroup);

        m_progress->setValue(i + 1);
        QApplication::processEvents();
    }

    QString saveErr;
    m_db->save(Database::Atomic, {}, &saveErr);

    m_progress->setVisible(false);
    m_btnImport->setEnabled(true);
    m_btnAdd->setEnabled(true);
    m_btnRemove->setEnabled(true);

    QString msg = tr("%1 arquivo(s) importado(s) com sucesso.").arg(ok);
    if (fail > 0)
        msg += tr("\n%1 com falha: %2").arg(fail).arg(failedFiles.join(", "));
    if (!saveErr.isEmpty())
        msg += tr("\nAviso ao salvar: %1").arg(saveErr);

    m_statusLabel->setText(msg);
    QMessageBox::information(this, tr("Importar Documentos"), msg);
    accept();
}

// ─── Populate group combo ─────────────────────────────────────────────────────

void ImportDocDialog::populateGroupCombo(Group* group, int depth)
{
    if (!group) return;
    m_groups << group;
    const QString indent(depth * 2, QChar(' '));
    m_groupCombo->addItem(indent + group->name());

    for (Group* child : group->children())
        populateGroupCombo(child, depth + 1);
}

// ─── Dispatcher ──────────────────────────────────────────────────────────────

QString ImportDocDialog::extractText(const QString& filePath, QString& errorOut)
{
    const QString ext = QFileInfo(filePath).suffix().toLower();

    if (ext == "pdf")
        return extractPdf(filePath, errorOut);

    if (ext == "docx")
        return extractDocx(filePath, errorOut);

    if (ext == "odt")
        return extractOdt(filePath, errorOut);

    if (ext == "rtf")
        return extractRtf(filePath, errorOut);

    if (ext == "html" || ext == "htm")
        return extractHtml(filePath);

    // Imagens → OCR
    static const QStringList imgExts = {
        "png","jpg","jpeg","bmp","tif","tiff","webp","gif"
    };
    if (imgExts.contains(ext))
        return extractImage(filePath, errorOut);

    // Texto plano (txt, md, log, csv, …)
    return extractTxt(filePath);
}

// ─── Texto plano ──────────────────────────────────────────────────────────────

QString ImportDocDialog::extractTxt(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(f.readAll());
}

// ─── PDF ─────────────────────────────────────────────────────────────────────

QString ImportDocDialog::extractPdf(const QString& path, QString& err)
{
    // pdftotext -layout file.pdf - (poppler-utils)
    QString out = runProcess("pdftotext", {"-layout", path, "-"}, err);
    if (out.isEmpty() && err.isEmpty())
        err = tr("pdftotext não encontrado. Instale: sudo apt install poppler-utils");
    return out;
}

// ─── DOCX ─────────────────────────────────────────────────────────────────────

QString ImportDocDialog::extractDocx(const QString& path, QString& err)
{
    // unzip -p file.docx word/document.xml
    QString xml = runProcess("unzip", {"-p", path, "word/document.xml"}, err);
    if (xml.isEmpty()) {
        if (err.isEmpty())
            err = tr("Falha ao descompactar .docx (unzip não encontrado?)");
        return {};
    }
    return stripXmlTags(xml);
}

// ─── ODT ──────────────────────────────────────────────────────────────────────

QString ImportDocDialog::extractOdt(const QString& path, QString& err)
{
    QString xml = runProcess("unzip", {"-p", path, "content.xml"}, err);
    if (xml.isEmpty()) {
        if (err.isEmpty())
            err = tr("Falha ao descompactar .odt");
        return {};
    }
    return stripXmlTags(xml);
}

// ─── RTF ──────────────────────────────────────────────────────────────────────

QString ImportDocDialog::extractRtf(const QString& path, QString& err)
{
    // unrtf --text file.rtf
    QString out = runProcess("unrtf", {"--text", path}, err);
    if (out.isEmpty() && err.isEmpty())
        err = tr("unrtf não encontrado. Instale: sudo apt install unrtf");
    return out;
}

// ─── HTML ─────────────────────────────────────────────────────────────────────

QString ImportDocDialog::extractHtml(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return stripHtmlTags(QString::fromUtf8(f.readAll()));
}

// ─── Imagem / OCR ─────────────────────────────────────────────────────────────

QString ImportDocDialog::extractImage(const QString& path, QString& err)
{
    // tesseract <input> stdout -l por (Portuguese) + eng fallback
    // tesseract aceita: png, jpg, bmp, tif, tiff
    // Para webp/gif, tenta converter com convert (ImageMagick) antes

    const QString ext = QFileInfo(path).suffix().toLower();
    QString srcPath = path;
    QString tmpPath; // se criamos arquivo temporário

    if (ext == "webp" || ext == "gif") {
        // Converte para PNG temporário via ImageMagick
        tmpPath = QDir::tempPath() + "/cfdm_ocr_" +
                  QUuid::createUuid().toString(QUuid::Id128) + ".png";
        QString convErr;
        runProcess("convert", {path, tmpPath}, convErr);
        if (!QFile::exists(tmpPath)) {
            err = tr("Não foi possível converter %1 para PNG (ImageMagick instalado?).").arg(ext);
            return {};
        }
        srcPath = tmpPath;
    }

    // tesseract <input> stdout -l por+eng
    QString tErr;
    QString text = runProcess("tesseract", {srcPath, "stdout", "-l", "por+eng"}, tErr);

    if (!tmpPath.isEmpty())
        QFile::remove(tmpPath);

    if (text.isEmpty()) {
        // fallback: só inglês
        text = runProcess("tesseract", {srcPath, "stdout"}, tErr);
    }

    if (text.isEmpty()) {
        err = tr("tesseract falhou ou não encontrado.\n"
                 "Instale: sudo apt install tesseract-ocr tesseract-ocr-por\n%1").arg(tErr);
    }
    return text.trimmed();
}

// ─── Strip XML ────────────────────────────────────────────────────────────────

QString ImportDocDialog::stripXmlTags(const QString& xml)
{
    // Remove tags, decodifica entidades básicas, comprime espaços
    QString out = xml;
    out.remove(QRegularExpression("<[^>]+>"));
    out.replace("&amp;",  "&");
    out.replace("&lt;",   "<");
    out.replace("&gt;",   ">");
    out.replace("&quot;", "\"");
    out.replace("&apos;", "'");
    out.replace("&nbsp;", " ");
    // Comprime múltiplas linhas em branco para no máximo 2
    out.replace(QRegularExpression("\n{3,}"), "\n\n");
    return out.trimmed();
}

// ─── Strip HTML ───────────────────────────────────────────────────────────────

QString ImportDocDialog::stripHtmlTags(const QString& html)
{
    QString out = html;
    // <br> e </p> → newline
    out.replace(QRegularExpression("<br\\s*/?>",
        QRegularExpression::CaseInsensitiveOption), "\n");
    out.replace(QRegularExpression("</p>",
        QRegularExpression::CaseInsensitiveOption), "\n");
    out.replace(QRegularExpression("</h[1-6]>",
        QRegularExpression::CaseInsensitiveOption), "\n");
    // Remove todas as tags
    out.remove(QRegularExpression("<[^>]+>"));
    // Entidades HTML
    out.replace("&amp;",  "&");
    out.replace("&lt;",   "<");
    out.replace("&gt;",   ">");
    out.replace("&quot;", "\"");
    out.replace("&apos;", "'");
    out.replace("&nbsp;", " ");
    out.replace("&mdash;", "—");
    out.replace("&ndash;", "–");
    // Comprime espaços/linhas
    out.replace(QRegularExpression("[ \t]+"), " ");
    out.replace(QRegularExpression("\n{3,}"), "\n\n");
    return out.trimmed();
}

// ─── Run external process ─────────────────────────────────────────────────────

QString ImportDocDialog::runProcess(const QString& program,
                                     const QStringList& args,
                                     QString& err,
                                     int timeoutMs)
{
    QProcess proc;
    proc.start(program, args);
    if (!proc.waitForStarted(5000)) {
        err = tr("Não foi possível iniciar '%1': %2")
                  .arg(program, proc.errorString());
        return {};
    }
    proc.waitForFinished(timeoutMs);

    const QString out   = QString::fromUtf8(proc.readAllStandardOutput());
    const QString errTxt = QString::fromUtf8(proc.readAllStandardError());

    if (out.isEmpty() && !errTxt.isEmpty())
        err = errTxt.trimmed();

    return out;
}
