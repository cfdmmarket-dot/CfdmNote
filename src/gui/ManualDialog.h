#pragma once

#include <QDialog>
#include <QTextBrowser>
#include <QListWidget>
#include <QLineEdit>
#include <QLabel>
#include <QStringList>

/**
 * ManualDialog — Visualizador do Manual de Consulta do CfdmNote.
 *
 * Lê MANUAL.md do recurso embarcado (:/cfdmnote/MANUAL.md),
 * renderiza com cmark-gfm e exibe num QTextBrowser.
 *
 * Funcionalidades:
 *   - Índice lateral com todas as seções (## e #)
 *   - Clique no índice → navega para a seção
 *   - Busca por texto (↑/↓ para múltiplas ocorrências)
 *   - Atalho F1 para abrir
 */
class ManualDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ManualDialog(QWidget* parent = nullptr);

    static void showManual(QWidget* parent = nullptr);

private slots:
    void onTocItemClicked(QListWidgetItem* item);
    void onSearchNext();
    void onSearchPrev();
    void onSearchChanged(const QString& text);
    void onClearSearch();

private:
    QString     loadMarkdown() const;
    QString     renderToHtml(const QString& markdown) const;
    void        buildToc(const QString& markdown);

    QTextBrowser* m_browser   = nullptr;
    QListWidget*  m_tocList   = nullptr;
    QLineEdit*    m_searchBox = nullptr;
    QLabel*       m_searchStatus = nullptr;

    QStringList   m_tocTitles;   // texto de cada seção
    QString       m_rawMarkdown;
};
