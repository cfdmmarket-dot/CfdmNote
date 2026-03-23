#pragma once
#include <QString>
#include <QApplication>
#include <QSettings>

/**
 * ThemeManager — aplica temas de cores ao CfdmNote via QSS.
 * Os temas afetam apenas os widgets do CfdmNote;
 * o núcleo do KeePassXC tem estilo próprio mas herda a maioria.
 */
class ThemeManager
{
public:
    enum class Theme { Default, NavyBlue, SteelBlue, VaultGreen };

    // Aplica um tema — VaultGreen NÃO é persistido (é temporário para o cofre)
    static void apply(Theme theme, QApplication* app) {
        switch (theme) {
            case Theme::NavyBlue:   app->setStyleSheet(navyBlue());   break;
            case Theme::SteelBlue:  app->setStyleSheet(steelBlue());  break;
            case Theme::VaultGreen: app->setStyleSheet(vaultGreen()); break;
            default:                app->setStyleSheet({});            break;
        }
        if (theme != Theme::VaultGreen) {
            QSettings cfg;
            QString name = "default";
            if (theme == Theme::NavyBlue)  name = "navy";
            if (theme == Theme::SteelBlue) name = "steel";
            cfg.setValue("cfdmnote/theme", name);
        }
    }

    static Theme saved() {
        QSettings cfg;
        const QString t = cfg.value("cfdmnote/theme", "navy").toString();
        if (t == "navy")  return Theme::NavyBlue;
        if (t == "steel") return Theme::SteelBlue;
        return Theme::Default;
    }

