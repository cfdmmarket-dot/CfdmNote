#include "CsvImportDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include <QDir>

#include "core/Database.h"
#include "core/Group.h"
#include "core/Entry.h"
#include "core/Metadata.h"
#include "gui/MainWindow.h"
#include "gui/DatabaseWidget.h"
#include "gui/DatabaseTabWidget.h"

// ─── Parser CSV mínimo (suporta campos entre aspas com vírgulas internas) ─────

static QStringList parseCsvLine(const QString& line)
{
    QStringList fields;
    QString field;
    bool inQuotes = false;

    for (int i = 0; i < line.size(); ++i) {
        const QChar c = line[i];
        if (inQuotes) {
            if (c == '"') {
                // Aspas duplas dentro de campo entre aspas → aspas literal
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                field += c;
            }
        } else {
            if (c == '"') {
                inQuotes = true;
            } else if (c == ',') {
                fields << field;
                field.clear();
            } else {
                field += c;
            }
        }
    }
    fields << field;
    return fields;
}

// ─── Construtor ───────────────────────────────────────────────────────────────

CsvImportDialog::CsvImportDialog(QSharedPointer<Database> db,
                                  MainWindow* mainWindow,
                                  QWidget* parent)
    : QDialog(parent)
    , m_db(db)
    , m_mainWindow(mainWindow)
{
    // Esta classe é usada internamente — nenhuma UI própria necessária.
    // A UI de progresso é criada em doImport() via QProgressDialog inline.
}

// ─── API pública ──────────────────────────────────────────────────────────────

void CsvImportDialog::runImport(QSharedPointer<Database> db,
                                 MainWindow* mainWindow,
                                 QWidget* parent)
{
    // Descobre o banco ativo se não recebeu um
    if (!db && mainWindow) {
        auto* tabWidget = mainWindow->findChild<DatabaseTabWidget*>();
        if (tabWidget && tabWidget->currentDatabaseWidget()) {
            db = tabWidget->currentDatabaseWidget()->database();
        }
    }
    if (!db) {
        QMessageBox::warning(parent,
            QObject::tr("Importar CSV"),
            QObject::tr("Nenhum banco de dados aberto.\n"
                        "Abra ou crie um banco antes de importar."));
        return;
    }

    // Seletor de arquivo
    const QString csvPath = QFileDialog::getOpenFileName(
        parent,
        QObject::tr("Selecionar CSV para importar"),
        QDir::homePath(),
        QObject::tr("Arquivos CSV (*.csv);;Todos os arquivos (*)")
    );
    if (csvPath.isEmpty()) return;

    CsvImportDialog dlg(db, mainWindow, parent);
    dlg.doImport(csvPath);
}

// ─── Implementação do import ─────────────────────────────────────────────────

