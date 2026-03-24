/*
 * CfdmNote v1.0.1 — Gerenciador de senhas com notas Markdown integradas
 */

#include <QFile>
#include <QDockWidget>
#include <QAction>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QEvent>
#include <QIcon>
#include <QPixmap>
#include <QTimer>
#include <QFileDialog>
#include <QDir>
#include <QSettings>
#include <QApplication>
#include <QScreen>
#include <QTabWidget>
#include <QBuffer>
#include <QMovie>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QPlainTextEdit>
#include <QDateTime>
#include <QFileInfo>
#include <QUrl>
#include <QDesktopServices>
#include <QStatusBar>
#include <QPushButton>
#include <QMessageBox>
#include <QAbstractButton>
#include <QToolButton>
#include <QToolBar>
#include <QToolTip>
#include <QHelpEvent>
#include <QAbstractItemView>
#include <QTreeView>
#include <QRegularExpression>
#include <QCursor>
#include <QSet>
#include <QMap>
#include <QDialog>
#include <QListWidget>

#include "core/Metadata.h"
#include "core/EntryAttributes.h"
#include "core/Group.h"
#include "gui/group/GroupModel.h"

// Motor interno do CfdmNote
#include "gui/Application.h"
#include "gui/MainWindow.h"
#include "gui/DatabaseWidget.h"
#include "gui/DatabaseTabWidget.h"
#include "core/Bootstrap.h"
#include "core/Config.h"
#include "core/Entry.h"

// Nossas adições
#include "gui/AppLockDialog.h"
#include "gui/NoteEditorWidget.h"
#include "gui/VaultDialog.h"
#include "gui/VaultUnlockDialog.h"
#include "gui/CsvImportDialog.h"
#include "gui/LinksPanelWidget.h"
#include "gui/RelatedEntriesWidget.h"
#include "gui/ReminderPanelWidget.h"
#include "gui/BackupDialog.h"
#include "gui/ShareDialog.h"
#include "gui/ExportNoteDialog.h"
#include "gui/ReminderDialog.h"
#include "gui/ManualDialog.h"
#include "gui/DatabaseInfoDialog.h"
#include "gui/MindMapDialog.h"
#include "gui/RoadmapDialog.h"
#include "gui/WhiteboardDialog.h"
#include "gui/PowerSearchDialog.h"
#include "gui/AudioCaptureDialog.h"
#include "gui/TranslateDialog.h"
#include "gui/VoiceCommandDialog.h"
#include "gui/DependencyCheckDialog.h"
#include "notes/NoteManager.h"
#include "reminders/ReminderManager.h"
#include "db/MainDatabaseManager.h"
#include "CfdmVersion.h"
#include "gui/ThemeManager.h"

static const QString APP_NAME    = "CfdmNote";
static const QString APP_VERSION = CFDMNOTE_VERSION;

/**
 * Percorre todos os widgets filhos de root procurando por objectName == name.
 */
static QWidget* findWidgetByName(QWidget* root, const QString& name)
{
    if (!root) return nullptr;
    for (QObject* child : root->children()) {
        auto* w = qobject_cast<QWidget*>(child);
        if (!w) continue;
        if (w->objectName() == name) return w;
        QWidget* found = findWidgetByName(w, name);
        if (found) return found;
    }
    return nullptr;
}

/**
 * Substitui os textos e ícone do WelcomeWidget pelas versões do CfdmNote.
 */
