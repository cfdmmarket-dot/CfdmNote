#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSharedPointer>

#include "core/Entry.h"
#include "core/Database.h"

/**
 * ReminderPanelWidget — painel compacto da aba "⏰ Lembrete"
 * no notesTabWidget. Mostra o estado do lembrete da entrada
 * selecionada e permite criar/editar/remover sem abrir o menu.
 */
class ReminderPanelWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ReminderPanelWidget(QWidget* parent = nullptr);

    void setEntry(Entry* entry, QSharedPointer<Database> db);
    void clear();

private:
    void refresh();
    void openReminderDialog();
    void removeReminder();

    static QString recurrenceLabel(const QString& key);

    Entry*                   m_entry = nullptr;
    QSharedPointer<Database> m_db;

    // Widgets do painel de status
    QLabel*      m_iconLbl      = nullptr;
    QLabel*      m_statusLbl    = nullptr;
    QLabel*      m_dateLbl      = nullptr;
    QLabel*      m_recurrLbl    = nullptr;
    QLabel*      m_msgLbl       = nullptr;
    QPushButton* m_btnEdit      = nullptr;
    QPushButton* m_btnRemove    = nullptr;
    QWidget*     m_detailsFrame = nullptr;
};
