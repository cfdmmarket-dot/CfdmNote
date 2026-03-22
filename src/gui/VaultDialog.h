#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QComboBox>

/**
 * VaultDialog
 *
 * Janela para criar um cofre criptografado (.kdbx) dentro do CfdmNote.
 *
 * O cofre é protegido por senha mestra com AES-256 + Argon2.
 * Entradas no cofre ficam no .kdbx (criptografado).
 * Entradas fora do cofre ficam no .ksdb (SQLite, banco principal do CfdmNote).
 */
class VaultDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VaultDialog(QWidget* parent = nullptr);
    ~VaultDialog() override = default;

    QString createdVaultPath() const { return m_vaultPath; }

private slots:
    void onBrowse();
    void onCreate();
    void onPasswordToggle();
    void validateForm();

private:
    void setupUi();
    bool createVault(const QString& filePath, const QString& password);

    QLineEdit*   m_nameEdit       = nullptr;
    QLineEdit*   m_pathEdit       = nullptr;
    QPushButton* m_btnBrowse      = nullptr;
    QLineEdit*   m_passwordEdit   = nullptr;
    QLineEdit*   m_confirmEdit    = nullptr;
    QPushButton* m_btnShowPass    = nullptr;
    QComboBox*   m_encryptionCombo= nullptr;
    QLabel*      m_strengthLabel  = nullptr;
    QProgressBar*m_strengthBar    = nullptr;
    QPushButton* m_btnCreate      = nullptr;
    QPushButton* m_btnCancel      = nullptr;
    QLabel*      m_statusLabel    = nullptr;

    QString      m_vaultPath;
};
