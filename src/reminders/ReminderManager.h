#pragma once

#include <QObject>
#include <QTimer>
#include <QSharedPointer>
#include <QJsonObject>
#include <QDateTime>
#include <QList>

class Database;
class DatabaseTabWidget;
class QWidget;
class Entry;

/**
 * ReminderManager — Singleton que verifica lembretes a cada 60 segundos.
 *
 * Quando um lembrete dispara:
 *   1. Eleva a janela principal
 *   2. Mostra NotificationDialog com título/mensagem da entrada
 *   3. Oferece: OK, Adiar 1h, Adiar 8h, WhatsApp, Telegram
 *   4. Se recorrente: avança a data e salva. Se "once": desativa.
 *
 * Registro: chame setDatabase() ao abrir um banco e quando trocar de banco ativo.
 */
class ReminderManager : public QObject
{
    Q_OBJECT

public:
    static ReminderManager& instance();

    // Define o banco ativo e a janela principal para notificações
    void setDatabase(const QSharedPointer<Database>& db, QWidget* mainWindow);

    // Verifica imediatamente (chamado pelo timer e na inicialização)
    void checkNow();

private slots:
    void onTimer();

private:
    ReminderManager();
    ~ReminderManager() = default;
    ReminderManager(const ReminderManager&) = delete;
    ReminderManager& operator=(const ReminderManager&) = delete;

    struct FiredReminder {
        Entry*      entry;
        QJsonObject obj;
        QString     title;
        QString     message;
        QDateTime   datetime;
    };

    void handleFired(const FiredReminder& r);
    void reschedule(Entry* entry, QJsonObject obj, int snoozeMinutes = 0);
    QString nextDatetime(const QJsonObject& obj) const;

    QTimer*                  m_timer     = nullptr;
    QSharedPointer<Database> m_db;
    QWidget*                 m_mainWin   = nullptr;
};
