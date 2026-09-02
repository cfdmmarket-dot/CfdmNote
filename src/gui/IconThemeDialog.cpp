#include "IconThemeDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QFrame>
#include <QStandardPaths>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QSet>

// ─────────────────────────────────────────────────────────────────────────────
QString IconThemeDialog::userIconsDir()
{
    const QString dir = QStandardPaths::writableLocation(
                            QStandardPaths::AppConfigLocation) + "/icons";
    QDir().mkpath(dir);
    return dir;
}

QIcon IconThemeDialog::icon(const QString& name, const QSize&)
{
    const QString userFile = userIconsDir() + "/" + name;
    if (QFile::exists(userFile)) {
        QIcon ic(userFile);
        if (!ic.isNull()) return ic;
    }
    QIcon ic(":/icons/" + name);
    return ic.isNull() ? QIcon::fromTheme("image-missing") : ic;
}

bool IconThemeDialog::replaceIcon(const QString& iconName, QWidget* parent)
{
    const QString src = QFileDialog::getOpenFileName(
        parent,
        QObject::tr("Escolher novo ícone para '%1'").arg(iconName),
        QDir::homePath(),
        QObject::tr("Imagens (*.png *.jpg *.svg *.ico *.bmp *.gif)"));
    if (src.isEmpty()) return false;
    const QString dst = userIconsDir() + "/" + iconName;
    QFile::remove(dst);
    if (!QFile::copy(src, dst)) {
        QMessageBox::warning(parent, QObject::tr("Erro"),
            QObject::tr("Não foi possível copiar:\n%1").arg(dst));
        return false;
    }
    return true;
}

// ─── Reaplica overrides de QActions ─────────────────────────────────────────
void IconThemeDialog::applyActionOverrides(QMainWindow* window)
{
    if (!window) return;
    QSettings cfg;
    cfg.beginGroup("cfdmnote/icon_action_overrides");
    const QStringList keys = cfg.childKeys();
    if (keys.isEmpty()) { cfg.endGroup(); return; }

    // Recolhe todos os QActions da janela
    QList<QAction*> all;
    for (auto* m : window->findChildren<QMenu*>())
        all << m->actions();
    for (auto* t : window->findChildren<QToolBar*>())
        all << t->actions();

    for (auto* action : qAsConst(all)) {
        if (action->text().isEmpty()) continue;
        const QString key = action->text().simplified().replace(" ", "_")
                                .replace(QRegExp("[^A-Za-z0-9_]"), "");
        if (key.isEmpty()) continue;
        const QString path = cfg.value(key).toString();
        if (!path.isEmpty() && QFile::exists(path))
            action->setIcon(QIcon(path));
    }
    cfg.endGroup();
}

// ─── Construtor ──────────────────────────────────────────────────────────────
IconThemeDialog::IconThemeDialog(QApplication* app, QMainWindow* mainWindow,
                                   QWidget* parent)
    : QDialog(parent), m_app(app), m_mainWindow(mainWindow)
{
    setWindowTitle(tr("🖼 Gerenciar Ícones do CfdmNote"));
    setMinimumSize(600, 640);
    buildUI();
}

void IconThemeDialog::buildUI()
{
    auto* root = new QVBoxLayout(this);

    auto* hdr = new QLabel(tr(
        "<b>Substitua qualquer ícone do CfdmNote.</b><br>"
        "<small>Ícones personalizados ficam em: <code>%1</code><br>"
        "Reinicie o app para aplicar em todos os lugares.</small>")
        .arg(userIconsDir()));
    hdr->setWordWrap(true);
    hdr->setMargin(6);
    root->addWidget(hdr);

    auto* tabs = new QTabWidget();

    // ── Aba 1: Ícones próprios do CfdmNote ──────────────────────────────────
    auto* tabCfdm = new QWidget();
    buildTabCfdmIcons(tabCfdm);
    tabs->addTab(tabCfdm, tr("💎 Ícones CfdmNote"));

    // ── Aba 2: Ícones dos menus/toolbar herdados ─────────────────────────────
    auto* tabActions = new QWidget();
    buildTabActionIcons(tabActions);
    tabs->addTab(tabActions, tr("📋 Menus e Toolbar"));

    root->addWidget(tabs, 1);

    // Rodapé
    auto* btns = new QHBoxLayout();
    auto* btnResetAll = new QPushButton(tr("↺ Restaurar Todos"));
    connect(btnResetAll, &QPushButton::clicked, this, [this] {
        if (QMessageBox::question(this, tr("Restaurar"),
            tr("Remover todos os ícones personalizados?")) != QMessageBox::Yes) return;
        QDir d(userIconsDir());
        for (const auto& f : d.entryList(QDir::Files)) d.remove(f);
        QSettings cfg;
        cfg.remove("cfdmnote/icon_action_overrides");
        QMessageBox::information(this, tr("Restaurado"),
            tr("Ícones restaurados. Reinicie o app para ver o efeito."));
    });
    btns->addWidget(btnResetAll);
    btns->addStretch();
    auto* btnClose = new QPushButton(tr("Fechar"));
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    btns->addWidget(btnClose);
    root->addLayout(btns);
}

