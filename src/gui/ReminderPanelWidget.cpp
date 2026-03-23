#include "ReminderPanelWidget.h"
#include "ReminderDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QMessageBox>

#include "core/EntryAttributes.h"

static constexpr const char* ATTR_REMINDER = "CfdmNote::Reminder";

// ─── Constructor ──────────────────────────────────────────────────────────────

ReminderPanelWidget::ReminderPanelWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(10, 10, 10, 10);
    outerLayout->setSpacing(10);

    // ── Card de status ────────────────────────────────────────────────────────
    auto* card = new QFrame();
    card->setFrameShape(QFrame::StyledPanel);
    card->setStyleSheet(
        "QFrame {"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 8px;"
        "  background: #f7fafc;"
        "}");
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(12, 10, 12, 10);
    cardLayout->setSpacing(6);

    // Linha de status (ícone + texto)
    auto* statusRow = new QHBoxLayout();
    m_iconLbl = new QLabel("⏰");
    m_iconLbl->setStyleSheet("font-size: 24px;");
    m_iconLbl->setFixedWidth(36);
    statusRow->addWidget(m_iconLbl);

    m_statusLbl = new QLabel(tr("Nenhum lembrete definido"));
    m_statusLbl->setStyleSheet("font-size: 13px; font-weight: bold; color: #4a5568;");
    m_statusLbl->setWordWrap(true);
    statusRow->addWidget(m_statusLbl, 1);
    cardLayout->addLayout(statusRow);

    // Separador
    auto* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #e2e8f0;");
    cardLayout->addWidget(line);

    // Detalhes (data, recorrência, mensagem)
    m_detailsFrame = new QWidget();
    auto* detLayout = new QVBoxLayout(m_detailsFrame);
    detLayout->setContentsMargins(0, 0, 0, 0);
    detLayout->setSpacing(4);

    auto makeDetailRow = [&](const QString& icon, QLabel*& lbl) {
        auto* row = new QHBoxLayout();
        auto* iconLbl = new QLabel(icon);
        iconLbl->setFixedWidth(20);
        iconLbl->setStyleSheet("font-size: 13px;");
        lbl = new QLabel();
        lbl->setStyleSheet("font-size: 12px; color: #2d3748;");
        lbl->setWordWrap(true);
        row->addWidget(iconLbl);
        row->addWidget(lbl, 1);
        detLayout->addLayout(row);
    };

    makeDetailRow("📅", m_dateLbl);
    makeDetailRow("🔁", m_recurrLbl);
    makeDetailRow("📝", m_msgLbl);

    cardLayout->addWidget(m_detailsFrame);
    outerLayout->addWidget(card);

    // ── Botões de ação ────────────────────────────────────────────────────────
    auto* btnRow = new QHBoxLayout();

    m_btnEdit = new QPushButton(tr("✏️  Definir / Editar lembrete"));
    m_btnEdit->setFixedHeight(34);
    m_btnEdit->setStyleSheet(
        "QPushButton {"
        "  background: #3182ce; color: white; border-radius: 6px;"
        "  font-weight: bold; font-size: 12px; padding: 0 14px;"
        "}"
        "QPushButton:hover { background: #2c5282; }"
        "QPushButton:disabled { background: #a0aec0; }");
    connect(m_btnEdit, &QPushButton::clicked, this,
            &ReminderPanelWidget::openReminderDialog);

    m_btnRemove = new QPushButton(tr("🗑  Remover"));
    m_btnRemove->setFixedHeight(34);
    m_btnRemove->setStyleSheet(
        "QPushButton {"
        "  border: 1px solid #fc8181; color: #c53030;"
        "  border-radius: 6px; font-size: 12px; padding: 0 14px;"
        "}"
        "QPushButton:hover { background: #fff5f5; }"
        "QPushButton:disabled { color: #a0aec0; border-color: #e2e8f0; }");
    connect(m_btnRemove, &QPushButton::clicked, this,
            &ReminderPanelWidget::removeReminder);

    btnRow->addWidget(m_btnEdit, 1);
    btnRow->addWidget(m_btnRemove);
    outerLayout->addLayout(btnRow);

    // Dica sobre o atalho
    auto* hint = new QLabel(
        tr("<span style='color:#a0aec0;font-size:10px'>"
           "Atalho global: <b>Ctrl+Shift+R</b> · Também disponível no menu CfdmNote</span>"));
    hint->setTextFormat(Qt::RichText);
    hint->setAlignment(Qt::AlignCenter);
    outerLayout->addWidget(hint);

    outerLayout->addStretch();

    // Estado inicial
    m_detailsFrame->setVisible(false);
    m_btnEdit->setEnabled(false);
    m_btnRemove->setEnabled(false);
}

// ─── API pública ──────────────────────────────────────────────────────────────

