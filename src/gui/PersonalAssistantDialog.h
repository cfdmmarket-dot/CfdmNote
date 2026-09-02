#pragma once

#include <QDialog>
#include <QSharedPointer>

class Entry;
class Database;

/**
 * PersonalAssistantDialog — Stub para o Assistente Pessoal (CFDM Triplex).
 *
 * O assistente orquestrará agentes de IA a partir de um repositório GitHub
 * cedido ao usuário. Coordena: criação de conteúdo, análise, email marketing,
 * propostas, ebooks e demais features do CFDM Triplex.
 *
 * Status: aguardando repositório GitHub (Feature 020+).
 */
class PersonalAssistantDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PersonalAssistantDialog(Entry* entry,
                                     QSharedPointer<Database> db,
                                     QWidget* parent = nullptr);

    static void show(Entry* entry,
                     QSharedPointer<Database> db,
                     QWidget* parent = nullptr);

private:
    void buildUI();

    Entry*                   m_entry;
    QSharedPointer<Database> m_db;
};
