#include "ReminderDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

#include "core/Entry.h"
#include "core/Database.h"
#include "core/EntryAttributes.h"

static const QString KEY_REMINDER = "CfdmNote::Reminder";

// ─── Construtor ───────────────────────────────────────────────────────────────

ReminderDialog::ReminderDialog(Entry* entry,
                                const QSharedPointer<Database>& db,
                                QWidget* parent)
    : QDialog(parent)
    , m_entry(entry)
    , m_db(db)
{
    setWindowTitle(tr("Lembrete — %1").arg(entry ? entry->title() : ""));
    setMinimumWidth(420);

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(14, 14, 14, 14);

    // Título
    auto* titleLabel = new QLabel(
        tr("<b>Definir lembrete para:</b> %1")
            .arg(entry ? entry->title().toHtmlEscaped() : ""));
    titleLabel->setStyleSheet("font-size: 12px;");
    titleLabel->setWordWrap(true);
    layout->addWidget(titleLabel);

    // Formulário
    auto* form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    form->setSpacing(8);

    m_dateEdit = new QDateEdit(QDate::currentDate().addDays(1));
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat("dd/MM/yyyy");
    m_dateEdit->setMinimumDate(QDate::currentDate());

    m_timeEdit = new QTimeEdit(QTime(9, 0));
    m_timeEdit->setDisplayFormat("HH:mm");

    // Data e hora na mesma linha
    auto* dtRow = new QHBoxLayout();
    dtRow->addWidget(m_dateEdit, 1);
    auto* atLabel = new QLabel(tr("às"));
    atLabel->setStyleSheet("color: palette(mid);");
    dtRow->addWidget(atLabel);
    dtRow->addWidget(m_timeEdit);

    m_recCombo = new QComboBox();
    m_recCombo->addItem(tr("Uma vez"),   "once");
    m_recCombo->addItem(tr("Diário"),    "daily");
    m_recCombo->addItem(tr("Semanal"),   "weekly");
    m_recCombo->addItem(tr("Mensal"),    "monthly");
    m_recCombo->addItem(tr("Anual"),     "yearly");

    m_msgEdit = new QLineEdit();
    m_msgEdit->setPlaceholderText(tr("Mensagem opcional do lembrete..."));
    m_msgEdit->setMaxLength(200);

    form->addRow(tr("Data / Hora:"),   dtRow);
    form->addRow(tr("Recorrência:"),   m_recCombo);
    form->addRow(tr("Mensagem:"),      m_msgEdit);
    layout->addLayout(form);

    // Status (exibe lembrete existente)
    m_status = new QLabel();
    m_status->setStyleSheet("font-size: 11px; color: palette(mid);");
    m_status->setWordWrap(true);
    layout->addWidget(m_status);

    // Botões
    auto* btnRow    = new QHBoxLayout();
    auto* btnGCal   = new QPushButton(QIcon::fromTheme("internet-web-browser"),
                                       tr("Google Calendar"));
    auto* btnSave   = new QPushButton(QIcon::fromTheme("document-save"), tr("Salvar"));
    auto* btnCancel = new QPushButton(tr("Cancelar"));
    m_btnRemove     = new QPushButton(QIcon::fromTheme("list-remove"), tr("Remover"));
    m_btnRemove->setEnabled(false);

    btnGCal->setFixedHeight(28);
    btnSave->setFixedHeight(28);
    btnCancel->setFixedHeight(28);
    m_btnRemove->setFixedHeight(28);

    btnRow->addWidget(btnGCal);
    btnRow->addWidget(m_btnRemove);
    btnRow->addStretch();
    btnRow->addWidget(btnSave);
    btnRow->addWidget(btnCancel);
    layout->addLayout(btnRow);

    connect(btnSave,       &QPushButton::clicked, this, &ReminderDialog::onSave);
    connect(btnCancel,     &QPushButton::clicked, this, &QDialog::reject);
    connect(m_btnRemove,   &QPushButton::clicked, this, &ReminderDialog::onRemove);
    connect(btnGCal,       &QPushButton::clicked, this, &ReminderDialog::onAddToGoogleCalendar);

    loadExisting();
}

// ─── Estático ─────────────────────────────────────────────────────────────────

void ReminderDialog::editReminder(Entry* entry,
                                   const QSharedPointer<Database>& db,
                                   QWidget* parent)
{
    if (!entry) return;
    ReminderDialog dlg(entry, db, parent);
    dlg.exec();
}

