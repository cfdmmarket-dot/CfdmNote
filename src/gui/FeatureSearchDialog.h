#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QString>
#include <QList>
#include <functional>

/**
 * FeatureSearchDialog — paleta de comandos do CfdmNote.
 * Lista todas as funcionalidades do app com nome, categoria,
 * atalho e descrição. Filtra enquanto digita. Abre com Ctrl+K.
 *
 * Uso:
 *   QList<FeatureItem> features = { ... };
 *   FeatureSearchDialog dlg(features, parent);
 *   dlg.exec();  // lança a feature escolhida automaticamente
 */
struct FeatureItem {
    QString emoji;
    QString name;
    QString category;
    QString shortcut;
    QString description;
    std::function<void()> action;
};

class FeatureSearchDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FeatureSearchDialog(const QList<FeatureItem>& features,
                                  QWidget* parent = nullptr);

private slots:
    void onSearch(const QString& text);
    void onActivate(QListWidgetItem* item);
    void onCurrentChanged(QListWidgetItem* current);

private:
    bool eventFilter(QObject* obj, QEvent* ev) override;
    void populate(const QString& filter = {});
    void launch(int idx);

    QLineEdit*   m_search  = nullptr;
    QListWidget* m_list    = nullptr;
    QLabel*      m_detail  = nullptr;

    QList<FeatureItem> m_features;
    QList<int>         m_filtered;  // índices em m_features após filtro
};
