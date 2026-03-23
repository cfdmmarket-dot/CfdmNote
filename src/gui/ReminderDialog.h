#pragma once

#include <QDialog>
#include <QDateEdit>
#include <QTimeEdit>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QSharedPointer>

class Entry;
class Database;

/**
 * ReminderDialog — Define ou edita um lembrete associado a uma entrada.
 *
 * Armazena em atributo personalizado da entrada:
 *   "CfdmNote::Reminder"  →  JSON compacto
 *   {
 *     "datetime":   "2026-03-25T09:00:00",   // ISO 8601
 *     "recurrence": "once",                  // once|daily|weekly|monthly|yearly
 *     "message":    "Renovar assinatura",
 *     "enabled":    true
 *   }
 *
 * Também oferece "Adicionar ao Google Calendar" via URL no browser.
 */
class ReminderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReminderDialog(Entry* entry,
                             const QSharedPointer<Database>& db,
                             QWidget* parent = nullptr);

    static void editReminder(Entry* entry,
                              const QSharedPointer<Database>& db,
                              QWidget* parent = nullptr);

private slots:
    void onSave();
    void onRemove();
    void onAddToGoogleCalendar();

private:
    void loadExisting();

    Entry*                   m_entry;
    QSharedPointer<Database> m_db;
    bool                     m_hasReminder = false;

    QDateEdit*   m_dateEdit  = nullptr;
    QTimeEdit*   m_timeEdit  = nullptr;
    QComboBox*   m_recCombo  = nullptr;
    QLineEdit*   m_msgEdit   = nullptr;
    QLabel*      m_status    = nullptr;
    QPushButton* m_btnRemove = nullptr;
};