void CsvImportDialog::doImport(const QString& csvPath)
{
    QFile file(csvPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(parentWidget(),
            tr("Erro ao abrir arquivo"),
            tr("Não foi possível abrir o arquivo:\n%1").arg(csvPath));
        return;
    }

    // ── Pré-contagem de linhas (para barra de progresso real) ─────────────
    qint64 totalLines = 0;
    {
        QTextStream counter(&file);
        counter.setCodec("UTF-8");
        while (!counter.atEnd()) {
            counter.readLine();
            ++totalLines;
        }
        // Remove cabeçalho + linhas vazias da contagem
        if (totalLines > 0) --totalLines; // cabeçalho
    }
    file.seek(0);

    if (totalLines <= 0) {
        QMessageBox::information(parentWidget(),
            tr("Arquivo vazio"),
            tr("O arquivo CSV não contém entradas para importar."));
        return;
    }

    // ── Janela de progresso ───────────────────────────────────────────────
    auto* progressDlg = new QDialog(parentWidget(), Qt::Dialog | Qt::WindowTitleHint);
    progressDlg->setWindowTitle(tr("Importando CSV — CfdmNote"));
    progressDlg->setFixedWidth(460);
    progressDlg->setModal(true);

    auto* vlay = new QVBoxLayout(progressDlg);
    vlay->setSpacing(10);
    vlay->setContentsMargins(24, 20, 24, 20);

    auto* titleLabel = new QLabel(tr("<b>Importando entradas...</b>"));
    titleLabel->setAlignment(Qt::AlignCenter);
    vlay->addWidget(titleLabel);

    auto* statusLabel = new QLabel(tr("Preparando..."));
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: gray; font-size: 11px;");
    vlay->addWidget(statusLabel);

    auto* progressBar = new QProgressBar();
    progressBar->setMinimum(0);
    progressBar->setMaximum(static_cast<int>(totalLines));
    progressBar->setValue(0);
    progressBar->setTextVisible(true);
    progressBar->setFormat("%v / %m entradas  (%p%)");
    progressBar->setFixedHeight(28);
    vlay->addWidget(progressBar);

    auto* fileLabel = new QLabel(QFileInfo(csvPath).fileName());
    fileLabel->setAlignment(Qt::AlignCenter);
    fileLabel->setStyleSheet("color: gray; font-size: 10px;");
    fileLabel->setWordWrap(true);
    vlay->addWidget(fileLabel);

    progressDlg->show();
    QApplication::processEvents();

    // ── Leitura do CSV ────────────────────────────────────────────────────
    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    // Lê cabeçalho
    const QString headerLine = stream.readLine();
    const QStringList headers = parseCsvLine(headerLine);

    // Mapeia colunas (case-insensitive)
    int colTitle    = -1, colUser  = -1, colPass = -1;
    int colUrl      = -1, colNotes = -1, colGroup = -1;

    for (int i = 0; i < headers.size(); ++i) {
        const QString h = headers[i].trimmed().toLower();
        if (h == "title")    colTitle  = i;
        else if (h == "username") colUser   = i;
        else if (h == "password") colPass   = i;
        else if (h == "url")      colUrl    = i;
        else if (h == "notes")    colNotes  = i;
        else if (h == "group")    colGroup  = i;
    }

    // Fallback: ordem padrão KeePassXC (Group,Title,Username,Password,URL,Notes)
    if (colTitle < 0 && headers.size() >= 6) {
        colGroup = 0; colTitle = 1; colUser = 2;
        colPass  = 3; colUrl   = 4; colNotes = 5;
    }

    // ── Grupo destino ─────────────────────────────────────────────────────
    auto* root = m_db->rootGroup();

    // Cache de grupos por caminho completo (suporta hierarquia "Root/Pessoal/YouTube")
    // Chave = caminho normalizado (ex: "Pessoal/YouTube"), valor = ponteiro para Group
    QHash<QString, Group*> groupCache;

    // Pré-popula cache com filhos imediatos da raiz
    for (auto* g : root->children()) {
        groupCache[g->name()] = g;
    }

    // Função auxiliar: resolve ou cria grupo por caminho hierárquico
    // "Root/Pessoal/YouTube" → cria Root→Pessoal→YouTube se necessário
    // Ignora o primeiro segmento se igual ao nome da raiz (ex: "Root")
    auto resolveGroup = [&](const QString& rawPath) -> Group* {
        if (rawPath.isEmpty()) return root;

        QStringList parts = rawPath.split('/', Qt::SkipEmptyParts);

        // Remove primeiro segmento se for igual ao nome da raiz
        if (!parts.isEmpty() && parts.first() == root->name())
            parts.removeFirst();

        if (parts.isEmpty()) return root;

        Group* current = root;
        QString cacheKey;

        for (const QString& part : parts) {
            cacheKey = cacheKey.isEmpty() ? part : (cacheKey + '/' + part);

            if (!groupCache.contains(cacheKey)) {
                auto* newGroup = new Group();
                newGroup->setUuid(QUuid::createUuid());
                newGroup->setName(part);
                newGroup->setParent(current);
                groupCache[cacheKey] = newGroup;
            }
            current = groupCache[cacheKey];
        }
        return current;
    };

    // ── Importação em lotes ───────────────────────────────────────────────
    static const int BATCH = 200;
    int imported    = 0;
    int skipped     = 0;
    bool cancelled  = false;

    while (!stream.atEnd() && !cancelled) {
        const QString line = stream.readLine();
        if (line.trimmed().isEmpty()) continue;

        const QStringList fields = parseCsvLine(line);
        if (fields.isEmpty()) continue;

        auto getField = [&](int col) -> QString {
            if (col < 0 || col >= fields.size()) return {};
            return fields[col].trimmed();
        };

        const QString title = getField(colTitle);
        if (title.isEmpty()) { ++skipped; continue; }

        // Resolve grupo (suporta hierarquia "Root/Pessoal/YouTube")
        const QString groupPath = getField(colGroup);
        Group* targetGroup = resolveGroup(groupPath);

        // Cria entrada
        auto* entry = new Entry();
        entry->setUuid(QUuid::createUuid());
        entry->setTitle(title);
        entry->setUsername(getField(colUser));
        entry->setPassword(getField(colPass));
        entry->setUrl(getField(colUrl));
        entry->setNotes(getField(colNotes));
        entry->setGroup(targetGroup);

        ++imported;

        // Atualiza UI a cada lote
        if (imported % BATCH == 0) {
            progressBar->setValue(imported);
            statusLabel->setText(tr("%1 importadas, %2 restantes...")
                                 .arg(imported)
                                 .arg(totalLines - imported));
            QApplication::processEvents();
        }
    }

    file.close();

    // Atualização final
    progressBar->setValue(imported);
    statusLabel->setText(tr("Salvando banco de dados..."));
    QApplication::processEvents();

    // ── Salva o banco pelo caminho oficial (limpa flag m_modified) ────────
    QString saveError;
    const bool saved = m_db->save(Database::Atomic, QString(), &saveError);

    progressDlg->hide();
    progressDlg->deleteLater();

    // ── Resultado ─────────────────────────────────────────────────────────
    if (!saved) {
        QMessageBox::warning(parentWidget(),
            tr("Aviso"),
            tr("Entradas importadas: <b>%1</b><br>"
               "Puladas (sem título): %2<br><br>"
               "<b>Atenção:</b> Não foi possível salvar automaticamente.<br>"
               "Erro: %3<br>"
               "Use Arquivo → Salvar para preservar as mudanças.")
            .arg(imported).arg(skipped).arg(saveError));
    } else {
        QMessageBox::information(parentWidget(),
            tr("Importação concluída"),
            tr("Importação concluída com sucesso!<br><br>"
               "Entradas importadas: <b>%1</b><br>"
               "Puladas (sem título): %2")
            .arg(imported).arg(skipped));
    }

    // Força atualização visual da janela principal
    if (m_mainWindow) {
        auto* tabWidget = m_mainWindow->findChild<DatabaseTabWidget*>();
        if (tabWidget && tabWidget->currentDatabaseWidget()) {
            tabWidget->currentDatabaseWidget()->update();
        }
    }
}
