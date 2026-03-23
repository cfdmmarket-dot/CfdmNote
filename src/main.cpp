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
#include "db/MainDatabaseManager.h"

static const QString APP_NAME    = "CfdmNote";
static const QString APP_VERSION = "1.1.0";

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
 * Conecta o sinal entrySelectionChanged do DatabaseWidget ao NoteEditorWidget.
 * Chamado sempre que um banco é aberto ou trocado.
 */
static void connectDbWidget(DatabaseWidget* dbWidget, NoteEditorWidget* noteEditor)
{
    if (!dbWidget || !noteEditor) return;
    QObject::connect(dbWidget, &DatabaseWidget::entrySelectionChanged,
                     noteEditor, [dbWidget, noteEditor]() {
        Entry* entry = dbWidget->currentSelectedEntry();
        if (entry)
            noteEditor->loadNoteForEntry(entry->uuidToHex());
        else
            noteEditor->clearNote();
    });
}

static void connectEntryToNotes(MainWindow* mw, NoteEditorWidget* noteEditor)
{
    auto* tabWidget = mw->findChild<DatabaseTabWidget*>();
    if (!tabWidget) return;

    // Quando banco ativo mudar → reconecta
    QObject::connect(tabWidget, &DatabaseTabWidget::activeDatabaseChanged,
                     noteEditor, [noteEditor](DatabaseWidget* dbWidget) {
        if (!dbWidget)
            noteEditor->clearNote();
        else
            connectDbWidget(dbWidget, noteEditor);
    });

    // Banco já aberto agora (se houver)
    connectDbWidget(tabWidget->currentDatabaseWidget(), noteEditor);
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

    // ── Painel de Notas Markdown (dock lateral) ───────────────────────────────
    auto* noteDock = new QDockWidget(QObject::tr("Notas"), &mainWindow);
    noteDock->setObjectName("CfdmNotesDock");
    noteDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    noteDock->setFeatures(QDockWidget::DockWidgetMovable   |
                          QDockWidget::DockWidgetFloatable |
                          QDockWidget::DockWidgetClosable);
    auto* noteEditor = new NoteEditorWidget(noteDock);
    noteDock->setWidget(noteEditor);
    mainWindow.addDockWidget(Qt::RightDockWidgetArea, noteDock);

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
    QTimer::singleShot(0, &mainWindow, [&mainWindow, &iconPath, noteEditor, busyOverlay]() {
        // Título com versão
        QString t = mainWindow.windowTitle();
        t.replace("KeePassXC", APP_NAME);
        if (!t.contains(APP_VERSION))
            t = t.isEmpty() ? QString("%1 v%2").arg(APP_NAME, APP_VERSION)
                            : QString("%1 — v%2").arg(t, APP_VERSION);
        mainWindow.setWindowTitle(t);

        // WelcomeWidget: texto de boas-vindas + ícone
        patchWelcomeWidget(&mainWindow, iconPath);

        // Conecta seleção de entrada → painel de notas
        connectEntryToNotes(&mainWindow, noteEditor);

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

    // Mostrar/ocultar painel de notas
    QAction* actNotes = cfdmMenu->addAction(QObject::tr("Painel de &Notas"));
    actNotes->setCheckable(true);
    actNotes->setChecked(true);
    actNotes->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F9));
    QObject::connect(actNotes, &QAction::toggled, noteDock, &QDockWidget::setVisible);

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
