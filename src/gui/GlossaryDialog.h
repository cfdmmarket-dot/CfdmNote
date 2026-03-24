#pragma once

#include <QDialog>
#include <QListWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QSharedPointer>
#include "notes/NoteManager.h"

class Entry;
class Database;

/**
 * GlossaryDialog — Seção do Saber (feature 018)
 *
 * Dicionário/glossário pessoal armazenado na tabela `glossary` do banco .ksdb.
 * Permite adicionar termos com definição, categoria, sinônimos e exemplo.
 * Busca em tempo real. Inserção na nota da entrada selecionada com um clique.
 */
class GlossaryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GlossaryDialog(Entry* entry,
                             const QSharedPointer<Database>& db,
                             QWidget* parent = nullptr);

    static void showGlossary(Entry* entry,
                              const QSharedPointer<Database>& db,
                              QWidget* parent = nullptr);

private slots:
    void onSearch(const QString& query);
    void onCategorySelected();
    void onTermSelected();
    void onNewTerm();
    void onEditTerm();
    void onDeleteTerm();
    void onInsertInNote();

private:
    void refreshCategories();
    void loadTerms(const QString& category = {}, const QString& search = {});
    void showTermDetail(const NoteManager::GlossaryTerm& t);
    void clearDetail();
    bool openTermEditor(NoteManager::GlossaryTerm& t);  // retorna true se confirmado

    Entry*                   m_entry = nullptr;
    QSharedPointer<Database> m_db;

    QLineEdit*   m_searchBox    = nullptr;
    QListWidget* m_categoryList = nullptr;
    QTableWidget* m_termTable   = nullptr;

    // Painel de detalhes (direita)
    QLabel*    m_detailTerm     = nullptr;
    QLabel*    m_detailCategory = nullptr;
    QLabel*    m_detailSynonyms = nullptr;
    QTextEdit* m_detailDef      = nullptr;
    QTextEdit* m_detailExample  = nullptr;

    QPushButton* m_btnInsert = nullptr;
    QPushButton* m_btnEdit   = nullptr;
    QPushButton* m_btnDelete = nullptr;

    QString m_currentId;  // ID do termo selecionado
};