    static QString steelBlue() {
        return R"(
/* ═══════════════════════════════════════════════════
   CfdmNote — Tema Steel Blue (Azul Suavizado)
   Tom intermediário — fundo claro, acentos azul-aço.
   Paleta: #2b6cb0 (dark) · #3182ce (mid) · #4299e1 (accent)
           #bee3f8 (light) · #f7fbff (very light)
   ═══════════════════════════════════════════════════ */

QMainWindow, QDialog, QWidget { background-color: #f7fbff; color: #1a202c; }

QMenuBar {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #4299e1, stop:1 #3182ce);
    color: #fff; border-bottom: 2px solid #2b6cb0; padding: 1px 4px;
}
QMenuBar::item { background: transparent; padding: 5px 12px; border-radius: 4px; color: #fff; }
QMenuBar::item:selected, QMenuBar::item:pressed { background: #2b6cb0; color: #fff; }

QMenu {
    background: #ebf8ff; color: #1a365d;
    border: 1px solid #4299e1; border-radius: 6px; padding: 4px 0;
}
QMenu::item { padding: 6px 28px 6px 12px; border-radius: 3px; margin: 1px 4px; }
QMenu::item:selected { background: #4299e1; color: white; }
QMenu::separator { height: 1px; background: #bee3f8; margin: 4px 10px; }

/* Toolbar clara — ícones pretos totalmente visíveis */
QToolBar {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #ebf8ff, stop:1 #dbeafe);
    border: none; border-bottom: 1px solid #bee3f8;
    spacing: 3px; padding: 2px 4px;
}
QToolBar::separator { background: #bee3f8; width: 1px; margin: 4px 3px; }
QToolButton { color: #1a365d; border: none; border-radius: 4px; padding: 4px 8px; }
QToolButton:hover { background: #bee3f8; color: #1a365d; border: 1px solid #90cdf4; }
QToolButton:pressed, QToolButton:checked { background: #4299e1; color: white; }

QTabWidget::pane { border: 1px solid #bee3f8; background: #fff; border-radius: 0 4px 4px 4px; }
QTabBar::tab {
    background: #ebf8ff; color: #2b6cb0;
    border: 1px solid #bee3f8; border-bottom: none;
    padding: 6px 16px; border-top-left-radius: 5px; border-top-right-radius: 5px;
    margin-right: 2px; font-size: 12px;
}
QTabBar::tab:selected { background: #2b6cb0; color: #fff; font-weight: bold; border-color: #2b6cb0; }
QTabBar::tab:hover:!selected { background: #bee3f8; color: #1a365d; }

QTreeView, QListView, QTableView {
    background: #fff; alternate-background-color: #f7fbff;
    selection-background-color: #4299e1; selection-color: #fff;
    border: 1px solid #bee3f8; border-radius: 4px; outline: none;
}
QTreeView::item:hover, QListView::item:hover { background: #dbeafe; color: #1a365d; }
QTreeView::item:selected, QListView::item:selected { background: #4299e1; color: white; }

QHeaderView::section {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #4299e1, stop:1 #3182ce);
    color: #fff; border: none; border-right: 1px solid #bee3f8;
    padding: 6px 10px; font-weight: bold; font-size: 11px;
}
QHeaderView::section:hover { background: #2b6cb0; }

QLineEdit, QPlainTextEdit, QTextEdit {
    background: #fff; color: #1a202c; border: 1px solid #bee3f8;
    border-radius: 4px; padding: 4px 8px;
    selection-background-color: #4299e1; selection-color: white;
}
QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus { border: 2px solid #4299e1; background: #f7fbff; }

QPushButton {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #4299e1, stop:1 #3182ce);
    color: #fff; border: none; border-radius: 5px;
    padding: 6px 16px; font-weight: bold; font-size: 12px;
}
QPushButton:hover { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #63b3ed, stop:1 #4299e1); }
QPushButton:pressed { background: #2b6cb0; }
QPushButton:disabled { background: #bee3f8; color: #e2e8f0; }
QPushButton:flat { background: transparent; color: #3182ce; border: 1px solid #bee3f8; font-weight: normal; }
QPushButton:flat:hover { background: #ebf8ff; border-color: #4299e1; }

QComboBox {
    background: #fff; color: #1a202c; border: 1px solid #bee3f8;
    border-radius: 4px; padding: 4px 32px 4px 8px;
}
QComboBox:focus { border: 2px solid #4299e1; }
QComboBox::drop-down { background: #3182ce; width: 24px; border-left: 1px solid #bee3f8; border-top-right-radius: 4px; border-bottom-right-radius: 4px; }
QComboBox QAbstractItemView { background: #ebf8ff; color: #1a365d; selection-background-color: #4299e1; border: 1px solid #4299e1; }

QScrollBar:vertical { background: #f0f7ff; width: 10px; border-radius: 5px; }
QScrollBar::handle:vertical { background: #bee3f8; border-radius: 5px; min-height: 24px; }
QScrollBar::handle:vertical:hover { background: #4299e1; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal { background: #f0f7ff; height: 10px; border-radius: 5px; }
QScrollBar::handle:horizontal { background: #bee3f8; border-radius: 5px; min-width: 24px; }
QScrollBar::handle:horizontal:hover { background: #4299e1; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

QGroupBox { color: #2b6cb0; border: 1px solid #bee3f8; border-radius: 6px; margin-top: 10px; padding-top: 6px; font-weight: bold; background: #f7fbff; }
QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 6px; color: #2b6cb0; background: #f7fbff; border-radius: 3px; }

QStatusBar {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #4299e1, stop:1 #3182ce);
    color: #fff; border-top: 1px solid #2b6cb0; font-size: 11px;
}
QStatusBar::item { border: none; }

QSplitter::handle { background: #bee3f8; }
QSplitter::handle:hover { background: #4299e1; }
QSplitter::handle:vertical {
    height: 6px;
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #bee3f8, stop:0.5 #f7fbff, stop:1 #bee3f8);
    border-top: 1px solid #90cdf4; border-bottom: 1px solid #90cdf4;
}

QCheckBox, QRadioButton { color: #1a202c; spacing: 6px; }
QCheckBox::indicator { width: 16px; height: 16px; border: 2px solid #bee3f8; border-radius: 3px; background: white; }
QCheckBox::indicator:checked { background: #4299e1; border-color: #3182ce; }
QCheckBox::indicator:hover { border-color: #4299e1; }
QRadioButton::indicator { width: 16px; height: 16px; border: 2px solid #bee3f8; border-radius: 8px; background: white; }
QRadioButton::indicator:checked { background: #4299e1; border-color: #3182ce; }

QProgressBar { background: #dbeafe; border: 1px solid #bee3f8; border-radius: 5px; text-align: center; color: #2b6cb0; font-weight: bold; height: 18px; }
QProgressBar::chunk { background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #3182ce, stop:1 #4299e1); border-radius: 5px; }

QToolTip { background: #1a365d; color: #e2e8f0; border: 1px solid #4299e1; border-radius: 6px; padding: 8px 12px; font-size: 12px; }
QLabel { color: #1a202c; }
QFrame[frameShape="4"], QFrame[frameShape="5"] { color: #bee3f8; }

QSpinBox, QDoubleSpinBox, QDateEdit, QTimeEdit, QDateTimeEdit { background: #fff; color: #1a202c; border: 1px solid #bee3f8; border-radius: 4px; padding: 4px 6px; }
QSpinBox:focus, QDateEdit:focus, QTimeEdit:focus { border: 2px solid #4299e1; }
QSpinBox::up-button, QSpinBox::down-button, QDateEdit::up-button, QDateEdit::down-button { background: #3182ce; border-radius: 2px; width: 16px; }

QDockWidget { color: #1a202c; }
QDockWidget::title { background: #2b6cb0; color: #fff; padding: 6px 8px; text-align: left; font-weight: bold; }
)";
    }

    static QString vaultGreen() {
        return R"(
/* ═══════════════════════════════════════════════════
   CfdmNote — Tema Vault Green (ativo com cofre aberto)
   Paleta: #1a4731 (dark) · #276749 (mid) · #38a169 (accent)
           #9ae6b4 (light) · #f0fff4 (very light)
   ═══════════════════════════════════════════════════ */

QMainWindow, QDialog, QWidget { background-color: #f0fff4; color: #1a202c; }

QMenuBar {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #276749, stop:1 #1a4731);
    color: #e2e8f0; border-bottom: 2px solid #38a169; padding: 1px 4px;
}
QMenuBar::item { background: transparent; padding: 5px 12px; border-radius: 4px; }
QMenuBar::item:selected, QMenuBar::item:pressed { background: #38a169; color: #fff; }

QMenu {
    background: #1c4532; color: #e2e8f0;
    border: 1px solid #38a169; border-radius: 6px; padding: 4px 0;
}
QMenu::item { padding: 6px 28px 6px 12px; border-radius: 3px; margin: 1px 4px; }
QMenu::item:selected { background: #38a169; color: white; }
QMenu::separator { height: 1px; background: #276749; margin: 4px 10px; }

QToolBar {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #276749, stop:1 #1a4731);
    border: none; spacing: 3px; padding: 2px 4px;
}
QToolButton { color: #e2e8f0; border: none; border-radius: 4px; padding: 4px 8px; }
QToolButton:hover { background: #276749; color: white; }
QToolButton:pressed, QToolButton:checked { background: #38a169; color: white; }

QTabWidget::pane { border: 1px solid #9ae6b4; background: #ffffff; border-radius: 0 4px 4px 4px; }
QTabBar::tab {
    background: #c6f6d5; color: #1a4731; border: 1px solid #9ae6b4;
    border-bottom: none; padding: 6px 16px;
    border-top-left-radius: 5px; border-top-right-radius: 5px; margin-right: 2px;
}
QTabBar::tab:selected { background: #1a4731; color: #fff; font-weight: bold; border-color: #1a4731; }
QTabBar::tab:hover:!selected { background: #9ae6b4; }

QTreeView, QListView, QTableView {
    background: #fff; alternate-background-color: #f0fff4;
    selection-background-color: #38a169; selection-color: #fff;
    border: 1px solid #9ae6b4; border-radius: 4px; outline: none;
}
QTreeView::item:hover, QListView::item:hover { background: #c6f6d5; color: #1a4731; }
QTreeView::item:selected, QListView::item:selected { background: #38a169; color: white; }

QHeaderView::section {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #276749, stop:1 #1a4731);
    color: #e2e8f0; border: none; border-right: 1px solid #38a169;
    padding: 6px 10px; font-weight: bold; font-size: 11px;
}
QHeaderView::section:hover { background: #38a169; }

QLineEdit, QPlainTextEdit, QTextEdit {
    background: #fff; color: #1a202c; border: 1px solid #9ae6b4;
    border-radius: 4px; padding: 4px 8px;
    selection-background-color: #38a169; selection-color: white;
}
QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus { border: 2px solid #38a169; }

QPushButton {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #38a169, stop:1 #276749);
    color: #fff; border: none; border-radius: 5px;
    padding: 6px 16px; font-weight: bold; font-size: 12px;
}
QPushButton:hover { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #48bb78, stop:1 #38a169); }
QPushButton:pressed { background: #1a4731; }
QPushButton:disabled { background: #9ae6b4; color: #e2e8f0; }
QPushButton:flat { background: transparent; color: #276749; border: 1px solid #9ae6b4; font-weight: normal; }
QPushButton:flat:hover { background: #c6f6d5; border-color: #38a169; }

QComboBox {
    background: #fff; color: #1a202c; border: 1px solid #9ae6b4;
    border-radius: 4px; padding: 4px 32px 4px 8px;
}
QComboBox::drop-down { background: #276749; width: 24px; border-left: 1px solid #9ae6b4; border-top-right-radius: 4px; border-bottom-right-radius: 4px; }
QComboBox QAbstractItemView { background: #1c4532; color: #e2e8f0; selection-background-color: #38a169; border: 1px solid #38a169; }

QScrollBar:vertical { background: #f0f4f0; width: 10px; border-radius: 5px; }
QScrollBar::handle:vertical { background: #9ae6b4; border-radius: 5px; min-height: 24px; }
QScrollBar::handle:vertical:hover { background: #38a169; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal { background: #f0f4f0; height: 10px; border-radius: 5px; }
QScrollBar::handle:horizontal { background: #9ae6b4; border-radius: 5px; min-width: 24px; }
QScrollBar::handle:horizontal:hover { background: #38a169; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

QGroupBox {
    color: #1a4731; border: 1px solid #9ae6b4; border-radius: 6px;
    margin-top: 10px; padding-top: 6px; font-weight: bold; background: #f0fff4;
}
QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 6px; color: #1a4731; }

QStatusBar {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #276749, stop:1 #1a4731);
    color: #e2e8f0; border-top: 1px solid #38a169; font-size: 11px;
}
QStatusBar::item { border: none; }

QSplitter::handle { background: #9ae6b4; }
QSplitter::handle:hover { background: #38a169; }
QSplitter::handle:vertical {
    height: 6px;
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #9ae6b4, stop:0.5 #f0fff4, stop:1 #9ae6b4);
    border-top: 1px solid #68d391; border-bottom: 1px solid #68d391;
}

QCheckBox::indicator { width: 16px; height: 16px; border: 2px solid #9ae6b4; border-radius: 3px; background: white; }
QCheckBox::indicator:checked { background: #38a169; border-color: #276749; }
QRadioButton::indicator { width: 16px; height: 16px; border: 2px solid #9ae6b4; border-radius: 8px; background: white; }
QRadioButton::indicator:checked { background: #38a169; border-color: #276749; }

QProgressBar {
    background: #c6f6d5; border: 1px solid #9ae6b4; border-radius: 5px;
    text-align: center; color: #1a4731; font-weight: bold; height: 18px;
}
QProgressBar::chunk {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #276749, stop:1 #38a169);
    border-radius: 5px;
}

QToolTip {
    background: #1a4731; color: #e2e8f0;
    border: 1px solid #38a169; border-radius: 6px; padding: 8px 12px; font-size: 12px;
}

QLabel { color: #1a202c; }
QFrame[frameShape="4"], QFrame[frameShape="5"] { color: #9ae6b4; }

QSpinBox, QDoubleSpinBox, QDateEdit, QTimeEdit, QDateTimeEdit {
    background: #fff; color: #1a202c; border: 1px solid #9ae6b4;
    border-radius: 4px; padding: 4px 6px;
}
QSpinBox:focus, QDateEdit:focus, QTimeEdit:focus { border: 2px solid #38a169; }
QSpinBox::up-button, QSpinBox::down-button { background: #276749; border-radius: 2px; width: 16px; }
)";
    }

    static QString navyBlue() {
        return R"(
/* ═══════════════════════════════════════════════════
   CfdmNote — Tema Navy Blue
   Toolbar clara (#4a86c8) para ícones pretos serem visíveis.
   Paleta: #1a365d (dark) · #2c5282 (mid) · #3182ce (accent)
           #4a86c8 (toolbar) · #bee3f8 (light) · #ebf8ff (very light)
   ═══════════════════════════════════════════════════ */

/* ── Base ─────────────────────────────────────────── */
QMainWindow, QDialog, QWidget {
    background-color: #f0f7ff;
    color: #1a202c;
}

/* ── Menu Bar ──────────────────────────────────────── */
QMenuBar {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #4a86c8, stop:1 #3a6cb0);
    color: #ffffff;
    border-bottom: 2px solid #2c5282;
    padding: 1px 4px;
}
QMenuBar::item {
    background: transparent;
    padding: 5px 12px;
    border-radius: 4px;
    color: #ffffff;
}
QMenuBar::item:selected, QMenuBar::item:pressed {
    background: #2c5282;
    color: #ffffff;
}

/* ── Menus ─────────────────────────────────────────── */
QMenu {
    background: #1e3a5f;
    color: #e2e8f0;
    border: 1px solid #3182ce;
    border-radius: 6px;
    padding: 4px 0;
}
QMenu::item {
    padding: 6px 28px 6px 12px;
    border-radius: 3px;
    margin: 1px 4px;
}
QMenu::item:selected {
    background: #3182ce;
    color: white;
}
QMenu::separator {
    height: 1px;
    background: #2c5282;
    margin: 4px 10px;
}
QMenu::icon { padding-left: 6px; }

/* ── Toolbar ───────────────────────────────────────── */
/* Fundo navy escuro — texto e ícones brancos */
QToolBar {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #1e3a5f, stop:1 #17304f);
    border: none;
    border-bottom: 2px solid #2c5282;
    spacing: 3px;
    padding: 2px 4px;
}
QToolBar::separator {
    background: #4a86c8;
    width: 1px;
    margin: 4px 3px;
}
QToolButton {
    color: #ffffff;
    border: none;
    border-radius: 4px;
    padding: 4px 8px;
}
QToolButton:hover {
    background: #2c5282;
    color: #ffffff;
    border: 1px solid #4a86c8;
}
QToolButton:pressed, QToolButton:checked {
    background: #3182ce;
    color: white;
}
QToolButton[popupMode="1"] {
    padding-right: 16px;
}

/* ── Abas ──────────────────────────────────────────── */
QTabWidget::pane {
    border: 1px solid #90cdf4;
    background: #ffffff;
    border-radius: 0 4px 4px 4px;
}
QTabBar::tab {
    background: #dbeafe;
    color: #1a365d;
    border: 1px solid #90cdf4;
    border-bottom: none;
    padding: 6px 16px;
    border-top-left-radius: 5px;
    border-top-right-radius: 5px;
    margin-right: 2px;
    font-size: 12px;
}
QTabBar::tab:selected {
    background: #1a365d;
    color: #ffffff;
    font-weight: bold;
    border-color: #1a365d;
}
QTabBar::tab:hover:!selected {
    background: #bee3f8;
    color: #1a365d;
}

/* ── Listas / Árvores ──────────────────────────────── */
QTreeView, QListView, QTableView {
    background: #ffffff;
    alternate-background-color: #f0f7ff;
    selection-background-color: #3182ce;
    selection-color: #ffffff;
    border: 1px solid #90cdf4;
    border-radius: 4px;
    outline: none;
    gridline-color: #dbeafe;
}
QTreeView::item, QListView::item {
    padding: 3px 4px;
    border-radius: 3px;
}
QTreeView::item:hover, QListView::item:hover {
    background: #dbeafe;
    color: #1a365d;
}
QTreeView::item:selected, QListView::item:selected {
    background: #3182ce;
    color: white;
}
QTreeView::branch:has-children:!has-siblings:closed,
QTreeView::branch:closed:has-children:has-siblings {
    border-image: none;
}

/* ── Cabeçalhos ────────────────────────────────────── */
QHeaderView {
    background: #1a365d;
}
QHeaderView::section {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #2c5282, stop:1 #1a365d);
    color: #e2e8f0;
    border: none;
    border-right: 1px solid #3182ce;
    padding: 6px 10px;
    font-weight: bold;
    font-size: 11px;
}
QHeaderView::section:hover {
    background: #3182ce;
}
QHeaderView::section:first { border-top-left-radius: 4px; }
QHeaderView::section:last  { border-top-right-radius: 4px; border-right: none; }

/* ── Campos de texto ───────────────────────────────── */
QLineEdit, QPlainTextEdit, QTextEdit {
    background: #ffffff;
    color: #1a202c;
    border: 1px solid #90cdf4;
    border-radius: 4px;
    padding: 4px 8px;
    selection-background-color: #3182ce;
    selection-color: white;
}
QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus {
    border: 2px solid #3182ce;
    background: #f8fbff;
}
QLineEdit:disabled, QPlainTextEdit:disabled {
    background: #f0f4f8;
    color: #a0aec0;
    border-color: #dbeafe;
}

/* ── Botões ────────────────────────────────────────── */
QPushButton {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #3182ce, stop:1 #2c5282);
    color: #ffffff;
    border: none;
    border-radius: 5px;
    padding: 6px 16px;
    font-weight: bold;
    font-size: 12px;
}
QPushButton:hover {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #4299e1, stop:1 #3182ce);
}
QPushButton:pressed {
    background: #1a365d;
}
QPushButton:disabled {
    background: #90cdf4;
    color: #e2e8f0;
}
QPushButton:flat {
    background: transparent;
    color: #2c5282;
    border: 1px solid #90cdf4;
    font-weight: normal;
}
QPushButton:flat:hover {
    background: #dbeafe;
    border-color: #3182ce;
}

/* ── ComboBox ──────────────────────────────────────── */
QComboBox {
    background: #ffffff;
    color: #1a202c;
    border: 1px solid #90cdf4;
    border-radius: 4px;
    padding: 4px 32px 4px 8px;
}
QComboBox:focus { border: 2px solid #3182ce; }
QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: right center;
    background: #2c5282;
    width: 24px;
    border-left: 1px solid #90cdf4;
    border-top-right-radius: 4px;
    border-bottom-right-radius: 4px;
}
QComboBox QAbstractItemView {
    background: #1e3a5f;
    color: #e2e8f0;
    selection-background-color: #3182ce;
    border: 1px solid #3182ce;
}

/* ── Scrollbars ────────────────────────────────────── */
QScrollBar:vertical {
    background: #f0f4f8;
    width: 10px;
    border-radius: 5px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: #90cdf4;
    border-radius: 5px;
    min-height: 24px;
}
QScrollBar::handle:vertical:hover { background: #3182ce; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal {
    background: #f0f4f8;
    height: 10px;
    border-radius: 5px;
}
QScrollBar::handle:horizontal {
    background: #90cdf4;
    border-radius: 5px;
    min-width: 24px;
}
QScrollBar::handle:horizontal:hover { background: #3182ce; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

/* ── GroupBox ──────────────────────────────────────── */
QGroupBox {
    color: #1a365d;
    border: 1px solid #90cdf4;
    border-radius: 6px;
    margin-top: 10px;
    padding-top: 6px;
    font-weight: bold;
    background: #f8fbff;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 10px;
    padding: 0 6px;
    color: #1a365d;
    background: #f0f7ff;
    border-radius: 3px;
}

/* ── Status Bar ────────────────────────────────────── */
QStatusBar {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #1e4080, stop:1 #1a365d);
    color: #e2e8f0;
    border-top: 1px solid #3182ce;
    font-size: 11px;
}
QStatusBar::item { border: none; }

/* ── Splitter ──────────────────────────────────────── */
QSplitter::handle {
    background: #bee3f8;
}
QSplitter::handle:hover {
    background: #3182ce;
}
QSplitter::handle:vertical {
    height: 6px;
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
        stop:0 #bee3f8, stop:0.5 #ebf8ff, stop:1 #bee3f8);
    border-top: 1px solid #90cdf4;
    border-bottom: 1px solid #90cdf4;
}

/* ── CheckBox / RadioButton ────────────────────────── */
QCheckBox, QRadioButton { color: #1a202c; spacing: 6px; }
QCheckBox::indicator {
    width: 16px; height: 16px;
    border: 2px solid #90cdf4;
    border-radius: 3px;
    background: white;
}
QCheckBox::indicator:checked {
    background: #3182ce;
    border-color: #2c5282;
}
QCheckBox::indicator:hover { border-color: #3182ce; }
QRadioButton::indicator {
    width: 16px; height: 16px;
    border: 2px solid #90cdf4;
    border-radius: 8px;
    background: white;
}
QRadioButton::indicator:checked {
    background: #3182ce;
    border-color: #2c5282;
}

/* ── ProgressBar ───────────────────────────────────── */
QProgressBar {
    background: #dbeafe;
    border: 1px solid #90cdf4;
    border-radius: 5px;
    text-align: center;
    color: #1a365d;
    font-weight: bold;
    height: 18px;
}
QProgressBar::chunk {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
        stop:0 #2c5282, stop:1 #3182ce);
    border-radius: 5px;
}

/* ── Tooltip ───────────────────────────────────────── */
QToolTip {
    background: #1a365d;
    color: #e2e8f0;
    border: 1px solid #3182ce;
    border-radius: 6px;
    padding: 8px 12px;
    font-size: 12px;
}

/* ── Labels ────────────────────────────────────────── */
QLabel { color: #1a202c; }

/* ── Separadores ───────────────────────────────────── */
QFrame[frameShape="4"], QFrame[frameShape="5"] {
    color: #90cdf4;
}

/* ── SpinBox / DateEdit ────────────────────────────── */
QSpinBox, QDoubleSpinBox, QDateEdit, QTimeEdit, QDateTimeEdit {
    background: #ffffff;
    color: #1a202c;
    border: 1px solid #90cdf4;
    border-radius: 4px;
    padding: 4px 6px;
}
QSpinBox:focus, QDateEdit:focus, QTimeEdit:focus {
    border: 2px solid #3182ce;
}
QSpinBox::up-button, QSpinBox::down-button,
QDateEdit::up-button, QDateEdit::down-button {
    background: #2c5282;
    border-radius: 2px;
    width: 16px;
}
QSpinBox::up-button:hover, QSpinBox::down-button:hover {
    background: #3182ce;
}

/* ── Dock / Frame ──────────────────────────────────── */
QDockWidget {
    color: #1a202c;
    titlebar-close-icon: none;
    titlebar-normal-icon: none;
}
QDockWidget::title {
    background: #1a365d;
    color: #e2e8f0;
    padding: 6px 8px;
    text-align: left;
    font-weight: bold;
}
)";
    }
};
