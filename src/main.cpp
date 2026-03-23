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
#include <QTabWidget>
#include <QBuffer>
#include <QMovie>
#include <QSplitter>
#include <QVBoxLayout>
#include <QDateTime>
#include <QFileInfo>

#include "core/Metadata.h"

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
#include "db/MainDatabaseManager.h"

static const QString APP_NAME    = "CfdmNote";
static const QString APP_VERSION = "1.2.0";

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

    auto* noteEditor = new NoteEditorWidget();
    notesLayout->addWidget(noteEditor, 1);

    tabWidget->addTab(notesContainer,
                      QIcon::fromTheme("text-editor"), QObject::tr("Notas"));

    // Aba Links (Local Disk + External/Removable)
    auto* linksPanel = new LinksPanelWidget();
    linksPanel->setObjectName("linksPanelWidget");
    tabWidget->addTab(linksPanel,
                      QIcon::fromTheme("folder-remote"), QObject::tr("Links"));

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
        auto* noteEditor = setupNotesTab(dbWidget);
        if (!noteEditor) {
            // Já configurado — garante que o NoteEditorWidget existente está conectado
            noteEditor = dbWidget->findChild<NoteEditorWidget*>();
            if (!noteEditor) return;
        }

        QObject::connect(dbWidget, &DatabaseWidget::entrySelectionChanged,
                         noteEditor, [dbWidget, noteEditor]() {
            auto* gifLabel    = dbWidget->findChild<QLabel*>("gifIconLabel");
            auto* linksPanel  = dbWidget->findChild<LinksPanelWidget*>();
            Entry* entry = dbWidget->currentSelectedEntry();
            if (entry) {
                noteEditor->loadNoteForEntry(entry->uuidToHex());
                updateGifPreview(gifLabel, entry, dbWidget->database());
                if (linksPanel) linksPanel->loadEntry(entry, dbWidget->database());
            } else {
                noteEditor->clearNote();
                updateGifPreview(gifLabel, nullptr, {});
                if (linksPanel) linksPanel->clearLinks();
            }
        }, Qt::UniqueConnection);
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
    Application app(argc, argv);

    // Bootstrap do motor (crypto, config, recursos, traduções, etc.)
    Bootstrap::bootstrap();
    app.setApplicationName(APP_NAME);
    app.setApplicationDisplayName(APP_NAME);
    app.setApplicationVersion(APP_VERSION);

    // ── Tela de login do CfdmNote (antes de qualquer janela) ─────────────────
    {
        AppLockDialog lockDlg;
        if (lockDlg.exec() != QDialog::Accepted)
            return 0;  // usuário fechou o diálogo — encerra o app
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

        // Intercepta ação de import — mostra overlay antes do freeze
        auto* actionImport = mainWindow.findChild<QAction*>("actionImport");
        if (actionImport) {
            QObject::connect(actionImport, &QAction::triggered, busyOverlay, [busyOverlay, &mainWindow]() {
                busyOverlay->setGeometry(mainWindow.rect());
                busyOverlay->raise();
                busyOverlay->setVisible(true);
                QApplication::processEvents();
            });
        }
    });

    // ── Menu CfdmNote ─────────────────────────────────────────────────────────
    QMenu* cfdmMenu = new QMenu(QObject::tr("&CfdmNote"), &mainWindow);

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

    cfdmMenu->addSeparator();

    // Abrir cofre criptografado (.kdbx)
    QAction* actOpenVault = cfdmMenu->addAction(
        QIcon::fromTheme("document-open"),
        QObject::tr("&Abrir Cofre...")
    );
    actOpenVault->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));
    QObject::connect(actOpenVault, &QAction::triggered, [&mainWindow]() {
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
        }
    });

    // Abrir cofre diferente (sempre mostra seletor de arquivo)
    QAction* actOpenOtherVault = cfdmMenu->addAction(
        QIcon::fromTheme("document-open"),
        QObject::tr("Abrir &outro cofre...")
    );
    QObject::connect(actOpenOtherVault, &QAction::triggered, [&mainWindow]() {
        const QString path = QFileDialog::getOpenFileName(
            &mainWindow,
            QObject::tr("Abrir Cofre CfdmNote"),
            QDir::homePath(),
            QObject::tr("Cofre CfdmNote (*.kdbx)")
        );
        if (!path.isEmpty()) {
            VaultUnlockDialog dlg(path, &mainWindow, &mainWindow);
            dlg.exec();
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

    // Insere o menu CfdmNote antes do menu Ajuda
    QList<QAction*> mbActions = mainWindow.menuBar()->actions();
    QAction* before = mbActions.isEmpty() ? nullptr : mbActions.last();
    mainWindow.menuBar()->insertMenu(before, cfdmMenu);

    // ── Desabilita auto-open do KeePassXC (usamos o nosso próprio) ──────────
    config()->set(Config::OpenPreviousDatabasesOnStartup, false);

    mainWindow.show();

    // ── Abre banco principal (sem senha) → substitui a tela de boas-vindas ──
    // O cofre criptografado só abre quando invocado pelo menu CfdmNote
    QTimer::singleShot(100, &mainWindow, [&mainWindow]() {
        MainDatabaseManager::openInMainWindow(&mainWindow);
    });

    // Abre arquivo passado como argumento
    const QStringList cliArgs = app.arguments();
    if (cliArgs.size() > 1) {
        const QString path = cliArgs.last();
        if (QFile::exists(path))
            mainWindow.openDatabase(path);
    }

    return app.exec();
}