static void patchWelcomeWidget(QWidget* mainWindow, const QString& iconPath)
{
    // Label de boas-vindas
    auto* welcomeLabel = qobject_cast<QLabel*>(findWidgetByName(mainWindow, "welcomeLabel"));
    if (welcomeLabel) {
        welcomeLabel->setText(QString("Bem-vindo ao %1 %2").arg(APP_NAME, APP_VERSION));
    }

    // Ícone no painel de boas-vindas
    if (QFile::exists(iconPath)) {
        auto* iconLabel = qobject_cast<QLabel*>(findWidgetByName(mainWindow, "iconLabel"));
        if (iconLabel) {
            iconLabel->setPixmap(QPixmap(iconPath).scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }

    // Oculta lista de bancos recentes (gerenciada pelo CfdmNote)
    auto* recentList = findWidgetByName(mainWindow, "recentListWidget");
    if (recentList) recentList->setVisible(false);

    // Oculta labels relacionados à lista recente
    auto* recentLabel = findWidgetByName(mainWindow, "recentDatabasesLabel");
    if (recentLabel) recentLabel->setVisible(false);
}

/**
 * TitleFilter — intercepta mudanças de título na MainWindow
 * e substitui "KeePassXC" por "CfdmNote" automaticamente.
 * Cobre todos os casos: janela vazia, banco aberto, banco modificado.
 */
class TitleFilter : public QObject
{
public:
    explicit TitleFilter(QObject* parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (event->type() == QEvent::WindowTitleChange) {
            auto* w = qobject_cast<QWidget*>(obj);
            if (w) {
                QString title = w->windowTitle();
                bool changed = false;
                if (title.contains("KeePassXC")) {
                    title.replace("KeePassXC", APP_NAME);
                    changed = true;
                }
                if (!title.contains(APP_VERSION)) {
                    title = title.isEmpty()
                        ? QString("%1 v%2").arg(APP_NAME, APP_VERSION)
                        : QString("%1 — v%2").arg(title, APP_VERSION);
                    changed = true;
                }
                if (changed) {
                    w->setWindowTitle(title);
                    return true;
                }
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

/**
 * MessageBoxFixer — garante que botões de QMessageBox sempre tenham texto visível.
 * Corrige o popup "Unsaved Changes" do KeePassXC cujos botões aparecem sem label
 * em certos temas do sistema Linux.
 */
class MessageBoxFixer : public QObject
{
public:
    explicit MessageBoxFixer(QObject* parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (event->type() == QEvent::Polish) {
            auto* box = qobject_cast<QMessageBox*>(obj);
            if (!box) return false;

            for (QAbstractButton* btn : box->buttons()) {
                if (!btn->text().trimmed().isEmpty()) continue;

                // Determina o texto pelo papel do botão no diálogo
                switch (box->buttonRole(btn)) {
                    case QMessageBox::AcceptRole:
                    case QMessageBox::YesRole:
                        btn->setText(QObject::tr("Salvar"));   break;
                    case QMessageBox::DestructiveRole:
                    case QMessageBox::NoRole:
                        btn->setText(QObject::tr("Descartar")); break;
                    case QMessageBox::RejectRole:
                        btn->setText(QObject::tr("Cancelar"));  break;
                    case QMessageBox::ApplyRole:
                        btn->setText(QObject::tr("Aplicar"));   break;
                    default:
                        btn->setText(QObject::tr("OK"));        break;
                }
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

// ── Tooltip rico nas entradas (023) ─────────────────────────────────────────
class EntryTooltipFilter : public QObject {
    Q_OBJECT
protected:
    bool eventFilter(QObject* obj, QEvent* ev) override {
        if (ev->type() != QEvent::ToolTip) return false;
        auto* view = qobject_cast<QAbstractItemView*>(obj);
        if (!view) return false;
        auto* he   = static_cast<QHelpEvent*>(ev);
        const QModelIndex idx = view->indexAt(he->pos());
        if (!idx.isValid()) return false;

        const auto* model = idx.model();
        const int   row   = idx.row();
        const int   cols  = qMin(model->columnCount(), 8);

        // Coleta campos visíveis (exceto senha)
        QStringList fields;
        for (int c = 0; c < cols; ++c) {
            const QModelIndex ci  = model->index(row, c, idx.parent());
            const QString     hdr = model->headerData(c, Qt::Horizontal).toString();
            const QString     val = ci.data(Qt::DisplayRole).toString();
            if (hdr.isEmpty() || val.isEmpty() || val.startsWith(QChar(0x25CF)))
                continue;
            fields << QString("<b>%1:</b> %2")
                        .arg(hdr.toHtmlEscaped(), val.left(120).toHtmlEscaped());
        }
        if (fields.isEmpty()) return false;

        // Título em destaque (primeira coluna)
        const QString title = model->index(row, 0, idx.parent())
                                   .data(Qt::DisplayRole).toString();

        QString tip = QString(
            "<div style='font-size:12px;min-width:280px'>"
            "<b style='font-size:14px;color:#2b6cb0'>🔑 %1</b><hr style='margin:4px 0'>")
            .arg(title.toHtmlEscaped());

        // Campos normais (sem o título que já está no cabeçalho)
        QStringList bodyFields;
        for (int c = 1; c < cols; ++c) {
            const QModelIndex ci  = model->index(row, c, idx.parent());
            const QString     hdr = model->headerData(c, Qt::Horizontal).toString();
            const QString     val = ci.data(Qt::DisplayRole).toString();
            if (hdr.isEmpty() || val.isEmpty() || val.startsWith(QChar(0x25CF))) continue;
            bodyFields << QString("<b>%1:</b> <span style='color:#2d3748'>%2</span>")
                            .arg(hdr.toHtmlEscaped(), val.left(120).toHtmlEscaped());
        }
        tip += bodyFields.join("<br>");

        // Extra: tenta obter Entry* para mostrar tags, comentário, nota
        // O modelo pode expor Entry* via UserRole
        const QVariant userRole = idx.data(Qt::UserRole);
        Entry* entry = nullptr;
        if (userRole.isValid() && userRole.canConvert<Entry*>())
            entry = userRole.value<Entry*>();

        // Alternativa: busca Entry* pelo ponteiro interno do índice
        if (!entry) {
            // Em alguns modelos Qt, o internalPointer aponta para o objeto
            if (idx.internalPointer())
                entry = static_cast<Entry*>(idx.internalPointer());
        }

        if (entry) {
            const QString tags    = entry->tags();
            const QString comment = entry->attributes()->value("CfdmNote::Comment");
            const QString notes   = entry->notes();

            if (!tags.isEmpty())
                tip += QString("<br><b>🏷️ Tags:</b> <span style='color:#276749'>%1</span>")
                       .arg(tags.toHtmlEscaped());

            if (!comment.isEmpty())
                tip += QString("<br><b>💬 Comentário:</b> <i style='color:#4a5568'>%1</i>")
                       .arg(comment.left(150).toHtmlEscaped());

            if (!notes.isEmpty()) {
                const QString preview = notes.left(180).replace('\n', ' ');
                tip += QString("<br><b>📝 Nota:</b> <span style='color:#553c9a'>%1%2</span>")
                       .arg(preview.toHtmlEscaped(),
                            notes.length() > 180 ? "…" : "");
            }

            // Modificado
            const QDateTime lastMod = entry->timeInfo().lastModificationTime().toLocalTime();
            tip += QString("<br><span style='color:#a0aec0;font-size:10px'>"
                           "✏️ Modificado: %1</span>")
                   .arg(lastMod.toString("dd/MM/yyyy HH:mm"));
        }

        tip += "</div>";

        QToolTip::showText(he->globalPos(), tip, view, view->visualRect(idx), 8000);
        ev->accept();
        return true;
    }
};

// ── Tooltip rico nos grupos (hover na árvore de grupos) ─────────────────────
class GroupTooltipFilter : public QObject {
    Q_OBJECT
protected:
    bool eventFilter(QObject* obj, QEvent* ev) override {
        if (ev->type() != QEvent::ToolTip) return false;
        auto* view = qobject_cast<QTreeView*>(obj);
        if (!view) return false;
        auto* he  = static_cast<QHelpEvent*>(ev);
        const QModelIndex idx = view->indexAt(he->pos());
        if (!idx.isValid()) return false;

        // Obtém Group* via GroupModel
        auto* groupModel = qobject_cast<GroupModel*>(view->model());
        if (!groupModel) return false;
        Group* group = groupModel->groupFromIndex(idx);
        if (!group) return false;

        // Conta entradas diretas e recursivas
        const int directEntries    = group->entries().count();
        const int recursiveEntries = group->entriesRecursive().count();
        const int subGroups        = group->children().count();

        QString tip = QString(
            "<div style='font-size:12px;line-height:1.8'>"
            "<b style='font-size:13px'>📁 %1</b><br>")
            .arg(group->name().toHtmlEscaped());

        if (subGroups > 0)
            tip += QString("<b>Subgrupos:</b> %1<br>").arg(subGroups);

        tip += QString("<b>Entradas (diretas):</b> %1<br>").arg(directEntries);
        if (recursiveEntries != directEntries)
            tip += QString("<b>Entradas (total c/ subgrupos):</b> %1<br>").arg(recursiveEntries);

        if (!group->notes().isEmpty())
            tip += QString("<br><i style='color:#4a5568'>%1</i>")
                   .arg(group->notes().left(200).toHtmlEscaped());

        const QDateTime lastMod = group->timeInfo().lastModificationTime().toLocalTime();
        tip += QString("<br><span style='color:#a0aec0;font-size:10px'>"
                       "Modificado: %1</span>")
               .arg(lastMod.toString("dd/MM/yyyy HH:mm"));

        tip += "</div>";

        QToolTip::showText(he->globalPos(), tip, view, view->visualRect(idx), 6000);
        ev->accept();
        return true;
    }
};

/**
 * Cria a aba "Notas" dentro do preview de entradas do DatabaseWidget.
 * Retorna o NoteEditorWidget criado (null se já existia ou falhou).
 */
static NoteEditorWidget* setupNotesTab(DatabaseWidget* dbWidget)
{
    if (!dbWidget) return nullptr;
    if (dbWidget->findChild<NoteEditorWidget*>()) return nullptr; // já configurado

    auto* previewWidget = dbWidget->findChild<QWidget*>("previewWidget");
    if (!previewWidget) return nullptr;

    auto* splitter = qobject_cast<QSplitter*>(previewWidget->parentWidget());
    if (!splitter) return nullptr;

    const int idx = splitter->indexOf(previewWidget);
    if (idx < 0) return nullptr;

    // Cria QTabWidget com abas no topo
    auto* tabWidget = new QTabWidget();
    tabWidget->setObjectName("notesTabWidget");
    tabWidget->setTabPosition(QTabWidget::North);
    tabWidget->setDocumentMode(false);

    // Substitui previewWidget no splitter; retorna o widget desconectado
    auto* detachedPreview = splitter->replaceWidget(idx, tabWidget);
    tabWidget->addTab(detachedPreview,
                      QIcon::fromTheme("dialog-information"), QObject::tr("Detalhes"));

    // ── Aba "Notas": preview de GIF + editor Markdown ─────────────────────
    auto* notesContainer = new QWidget();
    auto* notesLayout = new QVBoxLayout(notesContainer);
    notesLayout->setContentsMargins(0, 0, 0, 0);
    notesLayout->setSpacing(0);

    // Preview de GIF animado (oculto por padrão)
    auto* gifLabel = new QLabel();
    gifLabel->setObjectName("gifIconLabel");
    gifLabel->setAlignment(Qt::AlignCenter);
    gifLabel->setMaximumHeight(140);
    gifLabel->setStyleSheet(
        "QLabel { background: palette(base); border-bottom: 1px solid palette(mid); padding: 4px; }");
    gifLabel->setVisible(false);
    notesLayout->addWidget(gifLabel);

    // ── Campo "Comentário" (observação rápida por entrada) ─────────────────
    auto* commentFrame = new QFrame();
    commentFrame->setStyleSheet("QFrame { border: 0; background: transparent; }");
    auto* commentRowLayout = new QHBoxLayout(commentFrame);
    commentRowLayout->setContentsMargins(4, 4, 4, 0);
    commentRowLayout->setSpacing(6);

    auto* commentLabel = new QLabel(QObject::tr("Comentário:"));
    commentLabel->setStyleSheet("font-size: 11px; font-weight: bold;");
    commentLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    commentLabel->setFixedWidth(82);
    commentRowLayout->addWidget(commentLabel);

    auto* commentEdit = new QPlainTextEdit();
    commentEdit->setObjectName("entryCommentEdit");
    commentEdit->setMaximumHeight(60);
    commentEdit->setPlaceholderText(QObject::tr("Observação rápida sobre esta entrada..."));
    commentEdit->setStyleSheet("font-size: 11px; border: 1px solid palette(mid); "
                               "border-radius: 3px; padding: 2px;");
    commentEdit->setEnabled(false);
    commentRowLayout->addWidget(commentEdit, 1);
    notesLayout->addWidget(commentFrame);

    auto* noteEditor = new NoteEditorWidget();
    notesLayout->addWidget(noteEditor, 1);

    tabWidget->addTab(notesContainer,
                      QIcon::fromTheme("text-editor"), QObject::tr("Notas"));

    // Aba Links (Local Disk + External/Removable)
    auto* linksPanel = new LinksPanelWidget();
    linksPanel->setObjectName("linksPanelWidget");
    tabWidget->addTab(linksPanel,
                      QIcon::fromTheme("folder-remote"), QObject::tr("Links"));

    // Aba Relacionados (mesmo domínio + referências manuais cruzadas)
    auto* relatedWidget = new RelatedEntriesWidget();
    relatedWidget->setObjectName("relatedEntriesWidget");
    tabWidget->addTab(relatedWidget,
                      QIcon::fromTheme("emblem-symbolic-link"), QObject::tr("🔗 Relacionados"));

    // Aba Lembrete (status + edição rápida)
    auto* reminderPanel = new ReminderPanelWidget();
    reminderPanel->setObjectName("reminderPanelWidget");
    tabWidget->addTab(reminderPanel,
                      QIcon::fromTheme("appointment-new"), QObject::tr("⏰ Lembrete"));

    // ── Garante que o painel de abas nunca some ────────────────────────────
    tabWidget->setMinimumHeight(160);
    splitter->setCollapsible(idx, false);   // impede colapso total

    // ── Melhora visibilidade do handle do splitter ──────────────────────────
    splitter->setHandleWidth(8);
    splitter->setStyleSheet(
        "QSplitter::handle:vertical {"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "    stop:0 #bee3f8, stop:0.4 #ebf8ff, stop:0.6 #ebf8ff, stop:1 #bee3f8);"
        "  border-top: 1px solid #90cdf4;"
        "  border-bottom: 1px solid #90cdf4;"
        "  margin: 0 4px;"
        "}"
        "QSplitter::handle:vertical:hover {"
        "  background: #90cdf4;"
        "  cursor: ns-resize;"
        "}"
        "QSplitter::handle:vertical:pressed {"
        "  background: #3182ce;"
        "}"
    );

    // Define tamanho inicial: 55% lista de entradas / 45% painel de abas
    QTimer::singleShot(0, splitter, [splitter, idx]() {
        const int total = splitter->height();
        if (total < 200) return;
        QList<int> sizes = splitter->sizes();
        // Só ajusta se o painel estiver muito pequeno (< 160px)
        if (sizes.value(idx, 0) < 160) {
            sizes[0]   = static_cast<int>(total * 0.55);
            if (idx < sizes.size())
                sizes[idx] = total - sizes[0];
            splitter->setSizes(sizes);
        }
    });

    return noteEditor;
}

/**
 * Atualiza o preview de GIF animado na aba Notas.
 * Mostra animação se a entrada tem um ícone personalizado GIF.
 */
static void updateGifPreview(QLabel* gifLabel, Entry* entry, const QSharedPointer<Database>& db)
{
    if (!gifLabel) return;

    // Remove movie anterior
    if (gifLabel->movie()) {
        gifLabel->movie()->stop();
        gifLabel->movie()->deleteLater();
        gifLabel->setMovie(nullptr);
        // Remove buffers filhos
        for (auto* child : gifLabel->children())
            if (qobject_cast<QBuffer*>(child)) child->deleteLater();
    }
    gifLabel->clear();
    gifLabel->setVisible(false);

    if (!entry || entry->iconUuid().isNull() || !db) return;

    const auto& iconData = db->metadata()->customIcon(entry->iconUuid());
    if (!iconData.data.startsWith("GIF8")) return;

    // Cria QBuffer com cópia interna dos bytes GIF
    auto* buf = new QBuffer(gifLabel);
    buf->setData(iconData.data);
    buf->open(QIODevice::ReadOnly);

    auto* movie = new QMovie(gifLabel);
    movie->setDevice(buf);

    if (!movie->isValid()) { movie->deleteLater(); return; }

    gifLabel->setMovie(movie);
    movie->start();
    gifLabel->setVisible(true);
}

/**
 * Conecta entrySelectionChanged e configura a aba Notas para um DatabaseWidget.
 */
static void connectDbWidget(DatabaseWidget* dbWidget)
{
    if (!dbWidget) return;

    QTimer::singleShot(400, dbWidget, [dbWidget]() {
        // ── Instala tooltip rico em todas as QTreeView do DatabaseWidget ──────
        for (auto* view : dbWidget->findChildren<QTreeView*>()) {
            // Evita instalar duas vezes no mesmo widget
            bool already = false;
            for (auto* ef : view->children())
                if (qobject_cast<EntryTooltipFilter*>(ef)) { already = true; break; }
            if (!already)
                view->installEventFilter(new EntryTooltipFilter());
        }

        auto* noteEditor = setupNotesTab(dbWidget);
        if (!noteEditor) {
            noteEditor = dbWidget->findChild<NoteEditorWidget*>();
            if (!noteEditor) return;
        }

        // Registra banco no ReminderManager para verificar lembretes
        if (dbWidget->database()) {
            ReminderManager::instance().setDatabase(
                dbWidget->database(),
                dbWidget->window());
        }

        // Debounce timer para salvar comentário após 600ms sem digitar
        auto* commentEdit = dbWidget->findChild<QPlainTextEdit*>("entryCommentEdit");
        if (commentEdit) {
            auto* commentTimer = new QTimer(commentEdit);
            commentTimer->setSingleShot(true);
            commentTimer->setInterval(600);
            QObject::connect(commentEdit, &QPlainTextEdit::textChanged, commentTimer,
                [commentTimer]() { commentTimer->start(); }, Qt::UniqueConnection);
            QObject::connect(commentTimer, &QTimer::timeout, commentEdit,
                [dbWidget]() {
                    auto* ce = dbWidget->findChild<QPlainTextEdit*>("entryCommentEdit");
                    if (!ce) return;
                    auto* currentEntry = dbWidget->currentSelectedEntry();
                    if (!currentEntry || !dbWidget->database()) return;
                    currentEntry->attributes()->set(
                        "CfdmNote::Comment", ce->toPlainText(), false);
                    QString err;
                    dbWidget->database()->save(Database::Atomic, {}, &err);
                }, Qt::UniqueConnection);
        }

        QObject::connect(dbWidget, &DatabaseWidget::entrySelectionChanged,
                         noteEditor, [dbWidget, noteEditor]() {
            auto* gifLabel      = dbWidget->findChild<QLabel*>("gifIconLabel");
            auto* linksPanel    = dbWidget->findChild<LinksPanelWidget*>();
            auto* commentEdit   = dbWidget->findChild<QPlainTextEdit*>("entryCommentEdit");
            auto* relatedWidget  = dbWidget->findChild<RelatedEntriesWidget*>();
            auto* reminderPanel  = dbWidget->findChild<ReminderPanelWidget*>();
            Entry* entry = dbWidget->currentSelectedEntry();
            if (entry) {
                noteEditor->loadNoteForEntry(entry->uuidToHex());
                updateGifPreview(gifLabel, entry, dbWidget->database());
                if (linksPanel)    linksPanel->loadEntry(entry, dbWidget->database());
                if (relatedWidget) relatedWidget->setEntry(entry, dbWidget->database());
                if (reminderPanel) reminderPanel->setEntry(entry, dbWidget->database());
                if (commentEdit) {
                    commentEdit->blockSignals(true);
                    commentEdit->setPlainText(entry->attributes()->value("CfdmNote::Comment"));
                    commentEdit->blockSignals(false);
                    commentEdit->setEnabled(true);
                }
            } else {
                noteEditor->clearNote();
                updateGifPreview(gifLabel, nullptr, {});
                if (linksPanel)    linksPanel->clearLinks();
                if (relatedWidget) relatedWidget->clear();
                if (reminderPanel) reminderPanel->clear();
                if (commentEdit) {
                    commentEdit->blockSignals(true);
                    commentEdit->clear();
                    commentEdit->blockSignals(false);
                    commentEdit->setEnabled(false);
                }
            }
        }, Qt::UniqueConnection);

        // Navegação a partir do RelatedEntriesWidget
        auto* relatedWidget = dbWidget->findChild<RelatedEntriesWidget*>();
        if (relatedWidget) {
            QObject::connect(relatedWidget, &RelatedEntriesWidget::navigateRequested,
                             dbWidget, [dbWidget](const QString& uuidHex) {
                if (!dbWidget->database()) return;
                Entry* target = dbWidget->database()->rootGroup()
                    ->findEntryByUuid(QUuid::fromString(uuidHex));
                if (!target) return;
                // Tenta encontrar a search bar e filtrar pelo título
                const auto lineEdits = dbWidget->findChildren<QLineEdit*>();
                for (auto* le : lineEdits) {
                    const QString ph = le->placeholderText().toLower();
                    if (ph.contains("search") || ph.contains("busca") ||
                        ph.contains("pesquisa") || ph.contains("procurar")) {
                        le->setText(target->title());
                        le->setFocus();
                        break;
                    }
                }
            }, Qt::UniqueConnection);
        }
    });
}

static void connectEntryToNotes(MainWindow* mw)
{
    auto* tabWidget = mw->findChild<DatabaseTabWidget*>();
    if (!tabWidget) return;

    QObject::connect(tabWidget, &DatabaseTabWidget::activeDatabaseChanged,
                     mw, [](DatabaseWidget* dbWidget) {
        connectDbWidget(dbWidget);
    });

    connectDbWidget(tabWidget->currentDatabaseWidget());
}

int main(int argc, char* argv[])
{
    // Qt5WebEngine: desabilita processo GPU para evitar eventos de foco no startup
    // (o processo GPU ao conectar de volta dispara ApplicationActivate que abre bancos)
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--disable-gpu --disable-software-rasterizer");
    // Necessário para Qt5WebEngine — DEVE vir antes de QApplication
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    Application app(argc, argv);

    // Se já existe uma instância rodando, apenas traz ela para frente e sai
    if (app.isAlreadyRunning()) {
        return 0;
    }

    // Corrige botões sem texto em QMessageBox (tema do sistema)
    app.installEventFilter(new MessageBoxFixer(&app));

    // Bootstrap do motor (crypto, config, recursos, traduções, etc.)
    Bootstrap::bootstrap();
    app.setApplicationName(APP_NAME);
    app.setApplicationDisplayName(APP_NAME);
    app.setApplicationVersion(APP_VERSION);

    // ── Tela de login do CfdmNote (antes de qualquer janela) ─────────────────
    // Processa eventos pendentes do Qt5WebEngine antes de mostrar o dialog.
    // IMPORTANTE: a limpeza do config vem DEPOIS do processEvents() para evitar
    // que o inotify do QSettings releia o disco e sobrescreva nossos valores.
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    // Bloqueia TODOS os mecanismos de auto-open do KeePassXC.
    config()->set(Config::OpenPreviousDatabasesOnStartup, false);
    config()->remove(Config::LastOpenedDatabases);
    config()->remove(Config::LastActiveDatabase);

    {
        AppLockDialog lockDlg;
        lockDlg.setWindowFlag(Qt::WindowStaysOnTopHint, true);
        if (lockDlg.exec() != QDialog::Accepted)
            return 0;
    }

    // ── Ícone do CfdmNote (substitui o ícone do KeePassXC em toda a app) ──────
    const QString iconPath = QString("%1/../resources/icons/cfdmnote_128.png")
                             .arg(app.applicationDirPath());
    const QIcon cfdmIcon = QFile::exists(iconPath)
                           ? QIcon(iconPath)
                           : QIcon::fromTheme("dialog-password");
    app.setWindowIcon(cfdmIcon);

    // ── Janela principal do CfdmNote ──────────────────────────────────────────
    MainWindow mainWindow;
    mainWindow.setWindowIcon(cfdmIcon);

    // Notas integradas como aba no preview de entrada (não mais como dock)

    // ── Overlay de carregamento (aparece em operações longas) ─────────────────
    auto* busyOverlay = new QLabel(&mainWindow);
    busyOverlay->setText(QObject::tr(
        "⏳  Aguarde — sistema ocupado...\n"
        "Importando dados. O app pode parecer travado.\n"
        "Não feche a janela."));
    busyOverlay->setAlignment(Qt::AlignCenter);
    busyOverlay->setStyleSheet(
        "QLabel { background: rgba(0,0,0,180); color: white; font-size: 15px; "
        "font-weight: bold; border-radius: 10px; padding: 24px; }");
    busyOverlay->setVisible(false);
    busyOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    busyOverlay->setGeometry(mainWindow.rect());

    // Redimensiona overlay junto com a janela
    QObject::connect(&mainWindow, &QMainWindow::destroyed, busyOverlay, &QLabel::deleteLater);

    // Instala filtro de título (substitui "KeePassXC" → "CfdmNote")
    auto* titleFilter = new TitleFilter(&mainWindow);
    mainWindow.installEventFilter(titleFilter);

    // Força título, patches visuais e conexão de entradas após inicializar
    QTimer::singleShot(0, &mainWindow, [&mainWindow, &iconPath, busyOverlay]() {
        // Título com versão
        QString t = mainWindow.windowTitle();
        t.replace("KeePassXC", APP_NAME);
        if (!t.contains(APP_VERSION))
            t = t.isEmpty() ? QString("%1 v%2").arg(APP_NAME, APP_VERSION)
                            : QString("%1 — v%2").arg(t, APP_VERSION);
        mainWindow.setWindowTitle(t);

        // WelcomeWidget: texto de boas-vindas + ícone
        patchWelcomeWidget(&mainWindow, iconPath);

        // Conecta seleção de entrada → aba Notas (criada por setupNotesTab)
        connectEntryToNotes(&mainWindow);

        // Fallback: garante que as abas são criadas mesmo se o sinal falhar
        QTimer::singleShot(800, &mainWindow, [&mainWindow]() {
            auto* dbTabWidget = mainWindow.findChild<DatabaseTabWidget*>();
            if (!dbTabWidget) return;
            for (auto* dbWidget : dbTabWidget->findChildren<DatabaseWidget*>()) {
                if (!dbWidget->findChild<NoteEditorWidget*>())
                    connectDbWidget(dbWidget);
            }
        });

        // Failsafe: garante que o overlay some após o startup (caso tenha ficado visível)
        QTimer::singleShot(3000, busyOverlay, [busyOverlay]() {
            busyOverlay->setVisible(false);
        });
    });

    // ── Menu CfdmNote ─────────────────────────────────────────────────────────
    QMenu* cfdmMenu = new QMenu(QObject::tr("&CfdmNote"), &mainWindow);
    cfdmMenu->setToolTipsVisible(true);  // mostra tooltip ao passar mouse sobre cada item

    // Abrir aba Notas da entrada selecionada (Ctrl+F9)
    QAction* actNotes = cfdmMenu->addAction(
        QIcon::fromTheme("text-editor"),
        QObject::tr("&Notas da entrada"));
    actNotes->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F9));
    QObject::connect(actNotes, &QAction::triggered, [&mainWindow]() {
        auto* dbTabWidget = mainWindow.findChild<DatabaseTabWidget*>();
        if (!dbTabWidget || !dbTabWidget->currentDatabaseWidget()) return;
        auto* ntw = dbTabWidget->currentDatabaseWidget()
                        ->findChild<QTabWidget*>("notesTabWidget");
        if (ntw) ntw->setCurrentIndex(ntw->currentIndex() == 1 ? 0 : 1);
    });

    // Pesquisar na web com IA (001) — submenu de mecanismos de busca
    QMenu* searchMenu = cfdmMenu->addMenu(
        QIcon::fromTheme("edit-find"), QObject::tr("Pesquisar na &web..."));

    struct SearchEngine { QString name; QString urlTemplate; };
    const QList<SearchEngine> searchEngines = {
        { QObject::tr("Google"),       "https://www.google.com/search?q=%1" },
        { QObject::tr("DuckDuckGo"),   "https://duckduckgo.com/?q=%1" },
        { QObject::tr("Perplexity AI"),"https://www.perplexity.ai/search?q=%1" },
        { QObject::tr("ChatGPT"),      "https://chatgpt.com/?q=%1" },
    };
    for (const auto& eng : searchEngines) {
        const QString urlTpl = eng.urlTemplate;
        QObject::connect(
            searchMenu->addAction(eng.name),
            &QAction::triggered,
            [&mainWindow, urlTpl]() {
                auto* dbTabWidget = mainWindow.findChild<DatabaseTabWidget*>();
                if (!dbTabWidget || !dbTabWidget->currentDatabaseWidget()) return;
                Entry* entry = dbTabWidget->currentDatabaseWidget()->currentSelectedEntry();
                if (!entry) {
                    QMessageBox::information(&mainWindow, QObject::tr("Pesquisar na web"),
                        QObject::tr("Selecione uma entrada primeiro."));
                    return;
                }
                const QString query = QUrl::toPercentEncoding(entry->title());
                QDesktopServices::openUrl(QUrl(urlTpl.arg(query)));
            });
    }

    cfdmMenu->addSeparator();

    // Abrir cofre criptografado (.kdbx)
    QAction* actOpenVault = cfdmMenu->addAction(
        QIcon::fromTheme("document-open"),
        QObject::tr("&Abrir Cofre...")
    );
    actOpenVault->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));
    QObject::connect(actOpenVault, &QAction::triggered, [&mainWindow, &app]() {
        // Usa o último cofre se existir, senão abre o seletor de arquivo
        QSettings s(AppLockDialog::configPath(), QSettings::IniFormat);
        QString path = s.value("vault/lastPath").toString();

        if (path.isEmpty() || !QFile::exists(path)) {
            path = QFileDialog::getOpenFileName(
                &mainWindow,
                QObject::tr("Abrir Cofre CfdmNote"),
                QDir::homePath(),
                QObject::tr("Cofre CfdmNote (*.kdbx)")
            );
        }
        if (!path.isEmpty()) {
            VaultUnlockDialog dlg(path, &mainWindow, &mainWindow);
            dlg.exec();
            // Cofre aberto → tema verde; fechado → volta ao azul
            ThemeManager::apply(ThemeManager::Theme::VaultGreen, &app);
        }
    });

    // Abrir cofre diferente (sempre mostra seletor de arquivo)
    QAction* actOpenOtherVault = cfdmMenu->addAction(
        QIcon::fromTheme("document-open"),
        QObject::tr("Abrir &outro cofre...")
    );
    QObject::connect(actOpenOtherVault, &QAction::triggered, [&mainWindow, &app]() {
        const QString path = QFileDialog::getOpenFileName(
            &mainWindow,
            QObject::tr("Abrir Cofre CfdmNote"),
            QDir::homePath(),
            QObject::tr("Cofre CfdmNote (*.kdbx)")
        );
        if (!path.isEmpty()) {
            VaultUnlockDialog dlg(path, &mainWindow, &mainWindow);
            dlg.exec();
            ThemeManager::apply(ThemeManager::Theme::VaultGreen, &app);
        }
    });

    // Criar cofre criptografado
    QAction* actVault = cfdmMenu->addAction(
        QIcon::fromTheme("security-high"),
        QObject::tr("Criar &Cofre Criptografado...")
    );
    QObject::connect(actVault, &QAction::triggered, [&mainWindow]() {
        VaultDialog dlg(&mainWindow);
        dlg.exec();
    });

    cfdmMenu->addSeparator();

    // Backup externo para mídia removível
    QAction* actBackup = cfdmMenu->addAction(
        QIcon::fromTheme("document-save-as"),
        QObject::tr("&Backup Externo...")
    );
    actBackup->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_B));
    QObject::connect(actBackup, &QAction::triggered, [&mainWindow]() {
        BackupDialog::runBackup(&mainWindow, &mainWindow);
    });

    // Criar Grupo com seletor de pai
    QAction* actCreateGroup = cfdmMenu->addAction(
        QIcon::fromTheme("folder-new"),
        QObject::tr("📁 Criar &Grupo...")
    );
    actCreateGroup->setToolTip(QObject::tr(
        "<b>📁 Criar Grupo</b><br><br>"
        "Cria um novo grupo escolhendo onde ele ficará:<br>"
        "na raiz ou dentro de qualquer grupo existente."));
    actCreateGroup->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G));
    actCreateGroup->setStatusTip(QObject::tr("Criar grupo com escolha do grupo pai (Ctrl+Shift+G)"));
    QObject::connect(actCreateGroup, &QAction::triggered, [&mainWindow]() {
        auto* dbTab = mainWindow.findChild<DatabaseTabWidget*>();
        if (!dbTab) return;
        auto* dbWidget = dbTab->currentDatabaseWidget();
        if (!dbWidget) return;
        auto db = dbWidget->database();
        if (!db) return;

        // Dialog: nome + seletor de grupo pai
        QDialog dlg(&mainWindow);
        dlg.setWindowTitle(QObject::tr("Criar Grupo"));
        dlg.setFixedWidth(380);
        auto* lay = new QVBoxLayout(&dlg);
        lay->setSpacing(12);
        lay->setContentsMargins(20, 20, 20, 20);

        auto* lblNome = new QLabel(QObject::tr("Nome do grupo:"));
        lblNome->setStyleSheet("font-weight: bold;");
        auto* editNome = new QLineEdit();
        editNome->setPlaceholderText(QObject::tr("Ex: Trabalho, Pessoal, Projetos..."));
        editNome->setFixedHeight(34);

        auto* lblPai = new QLabel(QObject::tr("Criar dentro de:"));
        lblPai->setStyleSheet("font-weight: bold;");
        auto* cmbPai = new QComboBox();
        cmbPai->setFixedHeight(34);

        // Popula o combo com todos os grupos, indentado por nível
        std::function<void(Group*, int)> addGroups = [&](Group* g, int depth) {
            const QString indent = QString("  ").repeated(depth);
            const QString icon = depth == 0 ? "🏠 " : "📁 ";
            cmbPai->addItem(indent + icon + g->name(),
                            QVariant::fromValue(static_cast<void*>(g)));
            for (auto* child : g->children())
                addGroups(child, depth + 1);
        };
        addGroups(db->rootGroup(), 0);

        // Pré-seleciona o grupo atualmente ativo
        auto* current = dbWidget->currentGroup();
        if (current) {
            for (int i = 0; i < cmbPai->count(); ++i) {
                if (cmbPai->itemData(i).value<void*>() == static_cast<void*>(current)) {
                    cmbPai->setCurrentIndex(i);
                    break;
                }
            }
        }

        auto* errLbl = new QLabel();
        errLbl->setStyleSheet("color: #e53e3e; font-size: 11px;");

        auto* btnRow = new QHBoxLayout();
        auto* btnOk = new QPushButton(QObject::tr("✅ Criar Grupo"));
        btnOk->setDefault(true);
        btnOk->setFixedHeight(36);
        btnOk->setStyleSheet("QPushButton { background: #3182ce; color: white; border-radius: 5px; "
                             "font-weight: bold; padding: 0 16px; } "
                             "QPushButton:hover { background: #4299e1; }");
        auto* btnCancel = new QPushButton(QObject::tr("Cancelar"));
        btnCancel->setFixedHeight(36);
        btnRow->addStretch();
        btnRow->addWidget(btnCancel);
        btnRow->addWidget(btnOk);

        lay->addWidget(lblNome);
        lay->addWidget(editNome);
        lay->addWidget(lblPai);
        lay->addWidget(cmbPai);
        lay->addWidget(errLbl);
        lay->addLayout(btnRow);

        QObject::connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);
        QObject::connect(btnOk, &QPushButton::clicked, &dlg, [&]() {
            const QString name = editNome->text().trimmed();
            if (name.isEmpty()) {
                errLbl->setText(QObject::tr("⚠️ Digite um nome para o grupo."));
                editNome->setFocus();
                return;
            }
            auto* parentGroup = static_cast<Group*>(
                cmbPai->currentData().value<void*>());
            if (!parentGroup) parentGroup = db->rootGroup();

            auto* newGroup = new Group();
            newGroup->setUuid(QUuid::createUuid());
            newGroup->setName(name);
            newGroup->setParent(parentGroup);

            QString err;
            db->save(Database::Atomic, {}, &err);
            dlg.accept();

            QMessageBox::information(&mainWindow,
                QObject::tr("Grupo criado"),
                QObject::tr("Grupo <b>%1</b> criado em <b>%2</b>.")
                    .arg(name.toHtmlEscaped(),
                         parentGroup->name().toHtmlEscaped()));
        });

        editNome->setFocus();
        dlg.exec();
    });

    // Mover Grupo para outro pai
    QAction* actMoveGroup = cfdmMenu->addAction(
        QIcon::fromTheme("edit-cut"),
        QObject::tr("✂️ Mover &Grupo...")
    );
    actMoveGroup->setToolTip(QObject::tr(
        "<b>✂️ Mover Grupo</b><br><br>"
        "Move um grupo existente para dentro de outro grupo.<br>"
        "Selecione o grupo a mover e o destino."));
    QObject::connect(actMoveGroup, &QAction::triggered, [&mainWindow]() {
        auto* dbTab = mainWindow.findChild<DatabaseTabWidget*>();
        if (!dbTab) return;
        auto* dbWidget = dbTab->currentDatabaseWidget();
        if (!dbWidget) return;
        auto db = dbWidget->database();
        if (!db) return;

        QDialog dlg(&mainWindow);
        dlg.setWindowTitle(QObject::tr("Mover Grupo"));
        dlg.setFixedWidth(400);
        auto* lay = new QVBoxLayout(&dlg);
        lay->setSpacing(12);
        lay->setContentsMargins(20, 20, 20, 20);

        auto* lblMover = new QLabel(QObject::tr("Grupo a mover:"));
        lblMover->setStyleSheet("font-weight: bold;");
        auto* cmbMover = new QComboBox();
        cmbMover->setFixedHeight(34);

        auto* lblDest = new QLabel(QObject::tr("Mover para dentro de:"));
        lblDest->setStyleSheet("font-weight: bold;");
        auto* cmbDest = new QComboBox();
        cmbDest->setFixedHeight(34);

        std::function<void(Group*, int)> addGroups2 = [&](Group* g, int depth) {
            if (g == db->metadata()->recycleBin()) return;
            const QString indent = QString("  ").repeated(depth);
            const QString icon = depth == 0 ? "🏠 " : "📁 ";
            const QString label = indent + icon + g->name();
            const QVariant ptr = QVariant::fromValue(static_cast<void*>(g));
            if (depth > 0) cmbMover->addItem(label, ptr);
            cmbDest->addItem(label, ptr);
            for (auto* child : g->children()) addGroups2(child, depth + 1);
        };
        addGroups2(db->rootGroup(), 0);

        // Pré-seleciona grupo atual na lista "a mover"
        auto* current = dbWidget->currentGroup();
        if (current && current != db->rootGroup()) {
            for (int i = 0; i < cmbMover->count(); ++i) {
                if (cmbMover->itemData(i).value<void*>() == static_cast<void*>(current)) {
                    cmbMover->setCurrentIndex(i);
                    break;
                }
            }
        }

        auto* errLbl = new QLabel();
        errLbl->setStyleSheet("color: #e53e3e; font-size: 11px;");

        auto* btnRow = new QHBoxLayout();
        auto* btnOk = new QPushButton(QObject::tr("✅ Mover"));
        btnOk->setDefault(true);
        btnOk->setFixedHeight(36);
        btnOk->setStyleSheet("QPushButton { background: #3182ce; color: white; border-radius: 5px; "
                             "font-weight: bold; padding: 0 16px; } "
                             "QPushButton:hover { background: #4299e1; }");
        auto* btnCancel = new QPushButton(QObject::tr("Cancelar"));
        btnCancel->setFixedHeight(36);
        btnRow->addStretch();
        btnRow->addWidget(btnCancel);
        btnRow->addWidget(btnOk);

        lay->addWidget(lblMover);
        lay->addWidget(cmbMover);
        lay->addWidget(lblDest);
        lay->addWidget(cmbDest);
        lay->addWidget(errLbl);
        lay->addLayout(btnRow);

        QObject::connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);
        QObject::connect(btnOk, &QPushButton::clicked, &dlg, [&]() {
            auto* grpMover = static_cast<Group*>(cmbMover->currentData().value<void*>());
            auto* grpDest  = static_cast<Group*>(cmbDest->currentData().value<void*>());
            if (!grpMover || !grpDest) {
                errLbl->setText(QObject::tr("⚠️ Selecione o grupo a mover e o destino."));
                return;
            }
            if (grpMover == grpDest) {
                errLbl->setText(QObject::tr("⚠️ O grupo de origem e destino são iguais."));
                return;
            }
            // Verifica se destino não é descendente de origem (evita ciclo)
            Group* check = grpDest;
            while (check) {
                if (check == grpMover) {
                    errLbl->setText(QObject::tr("⚠️ Não é possível mover um grupo para dentro de si mesmo."));
                    return;
                }
                check = check->parentGroup();
            }
            const QString nomeMover = grpMover->name();
            const QString nomeDest  = grpDest->name();
            grpMover->setParent(grpDest);
            QString err;
            db->save(Database::Atomic, {}, &err);
            dlg.accept();
            QMessageBox::information(&mainWindow,
                QObject::tr("Grupo movido"),
                QObject::tr("Grupo <b>%1</b> movido para <b>%2</b>.")
                    .arg(nomeMover.toHtmlEscaped(), nomeDest.toHtmlEscaped()));
        });

        dlg.exec();
    });

    // Importar CSV (com barra de progresso real)
    QAction* actImportCsv = cfdmMenu->addAction(
        QIcon::fromTheme("document-import"),
        QObject::tr("&Importar CSV...")
    );
    QObject::connect(actImportCsv, &QAction::triggered, [&mainWindow]() {
        auto* tabWidget = mainWindow.findChild<DatabaseTabWidget*>();
        QSharedPointer<Database> db;
        if (tabWidget && tabWidget->currentDatabaseWidget()) {
            db = tabWidget->currentDatabaseWidget()->database();
        }
        CsvImportDialog::runImport(db, &mainWindow, &mainWindow);
    });

    // Importar TXT — cria entradas a partir de arquivos de texto
    QAction* actImportTxt = cfdmMenu->addAction(
        QIcon::fromTheme("text-plain"),
        QObject::tr("Importar &TXT...")
    );
    QObject::connect(actImportTxt, &QAction::triggered, [&mainWindow, busyOverlay]() {
        auto* dbTabWidget = mainWindow.findChild<DatabaseTabWidget*>();
        if (!dbTabWidget || !dbTabWidget->currentDatabaseWidget()) {
            QMessageBox::warning(&mainWindow, QObject::tr("Importar TXT"),
                QObject::tr("Abra um banco de dados primeiro."));
            return;
        }
        auto db = dbTabWidget->currentDatabaseWidget()->database();
        if (!db) return;

        const QStringList files = QFileDialog::getOpenFileNames(
            &mainWindow, QObject::tr("Selecionar arquivos TXT"),
            QDir::homePath(),
            QObject::tr("Arquivos de texto (*.txt);;Todos os arquivos (*)")
        );
        if (files.isEmpty()) return;

        busyOverlay->setGeometry(mainWindow.rect());
        busyOverlay->raise();
        busyOverlay->setVisible(true);
        QApplication::processEvents();

        auto* root = db->rootGroup();
        int count = 0;
        for (const QString& filePath : files) {
            QFile f(filePath);
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
            const QString content = QString::fromUtf8(f.readAll());
            f.close();

            auto* entry = new Entry();
            entry->setTitle(QFileInfo(filePath).completeBaseName());
            entry->setNotes(content);
            entry->setGroup(root);
            ++count;
            QApplication::processEvents();
        }

        busyOverlay->setVisible(false);

        QString saveErr;
        db->save(Database::Atomic, {}, &saveErr);

        QMessageBox::information(&mainWindow, QObject::tr("Importar TXT"),
            saveErr.isEmpty()
                ? QObject::tr("%1 arquivo(s) importado(s) com sucesso.").arg(count)
                : QObject::tr("%1 arquivo(s) importado(s). Aviso: %2").arg(count).arg(saveErr));
    });

    // Definir lembrete na entrada (004)
    QAction* actReminder = cfdmMenu->addAction(
        QIcon::fromTheme("appointment-new"),
        QObject::tr("Definir &lembrete...")
    );
    actReminder->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
    QObject::connect(actReminder, &QAction::triggered, [&mainWindow]() {
        auto* dbTabWidget = mainWindow.findChild<DatabaseTabWidget*>();
        if (!dbTabWidget || !dbTabWidget->currentDatabaseWidget()) return;
        auto* dbWidget = dbTabWidget->currentDatabaseWidget();
        Entry* entry = dbWidget->currentSelectedEntry();
        if (!entry) {
            QMessageBox::information(&mainWindow, QObject::tr("Lembrete"),
                QObject::tr("Selecione uma entrada primeiro."));
            return;
        }
        ReminderDialog::editReminder(entry, dbWidget->database(), &mainWindow);
    });

    // Tradução de notas (026)
    QAction* actTranslate = cfdmMenu->addAction(
        QIcon::fromTheme("preferences-desktop-locale"),
        QObject::tr("&Traduzir nota...")
    );
    actTranslate->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T));
    QObject::connect(actTranslate, &QAction::triggered, [&mainWindow]() {
        auto* dbTabWidget = mainWindow.findChild<DatabaseTabWidget*>();
        Entry* entry = nullptr;
        QSharedPointer<Database> db;
        if (dbTabWidget && dbTabWidget->currentDatabaseWidget()) {
            entry = dbTabWidget->currentDatabaseWidget()->currentSelectedEntry();
            db    = dbTabWidget->currentDatabaseWidget()->database();
        }
        TranslateDialog::translateEntry(entry, db, &mainWindow);
    });

    // Pesquisar contexto da nota na web (027)
    QAction* actNoteSearch = cfdmMenu->addAction(
        QIcon::fromTheme("edit-find"),
        QObject::tr("Pesquisar contexto da &nota...")
    );
    QObject::connect(actNoteSearch, &QAction::triggered, [&mainWindow]() {
        auto* dbTabWidget = mainWindow.findChild<DatabaseTabWidget*>();
        if (!dbTabWidget || !dbTabWidget->currentDatabaseWidget()) return;
        auto* dbWidget = dbTabWidget->currentDatabaseWidget();
        Entry* entry = dbWidget->currentSelectedEntry();
        if (!entry) {
            QMessageBox::information(&mainWindow, QObject::tr("Pesquisar nota"),
                QObject::tr("Selecione uma entrada primeiro."));
            return;
        }

        // Extrai palavras-chave da nota (remove stopwords PT/EN, pega top 8)
        const QString noteId = NoteManager::instance().getNoteIdForEntry(entry->uuidToHex());
        QString text = noteId.isEmpty()
            ? entry->notes()
            : NoteManager::instance().getNoteContent(noteId);
        if (text.isEmpty()) text = entry->title();

        // Stopwords básicas PT+EN
        static const QSet<QString> stopwords = {
            "de","da","do","em","para","com","por","que","se","na","no",
            "uma","um","as","os","ao","aos","das","dos","pelo","pela",
            "the","and","or","in","on","at","to","for","of","a","an",
            "is","it","this","that","are","was","were","be","been",
            "have","has","had","with","from","by","as","but","not",
        };

        QMap<QString,int> freq;
        const QStringList words = text.split(QRegularExpression("[\\s\\W]+"),
                                             Qt::SkipEmptyParts);
        for (const QString& w : words) {
            const QString lw = w.toLower();
            if (lw.length() < 3 || stopwords.contains(lw)) continue;
            freq[lw]++;
        }

        // Ordena por frequência e pega os top 8
        QList<QPair<int,QString>> ranked;
        for (auto it = freq.begin(); it != freq.end(); ++it)
            ranked.append({it.value(), it.key()});
        std::sort(ranked.begin(), ranked.end(), std::greater<>());

        QStringList keywords;
        for (int i = 0; i < qMin(8, ranked.size()); ++i)
            keywords << ranked[i].second;

        if (keywords.isEmpty()) {
            keywords << entry->title();
        }

        // Abre menu para escolher onde buscar
        const QString query = keywords.join(" ");
        const QString enc   = QString::fromUtf8(QUrl::toPercentEncoding(query));

        QMenu searchCtxMenu(&mainWindow);
        searchCtxMenu.setTitle(QObject::tr("Pesquisar: %1").arg(query.left(50)));
        searchCtxMenu.addAction(QObject::tr("Google"), [enc]() {
            QDesktopServices::openUrl(QUrl("https://www.google.com/search?q=" + enc));
        });
        searchCtxMenu.addAction(QObject::tr("Wikipedia"), [enc]() {
            QDesktopServices::openUrl(QUrl("https://pt.wikipedia.org/wiki/Special:Search?search=" + enc));
        });
        searchCtxMenu.addAction(QObject::tr("ChatGPT"), [query]() {
            const QString prompt = QString::fromUtf8(QUrl::toPercentEncoding(
                "Me explique e dê referências sobre: " + query));
            QDesktopServices::openUrl(QUrl("https://chat.openai.com/?q=" + prompt));
        });
        searchCtxMenu.addAction(QObject::tr("GitHub"), [enc]() {
            QDesktopServices::openUrl(QUrl("https://github.com/search?q=" + enc));
        });
        searchCtxMenu.addSeparator();
        searchCtxMenu.addAction(QObject::tr("Palavras-chave extraídas: %1").arg(query))->setEnabled(false);
        searchCtxMenu.exec(QCursor::pos());
    });

    // Capturar Áudio / Transcrição (024)
    QAction* actAudio = cfdmMenu->addAction(
        QIcon::fromTheme("audio-input-microphone"),
        QObject::tr("&Capturar Áudio / Transcrição...")
    );
    actAudio->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_A));
    QObject::connect(actAudio, &QAction::triggered, [&mainWindow]() {
        auto* dbTabWidget = mainWindow.findChild<DatabaseTabWidget*>();
        Entry* entry = nullptr;
        QSharedPointer<Database> db;
        if (dbTabWidget && dbTabWidget->currentDatabaseWidget()) {
            entry = dbTabWidget->currentDatabaseWidget()->currentSelectedEntry();
            db    = dbTabWidget->currentDatabaseWidget()->database();
        }
        AudioCaptureDialog::captureAudio(entry, db, &mainWindow);
    });

    // Assistente de Voz (comandos falados → criar entrada/grupo/nota)
    QAction* actVoiceCmd = cfdmMenu->addAction(
        QIcon::fromTheme("audio-input-microphone"),
        QObject::tr("&Assistente de Voz...")
    );
    actVoiceCmd->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V));
    QObject::connect(actVoiceCmd, &QAction::triggered, [&mainWindow]() {
        auto* dbTabWidget = mainWindow.findChild<DatabaseTabWidget*>();
        QSharedPointer<Database> db;
        Group* grp   = nullptr;
        Entry* entry = nullptr;
        if (dbTabWidget && dbTabWidget->currentDatabaseWidget()) {
            auto* dbw = dbTabWidget->currentDatabaseWidget();
            db    = dbw->database();
            entry = dbw->currentSelectedEntry();
            // Grupo atual = grupo da entrada selecionada, ou grupo selecionado na árvore
            if (entry && entry->group())
                grp = entry->group();
            else if (db)
                grp = db->rootGroup();
        }
        VoiceCommandDialog::run(db, grp, entry, &mainWindow);
    });

    // Pesquisa Poderosa (011) — busca local + web + Google Dorking + verificar link
    QAction* actPowerSearch = cfdmMenu->addAction(
        QIcon::fromTheme("system-search"),
        QObject::tr("&Pesquisa Poderosa...")
    );
    actPowerSearch->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F));
    QObject::connect(actPowerSearch, &QAction::triggered, [&mainWindow]() {
        PowerSearchDialog dlg(&mainWindow);
        dlg.exec();
    });

    cfdmMenu->addSeparator();

    // Exportar nota criptografada (008)
    QAction* actExportNote = cfdmMenu->addAction(
        QIcon::fromTheme("document-send"),
        QObject::tr("&Exportar nota...")
    );
    QObject::connect(actExportNote, &QAction::triggered, [&mainWindow]() {
        auto* dbTabWidget = mainWindow.findChild<DatabaseTabWidget*>();
        if (!dbTabWidget || !dbTabWidget->currentDatabaseWidget()) return;
        auto* dbWidget = dbTabWidget->currentDatabaseWidget();
        Entry* entry = dbWidget->currentSelectedEntry();
        if (!entry) {
            QMessageBox::information(&mainWindow, QObject::tr("Exportar nota"),
                QObject::tr("Selecione uma entrada primeiro."));
            return;
        }
        ExportNoteDialog::exportNote(entry, dbWidget->database(), &mainWindow);
    });

    // Compartilhar entrada (009)
    QAction* actShare = cfdmMenu->addAction(
        QIcon::fromTheme("document-share"),
        QObject::tr("&Compartilhar entrada...")
    );
    QObject::connect(actShare, &QAction::triggered, [&mainWindow]() {
        auto* dbTabWidget = mainWindow.findChild<DatabaseTabWidget*>();
        if (!dbTabWidget || !dbTabWidget->currentDatabaseWidget()) return;
        Entry* entry = dbTabWidget->currentDatabaseWidget()->currentSelectedEntry();
        if (!entry) {
            QMessageBox::information(&mainWindow, QObject::tr("Compartilhar"),
                QObject::tr("Selecione uma entrada primeiro."));
            return;
        }
        ShareDialog::shareEntry(entry, &mainWindow);
    });

    cfdmMenu->addSeparator();

    // Definir ícone personalizado para a entrada selecionada (suporta GIF animado)
    QAction* actCustomIcon = cfdmMenu->addAction(
        QIcon::fromTheme("preferences-desktop-icons"),
        QObject::tr("Definir &ícone da entrada...")
    );
    QObject::connect(actCustomIcon, &QAction::triggered, [&mainWindow]() {
        auto* dbTabWidget = mainWindow.findChild<DatabaseTabWidget*>();
        if (!dbTabWidget || !dbTabWidget->currentDatabaseWidget()) return;

        auto* dbWidget = dbTabWidget->currentDatabaseWidget();
        Entry* entry = dbWidget->currentSelectedEntry();
        if (!entry) {
            QMessageBox::information(&mainWindow, QObject::tr("Definir ícone"),
                QObject::tr("Selecione uma entrada primeiro."));
            return;
        }

        const QString path = QFileDialog::getOpenFileName(
            &mainWindow,
            QObject::tr("Selecionar ícone personalizado"),
            QDir::homePath(),
            QObject::tr("Imagens (*.gif *.png *.jpg *.jpeg *.bmp *.svg *.ico *.webp);;"
                        "Todos os arquivos (*)")
        );
        if (path.isEmpty()) return;

        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(&mainWindow, QObject::tr("Erro"),
                QObject::tr("Não foi possível abrir o arquivo."));
            return;
        }
        const QByteArray iconBytes = f.readAll();
        f.close();
        if (iconBytes.isEmpty()) return;

        // GIF: armazena bytes originais (animação preservada na aba Notas)
        // Outros formatos: converte para PNG para compatibilidade máxima
        QByteArray finalBytes = iconBytes;
        if (!path.toLower().endsWith(".gif")) {
            QImage img;
            img.loadFromData(iconBytes);
            if (!img.isNull()) {
                QBuffer buf;
                buf.open(QIODevice::WriteOnly);
                img.save(&buf, "PNG");
                if (!buf.data().isEmpty()) finalBytes = buf.data();
            }
        }

        auto db = dbWidget->database();
        const QUuid iconUuid = QUuid::createUuid();
        db->metadata()->addCustomIcon(
            iconUuid, finalBytes,
            QFileInfo(path).fileName(),
            QDateTime::currentDateTimeUtc()
        );
        entry->setIcon(iconUuid);

        QString saveErr;
        db->save(Database::Atomic, {}, &saveErr);
        if (!saveErr.isEmpty()) {
            QMessageBox::warning(&mainWindow, QObject::tr("Aviso"),
                QObject::tr("Ícone definido, mas erro ao salvar: %1").arg(saveErr));
        }
    });

    cfdmMenu->addSeparator();

    // Informações dos bancos (Ctrl+Shift+I)
    QAction* actDbInfo = cfdmMenu->addAction(
        QIcon::fromTheme("dialog-information"),
        QObject::tr("&Informações dos Bancos...")
    );
    actDbInfo->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I));
    QObject::connect(actDbInfo, &QAction::triggered, [&mainWindow]() {
        DatabaseInfoDialog::showInfo(&mainWindow, &mainWindow);
    });

    // Manual de Consulta (F1)
    QAction* actManual = cfdmMenu->addAction(
        QIcon::fromTheme("help-contents"),
        QObject::tr("&Manual de Consulta")
    );
    actManual->setShortcut(QKeySequence(Qt::Key_F1));
    QObject::connect(actManual, &QAction::triggered, [&mainWindow]() {
        ManualDialog::showManual(&mainWindow);
    });

    // Verificar Dependências
    QObject::connect(
        cfdmMenu->addAction(
            QIcon::fromTheme("system-search"),
            QObject::tr("🔍 Verificar &Dependências...")),
        &QAction::triggered,
        [&mainWindow]() {
            DependencyCheckDialog::showFull(&mainWindow);
        });

    cfdmMenu->addSeparator();

    // Sobre CfdmNote
    QObject::connect(
        cfdmMenu->addAction(QObject::tr("Sobre &CfdmNote")),
        &QAction::triggered,
        [&mainWindow]() {
            QMessageBox::about(&mainWindow,
                QObject::tr("Sobre CfdmNote"),
                QObject::tr(
                    "<b>CfdmNote v%1</b><br><br>"
                    "Gerenciador de senhas com notas Markdown integradas.<br><br>"
                    "<b>Funcionalidades:</b><br>"
                    "&#8226; Entradas e grupos de senhas<br>"
                    "&#8226; Auto-Type<br>"
                    "&#8226; Notas Markdown vinculadas a entradas<br>"
                    "&#8226; [[wiki-links]] entre notas<br>"
                    "&#8226; Grafo de conhecimento<br>"
                    "&#8226; Banco nativo .ksdb (SQLite + FTS5)<br>"
                    "&#8226; Cofre criptografado opcional (.kdbx)"
                ).arg(APP_VERSION)
            );
        }
    );

    // ── Tooltips e StatusTips de todos os itens do menu CfdmNote ─────────────
    actNotes->setToolTip(QObject::tr(
        "<b>📝 Notas da Entrada</b> <i>(Ctrl+F9)</i><br><br>"
        "Abre/fecha o painel de Notas da entrada selecionada.<br>"
        "Edite texto em Markdown, adicione comentários rápidos<br>"
        "e visualize GIFs animados atribuídos à entrada."));
    actNotes->setStatusTip(QObject::tr("Abrir/fechar painel de Notas da entrada selecionada (Ctrl+F9)"));

    actOpenVault->setToolTip(QObject::tr(
        "<b>🔓 Abrir Cofre</b> <i>(Ctrl+Shift+O)</i><br><br>"
        "Abre o último Cofre Criptografado utilizado.<br>"
        "Cofres guardam dados sensíveis protegidos por senha<br>"
        "e/ou arquivo-chave (criptografia AES-256 + Argon2)."));
    actOpenVault->setStatusTip(QObject::tr("Abrir o último cofre .kdbx criptografado (Ctrl+Shift+O)"));

    actOpenOtherVault->setToolTip(QObject::tr(
        "<b>🗂️ Abrir Outro Cofre</b><br><br>"
        "Escolhe qualquer arquivo .kdbx para abrir como Cofre.<br>"
        "Use para alternar entre diferentes cofres criptografados."));
    actOpenOtherVault->setStatusTip(QObject::tr("Escolher e abrir outro arquivo .kdbx como cofre"));

    actVault->setToolTip(QObject::tr(
        "<b>🔒 Criar Cofre Criptografado</b><br><br>"
        "Cria um novo arquivo .kdbx protegido por senha<br>"
        "e/ou arquivo-chave. Use para guardar dados realmente<br>"
        "sensíveis (financeiros, senhas críticas, documentos)."));
    actVault->setStatusTip(QObject::tr("Criar novo cofre .kdbx criptografado com senha/arquivo-chave"));

    actBackup->setToolTip(QObject::tr(
        "<b>💾 Backup Externo</b> <i>(Ctrl+Shift+B)</i><br><br>"
        "Copia o banco de dados para um pendrive ou HD externo.<br>"
        "Detecta automaticamente mídias removíveis conectadas.<br>"
        "Mantém histórico dos últimos 20 backups com notas descritivas."));
    actBackup->setStatusTip(QObject::tr("Fazer backup em pendrive/HD externo (Ctrl+Shift+B)"));

    actImportCsv->setToolTip(QObject::tr(
        "<b>📋 Importar CSV</b><br><br>"
        "Importa entradas a partir de um arquivo CSV.<br>"
        "Compatível com exportações do Chrome, Firefox,<br>"
        "LastPass, Bitwarden e outros gestores de senhas."));
    actImportCsv->setStatusTip(QObject::tr("Importar entradas de arquivo .csv (Chrome, Firefox, LastPass, etc.)"));

    actImportTxt->setToolTip(QObject::tr(
        "<b>📄 Importar TXT</b><br><br>"
        "Cria entradas a partir de arquivos de texto (.txt).<br>"
        "O nome do arquivo vira o título da entrada;<br>"
        "o conteúdo é salvo na aba Notas."));
    actImportTxt->setStatusTip(QObject::tr("Criar entradas a partir de arquivos .txt"));

    actReminder->setToolTip(QObject::tr(
        "<b>⏰ Definir Lembrete</b> <i>(Ctrl+Shift+R)</i><br><br>"
        "Define um lembrete com data e hora para a entrada.<br>"
        "Suporta recorrência (diária, semanal, mensal).<br>"
        "Notificação permite adiar, abrir Google Calendar,<br>"
        "enviar via WhatsApp ou Telegram."));
    actReminder->setStatusTip(QObject::tr("Definir lembrete com data/hora para a entrada selecionada (Ctrl+Shift+R)"));

    actTranslate->setToolTip(QObject::tr(
        "<b>🌐 Traduzir Nota</b> <i>(Ctrl+Shift+T)</i><br><br>"
        "Traduz a nota da entrada para outro idioma.<br>"
        "Usa LibreTranslate (gratuito, sem cadastro/API key).<br>"
        "Suporta 22 idiomas: PT, EN, ES, FR, DE, IT, AR, ZH..."));
    actTranslate->setStatusTip(QObject::tr("Traduzir nota para outro idioma via LibreTranslate (Ctrl+Shift+T)"));

    actNoteSearch->setToolTip(QObject::tr(
        "<b>🔍 Pesquisar Contexto da Nota</b><br><br>"
        "Extrai as palavras-chave mais importantes da nota<br>"
        "e pesquisa automaticamente na web.<br>"
        "Abre Google, Wikipedia, ChatGPT ou GitHub<br>"
        "com os termos encontrados."));
    actNoteSearch->setStatusTip(QObject::tr("Extrair palavras-chave da nota e pesquisar na web"));

    actAudio->setToolTip(QObject::tr(
        "<b>🎙️ Capturar Áudio / Transcrição</b> <i>(Ctrl+Shift+A)</i><br><br>"
        "Grava áudio pelo microfone e transcreve para texto.<br>"
        "A transcrição é inserida automaticamente nas Notas.<br>"
        "Usa <i>whisper</i> (IA local, sem enviar dados para a nuvem).<br>"
        "<i>Requer: pip install openai-whisper</i>"));
    actAudio->setStatusTip(QObject::tr("Gravar áudio e transcrever para a nota com Whisper (Ctrl+Shift+A)"));

    actPowerSearch->setToolTip(QObject::tr(
        "<b>⚡ Pesquisa Poderosa</b> <i>(Ctrl+Shift+F)</i><br><br>"
        "Centro de pesquisa completo:<br>"
        "• <b>Arquivos Locais</b> — busca no disco/pendrive<br>"
        "• <b>Busca na Web</b> — Google, GitHub, YouTube, Wikipedia<br>"
        "• <b>Google Dorking</b> — busca avançada com operadores<br>"
        "• <b>Verificar Link</b> — VirusTotal, Safe Browsing, URLVoid<br>"
        "• <b>📈 Em Alta</b> — tendências globais por região"));
    actPowerSearch->setStatusTip(QObject::tr("Pesquisa Poderosa: arquivos + web + dorking + tendências globais (Ctrl+Shift+F)"));

    actExportNote->setToolTip(QObject::tr(
        "<b>📤 Exportar Nota</b><br><br>"
        "Exporta a nota da entrada em dois formatos:<br>"
        "• <b>.txt</b> — texto simples, legível em qualquer programa<br>"
        "• <b>.cfdmnote</b> — criptografado com AES-256-GCM + senha"));
    actExportNote->setStatusTip(QObject::tr("Exportar nota como .txt ou .cfdmnote criptografado"));

    actShare->setToolTip(QObject::tr(
        "<b>📲 Compartilhar Entrada</b><br><br>"
        "Compartilha o conteúdo da entrada (sem senha) via:<br>"
        "WhatsApp, Telegram, Discord ou Facebook.<br>"
        "<i>Nota: a senha da entrada NUNCA é enviada.</i>"));
    actShare->setStatusTip(QObject::tr("Compartilhar entrada via WhatsApp / Telegram / Discord / Facebook"));

    actCustomIcon->setToolTip(QObject::tr(
        "<b>🖼️ Definir Ícone da Entrada</b><br><br>"
        "Define uma imagem personalizada como ícone da entrada.<br>"
        "Suporta: PNG, JPG, GIF animado, SVG, ICO, WebP.<br>"
        "GIFs animados ficam visíveis na aba Notas da entrada."));
    actCustomIcon->setStatusTip(QObject::tr("Definir ícone personalizado para a entrada (PNG, GIF animado, SVG...)"));

    actDbInfo->setToolTip(QObject::tr(
        "<b>📊 Informações dos Bancos</b> <i>(Ctrl+Shift+I)</i><br><br>"
        "Mostra informações em tempo real de todos os bancos abertos:<br>"
        "tamanho do arquivo, número de entradas, grupos,<br>"
        "datas de criação e de última modificação."));
    actDbInfo->setStatusTip(QObject::tr("Ver informações dos bancos de dados abertos (Ctrl+Shift+I)"));

    actManual->setToolTip(QObject::tr(
        "<b>📖 Manual de Consulta</b> <i>(F1)</i><br><br>"
        "Abre o manual integrado do CfdmNote.<br>"
        "Inclui índice lateral navegável, busca por palavra<br>"
        "e renderização Markdown com formatação."));
    actManual->setStatusTip(QObject::tr("Abrir Manual de Consulta integrado (F1)"));

    // Insere o menu CfdmNote antes do menu Ajuda
    QList<QAction*> mbActions = mainWindow.menuBar()->actions();
    QAction* before = mbActions.isEmpty() ? nullptr : mbActions.last();
    mainWindow.menuBar()->insertMenu(before, cfdmMenu);

    // ── Ferramentas Visuais ────────────────────────────────────────────────────
    {
        cfdmMenu->addSeparator();
        auto* visualMenu = cfdmMenu->addMenu(
            QIcon::fromTheme("applications-graphics"),
            QObject::tr("🎨 &Ferramentas Visuais"));
        visualMenu->setToolTipsVisible(true);

        auto* actMindMap = visualMenu->addAction(
            QIcon::fromTheme("view-presentation"), QObject::tr("🧠 Mapa Mental"));
        actMindMap->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_M));
        actMindMap->setToolTip(QObject::tr(
            "<b>🧠 Mapa Mental</b> <i>(Ctrl+Shift+M)</i><br><br>"
            "Visualiza conexões entre entradas como um grafo interativo.<br>"
            "Conecta por domínio, tag ou referência manual.<br>"
            "Nós arrastáveis. Exporta para SVG."));

        auto* actRoadmap = visualMenu->addAction(
            QIcon::fromTheme("view-column"), QObject::tr("🗺️ Roadmap / Kanban"));
        actRoadmap->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_K));
        actRoadmap->setToolTip(QObject::tr(
            "<b>🗺️ Roadmap / Kanban</b> <i>(Ctrl+Shift+K)</i><br><br>"
            "Organiza entradas em colunas: Backlog → Em Progresso → Concluído → Aguardando.<br>"
            "Mova cards entre colunas com clique direito.<br>"
            "Status salvo no banco de dados."));

        auto* actWhiteboard = visualMenu->addAction(
            QIcon::fromTheme("draw-freehand"), QObject::tr("🖌️ Whiteboard"));
        actWhiteboard->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W));
        actWhiteboard->setToolTip(QObject::tr(
            "<b>🖌️ Whiteboard</b> <i>(Ctrl+Shift+W)</i><br><br>"
            "Canvas livre para organizar entradas visualmente.<br>"
            "Adicione sticky notes coloridas, arraste entradas e organize seu pensamento.<br>"
            "Layout salvo automaticamente no banco."));

        // Conecta às funções — usa o banco do DatabaseWidget ativo
        auto getActiveDb = [&mainWindow]() -> QSharedPointer<Database> {
            auto* dbTab = mainWindow.findChild<DatabaseTabWidget*>();
            if (!dbTab) return {};
            auto* dbWidget = dbTab->currentDatabaseWidget();
            if (!dbWidget) return {};
            return dbWidget->database();
        };

        auto getSelectedEntry = [&mainWindow]() -> Entry* {
            auto* dbTab = mainWindow.findChild<DatabaseTabWidget*>();
            if (!dbTab) return nullptr;
            auto* dbWidget = dbTab->currentDatabaseWidget();
            if (!dbWidget) return nullptr;
            return dbWidget->currentSelectedEntry();
        };

        QObject::connect(actMindMap, &QAction::triggered, [&mainWindow, getActiveDb, getSelectedEntry]() {
            auto db = getActiveDb();
            if (!db) { QMessageBox::information(&mainWindow, QObject::tr("Mapa Mental"),
                QObject::tr("Abra um banco de dados primeiro.")); return; }
            MindMapDialog::showMap(db, &mainWindow);
        });

        QObject::connect(actRoadmap, &QAction::triggered, [&mainWindow, getActiveDb]() {
            auto db = getActiveDb();
            if (!db) { QMessageBox::information(&mainWindow, QObject::tr("Roadmap"),
                QObject::tr("Abra um banco de dados primeiro.")); return; }
            RoadmapDialog::showRoadmap(db, &mainWindow);
        });

        QObject::connect(actWhiteboard, &QAction::triggered, [&mainWindow, getActiveDb]() {
            auto db = getActiveDb();
            if (!db) { QMessageBox::information(&mainWindow, QObject::tr("Whiteboard"),
                QObject::tr("Abra um banco de dados primeiro.")); return; }
            WhiteboardDialog::showBoard(db, &mainWindow);
        });
    }

    // ── Tema de cores (aplica antes de mostrar a janela) ──────────────────────
    ThemeManager::apply(ThemeManager::saved(), &app);

    // ── Submenu de temas no menu CfdmNote ────────────────────────────────────
    {
        cfdmMenu->addSeparator();
        auto* themeMenu = cfdmMenu->addMenu(
            QIcon::fromTheme("preferences-desktop-color"),
            QObject::tr("🎨 &Tema de Cores"));
        themeMenu->setToolTipsVisible(true);

        auto* actDefault = themeMenu->addAction(QObject::tr("⬜ Padrão do Sistema"));
        actDefault->setCheckable(true);
        actDefault->setToolTip(QObject::tr(
            "<b>⬜ Padrão do Sistema</b><br>Remove o tema personalizado e usa as cores "
            "padrão do sistema operacional."));

        auto* actNavy = themeMenu->addAction(QObject::tr("🌊 Navy Blue"));
        actNavy->setCheckable(true);
        actNavy->setToolTip(QObject::tr(
            "<b>🌊 Navy Blue</b><br>Azul marinho escuro — toolbar clara para ícones visíveis.<br>"
            "Ideal para uso prolongado. Aplicado automaticamente ao abrir o banco principal."));

        auto* actSteel = themeMenu->addAction(QObject::tr("💠 Steel Blue (Suavizado)"));
        actSteel->setCheckable(true);
        actSteel->setToolTip(QObject::tr(
            "<b>💠 Steel Blue</b><br>Azul mais suave e luminoso — fundo claro, acentos azul-aço.<br>"
            "Ícones completamente visíveis. Ótimo para ambientes com bastante luz."));

        themeMenu->addSeparator();
        auto* infoAct = themeMenu->addAction(
            QObject::tr("ℹ️  Verde: ativo automaticamente ao abrir Cofre"));
        infoAct->setEnabled(false);

        // Marca o tema atual
        const ThemeManager::Theme currentTheme = ThemeManager::saved();
        actDefault->setChecked(currentTheme == ThemeManager::Theme::Default);
        actNavy->setChecked(currentTheme == ThemeManager::Theme::NavyBlue);
        actSteel->setChecked(currentTheme == ThemeManager::Theme::SteelBlue);

        auto* group = new QActionGroup(&mainWindow);
        group->addAction(actDefault);
        group->addAction(actNavy);
        group->addAction(actSteel);
        group->setExclusive(true);

        QObject::connect(actDefault, &QAction::triggered, [&app]() {
            ThemeManager::apply(ThemeManager::Theme::Default, &app);
        });
        QObject::connect(actNavy, &QAction::triggered, [&app]() {
            ThemeManager::apply(ThemeManager::Theme::NavyBlue, &app);
        });
        QObject::connect(actSteel, &QAction::triggered, [&app]() {
            ThemeManager::apply(ThemeManager::Theme::SteelBlue, &app);
        });
    }

    mainWindow.show();

    // ── Botão 📊 na barra de status → abre DatabaseInfoDialog ────────────────
    {
        auto* btnDbInfo = new QPushButton(QObject::tr("📊  DB Info"), &mainWindow);
        btnDbInfo->setFlat(true);
        btnDbInfo->setFixedHeight(20);
        btnDbInfo->setStyleSheet(
            "QPushButton { font-size: 11px; color: #2c5282; border: none; "
            "padding: 0 8px; } "
            "QPushButton:hover { color: #1a365d; text-decoration: underline; }");
        btnDbInfo->setToolTip(QObject::tr("Informações dos bancos abertos (Ctrl+Shift+I)"));
        mainWindow.statusBar()->addPermanentWidget(btnDbInfo);
        QObject::connect(btnDbInfo, &QPushButton::clicked, [&mainWindow]() {
            DatabaseInfoDialog::showInfo(&mainWindow, &mainWindow);
        });
    }

    // ── Tooltip rico na GroupView (árvore de grupos) ──────────────────────────
    QTimer::singleShot(800, &mainWindow, [&mainWindow]() {
        // A GroupView é a QTreeView que NÃO está dentro de um DatabaseWidget
        const auto allViews = mainWindow.findChildren<QTreeView*>();
        for (auto* view : allViews) {
            // Ignora views dentro de DatabaseWidget (essas são entry views)
            if (view->parent() && qobject_cast<DatabaseWidget*>(
                    view->parent()->parent())) continue;
            // Instala apenas se ainda não tiver o filtro
            bool already = false;
            for (auto* ef : view->children())
                if (qobject_cast<GroupTooltipFilter*>(ef)) { already = true; break; }
            if (!already && qobject_cast<GroupModel*>(view->model()))
                view->installEventFilter(new GroupTooltipFilter());
        }
    });

    // ── Botão 🤖 IA na toolbar — reservado para CFDM Triplex (feature 012) ───────
    QTimer::singleShot(700, &mainWindow, [&mainWindow]() {
        const auto toolbars = mainWindow.findChildren<QToolBar*>();
        if (toolbars.isEmpty()) return;
        QToolBar* tb = toolbars.first();

        tb->addSeparator();

        auto* btn = new QToolButton(tb);
        btn->setText(QObject::tr("🤖 IA"));
        btn->setIcon(QIcon::fromTheme("system-run"));
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        btn->setStyleSheet(
            "QToolButton { font-weight: bold; color: #805ad5; padding: 2px 10px; }"
            "QToolButton:hover { background: #faf5ff; border-radius: 4px; }"
            "QToolButton:pressed { background: #e9d8fd; }");
        btn->setToolTip(QObject::tr(
            "<b>🤖 IA / Assistente de Voz</b><br><br>"
            "• Assistente de Voz: crie entradas, grupos e notas por voz<br>"
            "• CFDM Triplex IA: em desenvolvimento"));
        btn->setPopupMode(QToolButton::InstantPopup);

        auto* iaMenu = new QMenu(btn);

        // Assistente de Voz — disponível agora
        auto* actVoiceIA = iaMenu->addAction(
            QIcon::fromTheme("audio-input-microphone"),
            QObject::tr("🎙 Assistente de Voz  (Ctrl+Shift+V)"));
        QObject::connect(actVoiceIA, &QAction::triggered, &mainWindow, [&mainWindow]() {
            auto* dbTabWidget = mainWindow.findChild<DatabaseTabWidget*>();
            QSharedPointer<Database> db;
            Group* grp   = nullptr;
            Entry* entry = nullptr;
            if (dbTabWidget && dbTabWidget->currentDatabaseWidget()) {
                auto* dbw = dbTabWidget->currentDatabaseWidget();
                db    = dbw->database();
                entry = dbw->currentSelectedEntry();
                if (entry && entry->group()) grp = entry->group();
                else if (db)                 grp = db->rootGroup();
            }
            VoiceCommandDialog::run(db, grp, entry, &mainWindow);
        });

        iaMenu->addSeparator();

        // CFDM Triplex — em desenvolvimento
        auto* actTriplex = iaMenu->addAction(
            QIcon::fromTheme("system-run"),
            QObject::tr("🤖 CFDM Triplex IA  (em breve)"));
        actTriplex->setEnabled(false);

        btn->setMenu(iaMenu);
        tb->addWidget(btn);
    });

    // ── Tema automático: volta ao Navy Blue quando cofre é fechado ────────────
    QTimer::singleShot(600, &mainWindow, [&mainWindow, &app]() {
        auto* dbTabWidget = mainWindow.findChild<DatabaseTabWidget*>();
        if (!dbTabWidget) return;
        QObject::connect(dbTabWidget, &DatabaseTabWidget::databaseClosed,
                         &mainWindow, [&mainWindow, &app](const QString& closedPath) {
            // Se o path fechado não é o banco principal → era um cofre → volta ao azul
            QSettings s(AppLockDialog::configPath(), QSettings::IniFormat);
            const QString mainPath = s.value("maindb/path").toString();
            if (closedPath != mainPath)
                ThemeManager::apply(ThemeManager::Theme::NavyBlue, &app);
        });
    });

    // ── Verificação de dependências (primeira execução ou se faltar algo) ────
    QTimer::singleShot(800, &mainWindow, [&mainWindow]() {
        DependencyCheckDialog::checkOnStartup(&mainWindow);
    });

    // ── Argumento de linha de comando: --database /caminho/banco.kdbx ────────
    {
        const QStringList args = app.arguments();
        const int dbIdx = args.indexOf("--database");
        if (dbIdx != -1 && dbIdx + 1 < args.size()) {
            const QString cliPath = args.at(dbIdx + 1);
            if (QFile::exists(cliPath)) {
                MainDatabaseManager::setConfiguredPath(cliPath);
            } else {
                QMessageBox::warning(&mainWindow,
                    QObject::tr("Banco não encontrado"),
                    QObject::tr("O arquivo indicado em --database não foi encontrado:\n%1").arg(cliPath));
            }
        }
    }

    // ── Descoberta automática se banco configurado não existir ────────────────
    {
        const QString cfgPath = MainDatabaseManager::configuredPath();
        const bool isDefault  = (cfgPath == MainDatabaseManager::defaultDatabasePath());
        if (!QFile::exists(cfgPath) || isDefault) {
            // Varre a máquina por bancos existentes (excluindo o caminho padrão vazio)
            const QStringList found = MainDatabaseManager::discoverOnMachine();
            // Filtra para não sugerir o próprio banco padrão vazio
            QStringList candidates;
            for (const QString& f : found)
                if (f != MainDatabaseManager::defaultDatabasePath())
                    candidates << f;

            if (!candidates.isEmpty()) {
                // Mostra picker simples
                auto* picker = new QDialog(&mainWindow);
                picker->setWindowTitle(QObject::tr("Bancos encontrados na máquina"));
                picker->resize(600, 320);
                auto* lay = new QVBoxLayout(picker);
                lay->addWidget(new QLabel(
                    QObject::tr("Encontramos bancos de dados na sua máquina.\n"
                                "Selecione um para abrir, ou clique em Novo para criar um banco limpo:")));
                auto* list = new QListWidget(picker);
                list->addItems(candidates);
                list->setCurrentRow(0);
                lay->addWidget(list);
                auto* btnRow = new QHBoxLayout();
                auto* btnOpen = new QPushButton(QObject::tr("Abrir selecionado"));
                auto* btnNew  = new QPushButton(QObject::tr("Criar novo banco"));
                auto* btnSkip = new QPushButton(QObject::tr("Perguntar depois"));
                btnRow->addWidget(btnOpen);
                btnRow->addWidget(btnNew);
                btnRow->addStretch();
                btnRow->addWidget(btnSkip);
                lay->addLayout(btnRow);
                QObject::connect(btnOpen, &QPushButton::clicked, picker, [picker, list]() {
                    if (list->currentItem())
                        MainDatabaseManager::setConfiguredPath(list->currentItem()->text());
                    picker->accept();
                });
                QObject::connect(btnNew, &QPushButton::clicked, picker, [picker]() {
                    // Volta ao banco padrão (será criado em createIfNeeded)
                    MainDatabaseManager::setConfiguredPath(MainDatabaseManager::defaultDatabasePath());
                    picker->accept();
                });
                QObject::connect(btnSkip, &QPushButton::clicked, picker, &QDialog::reject);
                picker->exec();
                picker->deleteLater();
            }
        }
    }

    // ── Abre banco principal (sem senha) → substitui a tela de boas-vindas ───
    QTimer::singleShot(100, &mainWindow, [&mainWindow]() {
        MainDatabaseManager::openInMainWindow(&mainWindow);
    });

    return app.exec();
}

#include "main.moc"
