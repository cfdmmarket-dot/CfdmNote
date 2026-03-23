#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QSharedPointer>

#include "core/Entry.h"
#include "core/Database.h"

/**
 * Aba "Relacionados" — mostra entradas com o mesmo domínio (auto)
 * e referências cruzadas manuais (armazenadas em CfdmNote::RelatedEntries).
 */
class RelatedEntriesWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RelatedEntriesWidget(QWidget* parent = nullptr);

    void setEntry(Entry* entry, QSharedPointer<Database> db);
    void clear();

signals:
    /// Emitido quando o usuário quer navegar para outra entrada (UUID hex)
    void navigateRequested(const QString& uuidHex);

private:
    void refresh();
    void buildAutoSection();
    void buildManualSection();
    QPushButton* makeEntryButton(Entry* e, bool isManual);
    void showEntryPopup(Entry* e, bool isManual);

    QStringList manualUuids() const;
    void        saveManualUuids(const QStringList& uuids);
    void        addManualLink();
    void        removeManualLink(const QString& uuidHex);

    static QString domainOf(const QString& url);

    Entry*                   m_entry        = nullptr;
    QSharedPointer<Database> m_db;

    QVBoxLayout*  m_autoContent    = nullptr;
    QVBoxLayout*  m_manualContent  = nullptr;
    QGroupBox*    m_autoGroup      = nullptr;
    QGroupBox*    m_manualGroup    = nullptr;
};
