#pragma once

#include <QDialog>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QApplication>
#include <QVBoxLayout>
#include <QMainWindow>
#include <QSettings>

/**
 * IconThemeDialog — substitui ícones do CfdmNote E do KeePassXC herdado.
 *
 * Aba 1 "Ícones do CfdmNote" — ícones próprios (app, toolbar, tray).
 * Aba 2 "Ícones do App (Menus/Toolbar)" — escaneia todos os QActions ativos
 *        e permite substituir cada ícone individualmente.
 *
 * Overrides persistidos em QSettings "cfdmnote/icon_action_overrides".
 * IconThemeDialog::applyActionOverrides(window) reaplica ao iniciar.
 */
class IconThemeDialog : public QDialog
{
    Q_OBJECT
public:
    explicit IconThemeDialog(QApplication* app, QMainWindow* mainWindow,
                              QWidget* parent = nullptr);

    // Carrega ícone: user folder → QRC fallback
    static QIcon icon(const QString& name, const QSize& size = {});

    // Pasta de ícones customizados do usuário
    static QString userIconsDir();

    // Abre QFileDialog e copia para userIconsDir/name
    static bool replaceIcon(const QString& iconName, QWidget* parent = nullptr);

    // Reaplica overrides de QActions (chame após setup da janela)
    static void applyActionOverrides(QMainWindow* window);

private:
    void buildUI();
    void buildTabCfdmIcons(QWidget* tab);
    void buildTabActionIcons(QWidget* tab);

    void addCfdmIconRow(QVBoxLayout* lay, const QString& name,
                         const QString& description);

    QApplication* m_app;
    QMainWindow*  m_mainWindow;
};
