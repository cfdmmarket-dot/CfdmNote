/*
 * CfdmNote — fork do KeePassXC com notas Markdown integradas
 * Baseado em: third_party/keepassxc/src/main.cpp
 */

#include <QApplication>
#include <QCommandLineParser>

#include "gui/KpsMainWindow.h"
#include "core/Config.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("CfdmNote");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("CfdmNote");
    app.setOrganizationDomain("cfdmnote.app");

    QCommandLineParser parser;
    parser.setApplicationDescription("Gerenciador de senhas com notas Markdown integradas");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("banco", "Arquivo .ksdb para abrir (opcional)", "[banco]");
    parser.process(app);

    KpsMainWindow window;
    window.show();

    // Abre arquivo passado como argumento, se houver
    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        window.openDatabase(args.first());
    }

    return app.exec();
}
