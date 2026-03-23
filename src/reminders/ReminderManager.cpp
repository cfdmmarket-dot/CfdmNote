#include "ReminderManager.h"

#include <QMessageBox>
#include <QPushButton>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QMainWindow>

#include "core/Database.h"
#include "core/Group.h"
#include "core/Entry.h"
#include "core/EntryAttributes.h"

static const QString KEY_REMINDER = "CfdmNote::Reminder";

// ─── Singleton ────────────────────────────────────────────────────────────────

ReminderManager& ReminderManager::instance()
{
    static ReminderManager inst;
    return inst;
}

ReminderManager::ReminderManager()
{
    m_timer = new QTimer(this);
    m_timer->setInterval(60 * 1000);  // verifica a cada 60 segundos
    connect(m_timer, &QTimer::timeout, this, &ReminderManager::onTimer);
    m_timer->start();
}

// ─── Registra banco ativo ─────────────────────────────────────────────────────

void ReminderManager::setDatabase(const QSharedPointer<Database>& db, QWidget* mainWindow)
{
    m_db      = db;
    m_mainWin = mainWindow;
    // Verifica imediatamente ao abrir banco (não espera 60s)
    QTimer::singleShot(2000, this, &ReminderManager::checkNow);
}

// ─── Verificação periódica ────────────────────────────────────────────────────

void ReminderManager::onTimer()  { checkNow(); }

void ReminderManager::checkNow()
{
    if (!m_db) return;

    const QDateTime now = QDateTime::currentDateTime();
    const auto entries  = m_db->rootGroup()->entriesRecursive();

    for (auto* entry : entries) {
        if (!entry->attributes()->contains(KEY_REMINDER)) continue;

        const QByteArray json = entry->attributes()->value(KEY_REMINDER).toUtf8();
        QJsonObject obj = QJsonDocument::fromJson(json).object();
        if (obj.isEmpty()) continue;

        if (!obj.value("enabled").toBool(true)) continue;

        const QDateTime dt = QDateTime::fromString(
            obj.value("datetime").toString(), Qt::ISODate);
        if (!dt.isValid() || dt > now) continue;

        // Lembrete disparado
        FiredReminder r;
        r.entry    = entry;
        r.obj      = obj;
        r.title    = entry->title();
        r.message  = obj.value("message").toString();
        r.datetime = dt;
        handleFired(r);

        // Recarrega obj pois handleFired pode ter modificado
        break;  // uma notificação por vez para não sobrecarregar
    }
}

// ─── Dispara notificação ──────────────────────────────────────────────────────

void ReminderManager::handleFired(const FiredReminder& r)
{
    // Eleva a janela principal
    if (m_mainWin) {
        m_mainWin->raise();
        m_mainWin->activateWindow();
    }

    // Constrói o diálogo de notificação
    QMessageBox msgBox(m_mainWin);
    msgBox.setWindowTitle(tr("⏰  Lembrete CfdmNote"));
    msgBox.setIcon(QMessageBox::Information);

    const QString body = r.message.isEmpty()
        ? tr("<b>%1</b>").arg(r.title.toHtmlEscaped())
        : tr("<b>%1</b><br><br>%2").arg(r.title.toHtmlEscaped(),
                                         r.message.toHtmlEscaped());
    msgBox.setText(body);
    msgBox.setTextFormat(Qt::RichText);

    const QString rec = r.obj.value("recurrence").toString("once");
    const QString recLabel = [&]() -> QString {
        if (rec == "daily")   return tr(" (repete diariamente)");
        if (rec == "weekly")  return tr(" (repete semanalmente)");
        if (rec == "monthly") return tr(" (repete mensalmente)");
        if (rec == "yearly")  return tr(" (repete anualmente)");
        return {};
    }();
    msgBox.setInformativeText(
        tr("Entrada: <i>%1</i>%2")
            .arg(r.title.toHtmlEscaped(), recLabel));

    // Botões
    auto* btnOk      = msgBox.addButton(tr("OK"),        QMessageBox::AcceptRole);
    auto* btnSnooze1 = msgBox.addButton(tr("Adiar 1h"),  QMessageBox::ActionRole);
    auto* btnSnooze8 = msgBox.addButton(tr("Adiar 8h"),  QMessageBox::ActionRole);
    auto* btnWA      = msgBox.addButton(tr("WhatsApp"),  QMessageBox::ActionRole);
    auto* btnTg      = msgBox.addButton(tr("Telegram"),  QMessageBox::ActionRole);
    Q_UNUSED(btnOk)

    msgBox.exec();

    auto* clicked = msgBox.clickedButton();

    if (clicked == btnSnooze1) {
        reschedule(r.entry, r.obj, 60);
        return;
    }
    if (clicked == btnSnooze8) {
        reschedule(r.entry, r.obj, 8 * 60);
        return;
    }
    if (clicked == btnWA || clicked == btnTg) {
        const QString text = r.message.isEmpty()
            ? tr("Lembrete: %1").arg(r.title)
            : tr("Lembrete: %1\n%2").arg(r.title, r.message);
        const QString encoded = QUrl::toPercentEncoding(text);
        if (clicked == btnWA) {
            QDesktopServices::openUrl(
                QUrl(QString("https://api.whatsapp.com/send?text=%1").arg(encoded)));
        } else {
            QDesktopServices::openUrl(
                QUrl(QString("https://t.me/share/url?url=%1&text=").arg(encoded)));
        }
    }

    // OK ou Share: avança para próxima ocorrência ou desativa
    reschedule(r.entry, r.obj, 0);
}

// ─── Reagenda ou desativa ─────────────────────────────────────────────────────

void ReminderManager::reschedule(Entry* entry, QJsonObject obj, int snoozeMinutes)
{
    if (!m_db) return;

    if (snoozeMinutes > 0) {
        // Adia: next = now + snooze
        const QDateTime next = QDateTime::currentDateTime().addSecs(snoozeMinutes * 60);
        obj["datetime"] = next.toString(Qt::ISODate);
        obj["enabled"]  = true;
    } else {
        const QString rec = obj.value("recurrence").toString("once");
        if (rec == "once") {
            obj["enabled"] = false;
        } else {
            obj["datetime"] = nextDatetime(obj);
            obj["enabled"]  = true;
        }
    }

    entry->attributes()->set(KEY_REMINDER,
        QJsonDocument(obj).toJson(QJsonDocument::Compact), false);

    QString err;
    m_db->save(Database::Atomic, {}, &err);
}

// ─── Calcula próxima ocorrência ───────────────────────────────────────────────

QString ReminderManager::nextDatetime(const QJsonObject& obj) const
{
    QDateTime dt = QDateTime::fromString(obj.value("datetime").toString(), Qt::ISODate);
    if (!dt.isValid()) return QDateTime::currentDateTime().addDays(1).toString(Qt::ISODate);

    const QString rec = obj.value("recurrence").toString("once");

    if      (rec == "daily")   dt = dt.addDays(1);
    else if (rec == "weekly")  dt = dt.addDays(7);
    else if (rec == "monthly") dt = dt.addMonths(1);
    else if (rec == "yearly")  dt = dt.addYears(1);

    // Se a próxima data ainda está no passado (app ficou offline), avança até ficar no futuro
    const QDateTime now = QDateTime::currentDateTime();
    while (dt <= now) {
        if      (rec == "daily")   dt = dt.addDays(1);
        else if (rec == "weekly")  dt = dt.addDays(7);
        else if (rec == "monthly") dt = dt.addMonths(1);
        else if (rec == "yearly")  dt = dt.addYears(1);
        else break;
    }

    return dt.toString(Qt::ISODate);
}
