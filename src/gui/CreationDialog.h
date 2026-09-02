#pragma once

#include <QDialog>
#include <QTextEdit>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QTabWidget>
#include <QSharedPointer>

class Entry;
class Database;

/**
 * CreationDialog — gera conteúdo usando os dados da entrada/grupo como base.
 *
 * Seção "Gerar Agora": templates funcionais imediatos (email, projeto, post).
 * Seção "Com IA (CFDM Triplex)": stubs para geração por IA (Feature 019+).
 *
 * Acesso: botão IA → 🛠 Criar a partir desta entrada...
 */
class CreationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CreationDialog(Entry* entry,
                             QSharedPointer<Database> db,
                             QWidget* parent = nullptr);

    static void create(Entry* entry,
                        QSharedPointer<Database> db,
                        QWidget* parent = nullptr);

private slots:
    void onTemplateChanged(int index);
    void onGenerate();
    void onCopy();
    void onInsertInNote();
    void onAiAction(const QString& action);

private:
    void buildUI();
    QString buildContext() const;
    QString applyTemplate(const QString& tmpl) const;

    Entry*                   m_entry;
    QSharedPointer<Database> m_db;

    QComboBox* m_templateCombo = nullptr;
    QTextEdit* m_contextView   = nullptr;  // dados da entrada (read-only)
    QTextEdit* m_output        = nullptr;  // resultado gerado
    QPushButton* m_btnGenerate = nullptr;
    QPushButton* m_btnCopy     = nullptr;
    QPushButton* m_btnInsert   = nullptr;
    QLabel*    m_statusLabel   = nullptr;
};
