#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QSharedPointer>

class Entry;
class Database;

/**
 * AttachmentsPanelWidget — aba "📎 Anexos" no preview de entrada.
 *
 * Agrega todos os arquivos vinculados a uma entrada:
 *   • KeePassXC binary attachments   (entry->attachments())
 *   • Arquivos locais / docs importados (CfdmNote::LocalLinks)
 *   • Áudio gravado                  (CfdmNote::AudioAttachments)
 *   • Mídia removível                (CfdmNote::RemovableLinks)
 *
 * Ações por item: abrir, salvar em disco, reproduzir (áudio), remover.
 */
class AttachmentsPanelWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AttachmentsPanelWidget(QWidget* parent = nullptr);

    void setEntry(Entry* entry, const QSharedPointer<Database>& db);
    void clear();

    // Chave do atributo de áudio gravado
    static const QString KEY_AUDIO;

private slots:
    void onItemDoubleClicked(QListWidgetItem* item);
    void onOpen();
    void onSaveToDesk();
    void onRemove();
    void onAddFile();
    void showContextMenu(const QPoint& pos);

private:
    void refresh();
    void addRow(const QString& icon, const QString& label,
                const QString& type, const QString& path,
                const QString& source);
    void openItem(QListWidgetItem* item);
    void removeItem(QListWidgetItem* item);

    QListWidget* m_list       = nullptr;
    QLabel*      m_emptyLbl   = nullptr;
    QPushButton* m_btnOpen    = nullptr;
    QPushButton* m_btnSave    = nullptr;
    QPushButton* m_btnRemove  = nullptr;
    QPushButton* m_btnAdd     = nullptr;

    Entry*                   m_entry = nullptr;
    QSharedPointer<Database> m_db;
};
