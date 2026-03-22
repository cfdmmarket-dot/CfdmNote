/*
 * CfdmNote — Gerenciador de senhas com notas Markdown integradas
 *
 * Interface: CfdmNote (baseada em motor open-source de gerenciamento de senhas).
 * Adiciona painel de notas como QDockWidget e menu CfdmNote.
 */

#include <QFile>
#include <QDockWidget>
#include <QAction>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>

// Motor interno do CfdmNote
#include "gui/Application.h"
#include "gui/MainWindow.h"
#include "core/Bootstrap.h"

// Nossas adições
#include "gui/NoteEditorWidget.h"
#include "gui/VaultDialog.h"

int main(int argc, char* argv[])
{
    Application app(argc, argv);

    // Bootstrap do motor (crypto, config, recursos, traduções, etc.)
    Bootstrap::bootstrap();
    app.setApplicationName("CfdmNote");
    app.setApplicationDisplayName("CfdmNote");
    app.setApplicationVersion("1.0.0");

    // ── Janela principal do CfdmNote ─────────────────────────────────────────
    MainWindow mainWindow;

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

    // Sobre
    QObject::connect(
        cfdmMenu->addAction(QObject::tr("Sobre &CfdmNote")),
        &QAction::triggered,
        [&mainWindow]() {
            QMessageBox::about(&mainWindow,
                QObject::tr("Sobre CfdmNote"),
                QObject::tr(
                    "<b>CfdmNote v1.0</b><br><br>"
                    "Gerenciador de senhas com notas Markdown integradas.<br><br>"
                    "<b>Funcionalidades:</b><br>"
                    "&#8226; Entradas e grupos de senhas<br>"
                    "&#8226; Notas Markdown vinculadas a entradas<br>"
                    "&#8226; [[wiki-links]] entre notas<br>"
                    "&#8226; Grafo de conhecimento<br>"
                    "&#8226; Banco nativo .ksdb (SQLite + FTS5)<br>"
                    "&#8226; Cofre criptografado opcional (.kdbx)<br>"
                    "&#8226; Auto-Type"
                )
            );
        }
    );

    // Insere o menu antes do último (Ajuda)
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