// ─── Carrega lembrete existente ────────────────────────────────────────────────

void ReminderDialog::loadExisting()
{
    if (!m_entry || !m_entry->attributes()->contains(KEY_REMINDER)) return;

    const QByteArray json = m_entry->attributes()->value(KEY_REMINDER).toUtf8();
    const QJsonObject obj = QJsonDocument::fromJson(json).object();
    if (obj.isEmpty()) return;

    const QDateTime dt = QDateTime::fromString(obj.value("datetime").toString(), Qt::ISODate);
    if (dt.isValid()) {
        m_dateEdit->setDate(dt.date());
        m_timeEdit->setTime(dt.time());
        // Permite datas passadas ao editar
        m_dateEdit->setMinimumDate(QDate(2000, 1, 1));
    }

    const QString rec = obj.value("recurrence").toString("once");
    for (int i = 0; i < m_recCombo->count(); ++i) {
        if (m_recCombo->itemData(i).toString() == rec) {
            m_recCombo->setCurrentIndex(i);
            break;
        }
    }

    m_msgEdit->setText(obj.value("message").toString());

    const bool enabled = obj.value("enabled").toBool(true);
    m_hasReminder = true;
    m_btnRemove->setEnabled(true);

    const QString statusText = enabled
        ? tr("✅ Lembrete ativo: %1").arg(dt.isValid() ? dt.toString("dd/MM/yyyy HH:mm") : "")
        : tr("⏸ Lembrete desativado (disparou e não repete).");
    m_status->setText(statusText);
    m_status->setStyleSheet(enabled
        ? "font-size: 11px; color: #007700;"
        : "font-size: 11px; color: palette(mid);");
}

// ─── Salvar ───────────────────────────────────────────────────────────────────

void ReminderDialog::onSave()
{
    if (!m_entry || !m_db) return;

    const QDateTime dt(m_dateEdit->date(), m_timeEdit->time());

    QJsonObject obj;
    obj["datetime"]   = dt.toString(Qt::ISODate);
    obj["recurrence"] = m_recCombo->currentData().toString();
    obj["message"]    = m_msgEdit->text().trimmed();
    obj["enabled"]    = true;

    m_entry->attributes()->set(KEY_REMINDER,
        QJsonDocument(obj).toJson(QJsonDocument::Compact), false);

    QString err;
    m_db->save(Database::Atomic, {}, &err);

    if (err.isEmpty()) {
        accept();
    } else {
        QMessageBox::warning(this, tr("Aviso"),
            tr("Lembrete salvo, mas erro ao gravar banco: %1").arg(err));
        accept();
    }
}

// ─── Remover ──────────────────────────────────────────────────────────────────

void ReminderDialog::onRemove()
{
    if (!m_entry || !m_db) return;

    const auto btn = QMessageBox::question(this, tr("Remover lembrete"),
        tr("Remover o lembrete desta entrada?"),
        QMessageBox::Yes | QMessageBox::No);
    if (btn != QMessageBox::Yes) return;

    m_entry->attributes()->remove(KEY_REMINDER);
    QString err;
    m_db->save(Database::Atomic, {}, &err);
    accept();
}

// ─── Google Calendar ──────────────────────────────────────────────────────────

void ReminderDialog::onAddToGoogleCalendar()
{
    if (!m_entry) return;

    const QDateTime start(m_dateEdit->date(), m_timeEdit->time());
    const QDateTime end = start.addSecs(3600); // evento de 1 hora

    // Formato Google Calendar: YYYYMMDDTHHmmss
    const QString startStr = start.toString("yyyyMMddTHHmmss");
    const QString endStr   = end.toString("yyyyMMddTHHmmss");

    const QString title   = QUrl::toPercentEncoding(m_entry->title());
    const QString details = QUrl::toPercentEncoding(
        m_msgEdit->text().isEmpty() ? m_entry->notes().left(200)
                                    : m_msgEdit->text());
    const QString location = QUrl::toPercentEncoding(m_entry->url());

    const QString url = QString(
        "https://calendar.google.com/calendar/render"
        "?action=TEMPLATE"
        "&text=%1"
        "&dates=%2/%3"
        "&details=%4"
        "&location=%5"
    ).arg(title, startStr, endStr, details, location);

    QDesktopServices::openUrl(QUrl(url));
}