// ─── Aba 1: ícones CfdmNote (QRC) ────────────────────────────────────────────
void IconThemeDialog::buildTabCfdmIcons(QWidget* tab)
{
    auto* lay = new QVBoxLayout(tab);
    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    auto* cont = new QWidget();
    auto* vlay = new QVBoxLayout(cont);
    vlay->setSpacing(2);

    struct IconEntry { QString name; QString desc; };
    const QList<IconEntry> entries = {
        {"cfdmnote_256.png", tr("App — 256px (janela e launcher)")},
        {"cfdmnote_128.png", tr("App — 128px")},
        {"cfdmnote_64.png",  tr("App — 64px")},
        {"cfdmnote_48.png",  tr("App — 48px")},
        {"cfdmnote_32.png",  tr("App — 32px")},
        {"cfdmnote_16.png",  tr("App — 16px (barra de tarefas)")},
        {"diamond_toolbar_32.png", tr("Botão IA toolbar — 32px")},
        {"diamond_toolbar_20.png", tr("Botão IA toolbar — 20px")},
        {"diamond_ai_32.png",      tr("IA — menus e dialogs (32px)")},
        {"diamond_ai_16.png",      tr("IA — menus pequenos (16px)")},
        {"diamond_tray_16.png",    tr("System tray — 16px")},
        {"diamond_flat_16.png",    tr("Diamante flat — 16px")},
        {"diamond_logo_animated.gif", tr("Logo animado — 256px GIF")},
        {"diamond_logo_128.gif",      tr("Logo animado — 128px GIF")},
    };

    for (const auto& e : entries)
        addCfdmIconRow(vlay, e.name, e.desc);

    vlay->addStretch();
    scroll->setWidget(cont);
    lay->addWidget(scroll);
}

void IconThemeDialog::addCfdmIconRow(QVBoxLayout* lay,
                                      const QString& name,
                                      const QString& description)
{
    auto* row = new QWidget();
    auto* hl = new QHBoxLayout(row);
    hl->setContentsMargins(4, 2, 4, 2);

    auto* imgLabel = new QLabel();
    imgLabel->setFixedSize(40, 40);
    imgLabel->setAlignment(Qt::AlignCenter);
    imgLabel->setFrameShape(QFrame::StyledPanel);
    const QIcon ic = icon(name);
    imgLabel->setPixmap(ic.isNull() ? QPixmap() : ic.pixmap(36, 36));
    hl->addWidget(imgLabel);

    auto* lbl = new QLabel(QStringLiteral("<b>%1</b><br><small>%2</small>")
                           .arg(name, description));
    lbl->setTextFormat(Qt::RichText);
    lbl->setWordWrap(true);
    hl->addWidget(lbl, 1);

    const bool isCustom = QFile::exists(userIconsDir() + "/" + name);
    auto* statusLbl = new QLabel(isCustom ? tr("✏ Custom") : tr("● Padrão"));
    statusLbl->setStyleSheet(isCustom ? "color:#22c55e;font-size:10px;"
                                      : "color:#64748b;font-size:10px;");
    hl->addWidget(statusLbl);

    auto* btnTrocar = new QPushButton(tr("Trocar"));
    btnTrocar->setFixedWidth(70);
    connect(btnTrocar, &QPushButton::clicked, this,
            [this, name, imgLabel, statusLbl] {
        if (!replaceIcon(name, this)) return;
        imgLabel->setPixmap(icon(name).pixmap(36, 36));
        statusLbl->setText(tr("✏ Custom"));
        statusLbl->setStyleSheet("color:#22c55e;font-size:10px;");
        QMessageBox::information(this, tr("Ícone substituído"),
            tr("<b>%1</b> atualizado.<br>Reinicie para aplicar.").arg(name));
    });
    hl->addWidget(btnTrocar);

    auto* btnRestore = new QPushButton(tr("Restaurar"));
    btnRestore->setFixedWidth(75);
    btnRestore->setEnabled(isCustom);
    connect(btnRestore, &QPushButton::clicked, this,
            [this, name, imgLabel, statusLbl, btnRestore] {
        QFile::remove(userIconsDir() + "/" + name);
        imgLabel->setPixmap(icon(name).pixmap(36, 36));
        statusLbl->setText(tr("● Padrão"));
        statusLbl->setStyleSheet("color:#64748b;font-size:10px;");
        btnRestore->setEnabled(false);
    });
    hl->addWidget(btnRestore);

    lay->addWidget(row);
    auto* sep = new QFrame(); sep->setFrameShape(QFrame::HLine);
    lay->addWidget(sep);
}