void ReminderPanelWidget::setEntry(Entry* entry, QSharedPointer<Database> db)
{
    m_entry = entry;
    m_db    = db;
    refresh();
}

void ReminderPanelWidget::clear()
{
    m_entry = nullptr;
    m_db.reset();
    refresh();
}

// ─── refresh ──────────────────────────────────────────────────────────────────

void ReminderPanelWidget::refresh()
{
    if (!m_entry) {
        m_iconLbl->setText("⏰");
        m_statusLbl->setText(tr("Selecione uma entrada para ver o lembrete."));
        m_statusLbl->setStyleSheet("font-size: 13px; color: #718096;");
        m_detailsFrame->setVisible(false);
        m_btnEdit->setEnabled(false);
        m_btnRemove->setEnabled(false);
        return;
    }

    m_btnEdit->setEnabled(true);

    const QString raw = m_entry->attributes()->value(ATTR_REMINDER);
    if (raw.isEmpty()) {
        m_iconLbl->setText("⏰");
        m_statusLbl->setText(tr("Nenhum lembrete definido para esta entrada."));
        m_statusLbl->setStyleSheet("font-size: 13px; font-weight: bold; color: #718096;");
        m_detailsFrame->setVisible(false);
        m_btnRemove->setEnabled(false);
        return;
    }

    const QJsonObject obj = QJsonDocument::fromJson(raw.toUtf8()).object();
    const bool enabled    = obj.value("enabled").toBool(true);
    const QString dtStr   = obj.value("datetime").toString();
    const QString recur   = obj.value("recurrence").toString("once");
    const QString msg     = obj.value("message").toString();

    const QDateTime dt = QDateTime::fromString(dtStr, Qt::ISODate);
    const bool isPast  = dt.isValid() && dt < QDateTime::currentDateTime() && recur == "once";

    // Ícone e cor conforme estado
    if (!enabled) {
        m_iconLbl->setText("🔕");
        m_statusLbl->setText(tr("Lembrete desativado"));
        m_statusLbl->setStyleSheet("font-size: 13px; font-weight: bold; color: #718096;");
    } else if (isPast) {
        m_iconLbl->setText("✅");
        m_statusLbl->setText(tr("Lembrete já disparado"));
        m_statusLbl->setStyleSheet("font-size: 13px; font-weight: bold; color: #276749;");
    } else {
        // Calcula tempo restante
        const qint64 secsLeft = QDateTime::currentDateTime().secsTo(dt);
        QString remaining;
        if (secsLeft > 86400)
            remaining = tr("em %1 dia(s)").arg(secsLeft / 86400);
        else if (secsLeft > 3600)
            remaining = tr("em %1 hora(s)").arg(secsLeft / 3600);
        else if (secsLeft > 60)
            remaining = tr("em %1 minuto(s)").arg(secsLeft / 60);
        else if (secsLeft > 0)
            remaining = tr("em menos de 1 minuto");
        else
            remaining = tr("agora");

        m_iconLbl->setText("🔔");
        m_statusLbl->setText(tr("Lembrete ativo — dispara %1").arg(remaining));
        m_statusLbl->setStyleSheet("font-size: 13px; font-weight: bold; color: #2c5282;");
    }

    // Detalhes
    m_dateLbl->setText(dt.isValid()
        ? dt.toString("dddd, dd/MM/yyyy 'às' HH:mm")
        : tr("Data inválida"));

    m_recurrLbl->setText(recurrenceLabel(recur));
    m_msgLbl->setText(msg.isEmpty() ? tr("(sem mensagem)") : msg);
    m_msgLbl->setVisible(!msg.isEmpty());

    m_detailsFrame->setVisible(true);
    m_btnRemove->setEnabled(true);
}

// ─── Ações ────────────────────────────────────────────────────────────────────

void ReminderPanelWidget::openReminderDialog()
{
    if (!m_entry || !m_db) return;
    ReminderDialog::editReminder(m_entry, m_db, this);
    refresh(); // Atualiza após fechar o diálogo
}

void ReminderPanelWidget::removeReminder()
{
    if (!m_entry || !m_db) return;

    const auto reply = QMessageBox::question(
        this, tr("Remover lembrete"),
        tr("Remover o lembrete desta entrada?"),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    m_entry->attributes()->remove(ATTR_REMINDER);
    QString err;
    m_db->save(Database::Atomic, {}, &err);
    refresh();
}

// ─── Helper ───────────────────────────────────────────────────────────────────

/*static*/
QString ReminderPanelWidget::recurrenceLabel(const QString& key)
{
    if (key == "daily")   return QObject::tr("Repete: todos os dias");
    if (key == "weekly")  return QObject::tr("Repete: toda semana");
    if (key == "monthly") return QObject::tr("Repete: todo mês");
    if (key == "yearly")  return QObject::tr("Repete: todo ano");
    return QObject::tr("Não repete (única vez)");
}
