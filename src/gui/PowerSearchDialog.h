#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QTreeWidget>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QThread>
#include <QDateTime>
#include <QStringList>
#include <atomic>

// ─── Worker: busca local em thread separada ───────────────────────────────────

class SearchWorker : public QObject
{
    Q_OBJECT
public:
    void setParams(const QString& dir, const QStringList& extensions,
                   const QString& nameFilter, bool recursive);
public slots:
    void start();
    void cancel() { m_cancelled = true; }
signals:
    void resultFound(const QString& name, const QString& path,
                     qint64 size, const QDateTime& modified);
    void finished();
private:
    QString              m_dir;
    QStringList          m_extensions;
    QString              m_nameFilter;
    bool                 m_recursive = true;
    std::atomic<bool>    m_cancelled{false};
};

// ─── Diálogo principal ────────────────────────────────────────────────────────

class PowerSearchDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PowerSearchDialog(QWidget* parent = nullptr);
    ~PowerSearchDialog();

private slots:
    void onBrowseDir();
    void onDetectMedia();
    void onTypeChanged(int index);
    void onSearch();
    void onCancelSearch();
    void onResultFound(const QString& name, const QString& path,
                       qint64 size, const QDateTime& modified);
    void onSearchFinished();
    void onItemDoubleClicked(QTreeWidgetItem* item, int col);
    void onCopyPath();
    void onWebSearch(const QString& engine);
    void onCheckUrl(const QString& service);

private:
    void        setupLocalTab(QTabWidget* tabs);
    void        setupWebTab(QTabWidget* tabs);
    void        setupLinkCheckTab(QTabWidget* tabs);
    void        setupTrendingTab(QTabWidget* tabs);
    QStringList extensionsForCurrentType() const;
    QString     buildDorkQuery() const;

    // Abre URL no navegador selecionado (privado ou padrão)
    void        openUrl(const QString& url);

    // Detecta navegadores privados instalados no sistema
    struct BrowserInfo { QString name; QString exe; QStringList args; };
    static QList<BrowserInfo> detectBrowsers();

    // ── Seletor de navegador (compartilhado entre abas) ────────────────────
    QComboBox*   m_browserCombo = nullptr;

    // ── Aba 1: Arquivos Locais ─────────────────────────────────────────────
    QLineEdit*   m_nameFilter   = nullptr;
    QComboBox*   m_typeCombo    = nullptr;
    QLineEdit*   m_customExt    = nullptr;
    QWidget*     m_customRow    = nullptr;
    QLineEdit*   m_dirEdit      = nullptr;
    QCheckBox*   m_recursive    = nullptr;
    QPushButton* m_btnSearch    = nullptr;
    QPushButton* m_btnCancel    = nullptr;
    QTreeWidget* m_results      = nullptr;
    QLabel*      m_statusLocal  = nullptr;

    // ── Aba 2: Busca na Web ────────────────────────────────────────────────
    QLineEdit*   m_webQuery     = nullptr;
    QLineEdit*   m_dorkSite     = nullptr;
    QLineEdit*   m_dorkFiletype = nullptr;
    QLineEdit*   m_dorkIntitle  = nullptr;
    QLineEdit*   m_dorkInurl    = nullptr;
    QLineEdit*   m_dorkExact    = nullptr;
    QLineEdit*   m_dorkTerm     = nullptr;
    QLabel*      m_dorkPreview  = nullptr;

    // ── Aba 3: Verificar Link ──────────────────────────────────────────────
    QLineEdit*   m_urlEdit      = nullptr;
    QLabel*      m_urlStatus    = nullptr;

    // ── Thread ────────────────────────────────────────────────────────────
    QThread*      m_thread     = nullptr;
    SearchWorker* m_worker     = nullptr;
    int           m_foundCount = 0;
};