// ─── Aba 2: ícones de QActions (menus e toolbar do KeePassXC) ────────────────
void IconThemeDialog::buildTabActionIcons(QWidget* tab)
{
    auto* lay = new QVBoxLayout(tab);

    auto* hint = new QLabel(tr(
        "<small>🔍 Todos os ícones ativos nos <b>menus e toolbar</b> do CfdmNote.<br>"
        "Clique <b>Trocar</b> para substituir — o novo ícone é aplicado imediatamente.</small>"));
    hint->setWordWrap(true);
    hint->setMargin(4);
    lay->addWidget(hint);

    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    auto* cont = new QWidget();
    auto* vlay = new QVBoxLayout(cont);
    vlay->setSpacing(2);

    if (!m_mainWindow) {
        vlay->addWidget(new QLabel(tr("(janela principal não disponível)")));
        scroll->setWidget(cont);
        lay->addWidget(scroll);
        return;
    }

    // Coleta todos os QActions únicos com ícone e texto
    QSet<QAction*> seen;
    QList<QAction*> all;
    for (auto* m : m_mainWindow->findChildren<QMenu*>())
        for (auto* a : m->actions()) {
            if (!seen.contains(a) && !a->isSeparator() && !a->icon().isNull()) {
                seen.insert(a); all << a;
            }
        }
    for (auto* t : m_mainWindow->findChildren<QToolBar*>())
        for (auto* a : t->actions()) {
            if (!seen.contains(a) && !a->isSeparator() && !a->icon().isNull()) {
                seen.insert(a); all << a;
            }
        }

    if (all.isEmpty()) {
        vlay->addWidget(new QLabel(
            tr("Nenhum ícone de menu/toolbar encontrado.\n"
               "Abra um banco e reabra este dialog.")));
        scroll->setWidget(cont);
        lay->addWidget(scroll);
        return;
    }

    QSettings cfg;

    for (auto* action : qAsConst(all)) {
        const QString cleanText = action->text().remove('&');
        const QString key = cleanText.simplified().replace(" ", "_")
                                .replace(QRegExp("[^A-Za-z0-9_]"), "");
        if (key.isEmpty()) continue;

        auto* row = new QWidget();
        auto* hl = new QHBoxLayout(row);
        hl->setContentsMargins(4, 2, 4, 2);

        // Preview do ícone atual
        auto* imgLabel = new QLabel();
        imgLabel->setFixedSize(32, 32);
        imgLabel->setAlignment(Qt::AlignCenter);
        imgLabel->setFrameShape(QFrame::StyledPanel);
        imgLabel->setPixmap(action->icon().pixmap(28, 28));
        hl->addWidget(imgLabel);

        // Texto da ação
        auto* lbl = new QLabel(cleanText.isEmpty() ? tr("(sem texto)") : cleanText);
        lbl->setMinimumWidth(180);
        hl->addWidget(lbl, 1);

        // Status
        const bool hasOverride = cfg.contains("cfdmnote/icon_action_overrides/" + key);
        auto* statusLbl = new QLabel(hasOverride ? tr("✏ Custom") : tr("● Padrão"));
        statusLbl->setStyleSheet(hasOverride ? "color:#22c55e;font-size:10px;"
                                             : "color:#64748b;font-size:10px;");
        hl->addWidget(statusLbl);

        // Botão Trocar
        auto* btnTrocar = new QPushButton(tr("Trocar"));
        btnTrocar->setFixedWidth(70);
        connect(btnTrocar, &QPushButton::clicked, this,
                [this, action, key, imgLabel, statusLbl] {
            const QString src = QFileDialog::getOpenFileName(
                this, tr("Escolher ícone"),
                QDir::homePath(),
                tr("Imagens (*.png *.jpg *.svg *.ico *.bmp)"));
            if (src.isEmpty()) return;
            QSettings cfg2;
            cfg2.setValue("cfdmnote/icon_action_overrides/" + key, src);
            const QIcon newIc(src);
            action->setIcon(newIc);
            imgLabel->setPixmap(newIc.pixmap(28, 28));
            statusLbl->setText(tr("✏ Custom"));
            statusLbl->setStyleSheet("color:#22c55e;font-size:10px;");
        });
        hl->addWidget(btnTrocar);

        // Botão Restaurar
        auto* btnRestore = new QPushButton(tr("Restaurar"));
        btnRestore->setFixedWidth(75);
        btnRestore->setEnabled(hasOverride);
        const QIcon origIcon = action->icon();
        connect(btnRestore, &QPushButton::clicked, this,
                [action, key, imgLabel, statusLbl, btnRestore, origIcon] {
            QSettings cfg2;
            cfg2.remove("cfdmnote/icon_action_overrides/" + key);
            // Não temos o ícone original em memória — avisa o usuário
            statusLbl->setText(tr("● Padrão"));
            statusLbl->setStyleSheet("color:#64748b;font-size:10px;");
            btnRestore->setEnabled(false);
            Q_UNUSED(action); Q_UNUSED(imgLabel); Q_UNUSED(origIcon);
            QMessageBox::information(nullptr, tr("Ícone restaurado"),
                tr("Reinicie o app para voltar ao ícone original."));
        });
        hl->addWidget(btnRestore);

        vlay->addWidget(row);
        auto* sep = new QFrame(); sep->setFrameShape(QFrame::HLine);
        vlay->addWidget(sep);
    }

    vlay->addStretch();
    scroll->setWidget(cont);
    lay->addWidget(scroll, 1);
}
