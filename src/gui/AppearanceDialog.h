#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QFontComboBox>
#include <QTabWidget>
#include <QApplication>
#include <QSettings>
#include <QColor>
#include <QMainWindow>

/**
 * AppearanceDialog — personalizar cores, fonte, peso e estilo de texto
 * do CfdmNote, incluindo elementos herdados do KeePassXC.
 *
 * Abas:
 *   Cores     — fundo, toolbar, texto, destaque
 *   Texto     — fonte, tamanho e peso por área (menus, botões, diálogos, listas)
 *
 * Acesso: menu CfdmNote → Tema → Personalizar  OU  clique direito na janela
 */
class AppearanceDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AppearanceDialog(QApplication* app, QWidget* parent = nullptr);

    static void applySaved(QApplication* app);

    static QString buildQss(const QString& bg, const QString& toolbarBg,
                             const QString& text, const QString& accent,
                             const QString& fontFamily, int fontSize,
                             int menuFontSize, int btnFontSize,
                             int dialogFontSize, int listFontSize,
                             int btnWeight,        // QFont::Weight int
                             int menuWeight,
                             bool boldDialogTitle);

signals:
    void resetRequested();

private slots:
    void onPickBg();
    void onPickToolbar();
    void onPickText();
    void onPickAccent();
    void onPreview();
    void onApply();
    void onReset();

private:
    void buildUI();
    void loadSaved();
    void updatePreviewLabel();

    QApplication* m_app;

    // Cores
    QColor m_bg        { "#1e3a5f" };
    QColor m_toolbar   { "#4a7fb5" };
    QColor m_text      { "#f0f4f8" };
    QColor m_accent    { "#60a5fa" };

    QPushButton* m_btnBg      = nullptr;
    QPushButton* m_btnToolbar = nullptr;
    QPushButton* m_btnText    = nullptr;
    QPushButton* m_btnAccent  = nullptr;

    // Fonte geral
    QFontComboBox* m_fontCombo = nullptr;
    QSpinBox*      m_fontSize  = nullptr;

    // Texto avançado
    QSpinBox*   m_menuSize     = nullptr;   // tamanho fonte menus
    QSpinBox*   m_btnSize      = nullptr;   // tamanho fonte botões
    QSpinBox*   m_dialogSize   = nullptr;   // tamanho fonte diálogos
    QSpinBox*   m_listSize     = nullptr;   // tamanho fonte listas/entradas
    QComboBox*  m_btnWeight    = nullptr;   // Normal/Médio/Negrito/ExtraNegrito
    QComboBox*  m_menuWeight   = nullptr;
    QComboBox*  m_dlgTitleBold = nullptr;  // título de dialog: normal/negrito

    QLabel* m_preview = nullptr;
};
