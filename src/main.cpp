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

// Motor interno do CfdmNote
#include "gui/Application.h"
#include "gui/MainWindow.h"
#include "core/Bootstrap.h"

// Nossas adições
#include "gui/NoteEditorWidget.h"
#include "gui/VaultDialog.h"

static const QString APP_NAME    = "CfdmNote";
static const QString APP_VERSION = "1.0.1";

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
                if (title.contains("KeePassXC")) {
                    title.replace("KeePassXC", APP_NAME);
                    w->setWindowTitle(title);
                    return true;
                }
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

int main(int argc, char* argv[])
{
    Application app(argc, argv);

    // Bootstrap do motor (crypto, config, recursos, traduções, etc.)
    Bootstrap::bootstrap();
    app.setApplicationName(APP_NAME);
    app.setApplicationDisplayName(APP_NAME);
    app.setApplicationVersion(APP_VERSION);

    // ── Ícone do CfdmNote ─────────────────────────────────────────────────────
    const QString iconPath = QString("%1/../resources/icons/cfdmnote_128.png")
                             .arg(app.applicationDirPath());
    if (QFile::exists(iconPath))
        app.setWindowIcon(QIcon(iconPath));

    // ── Janela principal do CfdmNote ──────────────────────────────────────────
    MainWindow mainWindow;

    // Instala filtro de título (substitui "KeePassXC" → "CfdmNote")
    auto* titleFilter = new TitleFilter(&mainWindow);
    mainWindow.installEventFilter(titleFilter);

    // Força o título correto após a janela inicializar
    QTimer::singleShot(0, &mainWindow, [&mainWindow]() {
        QString t = mainWindow.windowTitle();
        t.replace("KeePassXC", APP_NAME);
        mainWindow.setWindowTitle(t.isEmpty() ? APP_NAME : t);
    });

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

    // ── Menu CfdmNote ─────────────────────────────────────────────────────────
    QMenu* cfdmMenu = new QMenu(QObject::tr("&CfdmNote"), &mainWindow);

    // Mostrar/ocultar painel de notas
    QAction* actNotes = cfdmMenu->addAction(QObject::tr("Painel de &Notas"));
    actNotes->setCheckable(true);
    actNotes->setChecked(true);
    actNotes->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F9));
    QObject::connect(actNotes, &QAction::toggled, noteDock, &QDockWidget::setVisible);

    cfdmMenu->addSeparator();

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

    mainWindow.show();

    // Abre arquivo passado como argumento
    const QStringList cliArgs = app.arguments();
    if (cliArgs.size() > 1) {
        const QString path = cliArgs.last();
        if (QFile::exists(path))
            mainWindow.openDatabase(path);
    }

    return app.exec();
}
