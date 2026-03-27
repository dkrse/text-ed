#pragma once
#include <QDialog>
#include "SshSession.h"

class QLineEdit;
class QSpinBox;
class QCheckBox;
class QRadioButton;
class QListWidget;
class QPushButton;

class SshConnectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SshConnectDialog(QWidget *parent = nullptr);
    SshHostInfo hostInfo() const;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void saveConnection();
    void deleteConnection();
    void loadSelectedConnection();

private:
    void loadSavedConnections();
    void populateList();

    QListWidget *m_connectionList;
    QPushButton *m_deleteBtn;

    QLineEdit *m_host;
    QSpinBox *m_port;
    QLineEdit *m_username;
    QLineEdit *m_password;
    QLineEdit *m_keyPath;
    QRadioButton *m_passwordAuth;
    QRadioButton *m_keyAuth;

    struct SavedConnection {
        QString name;
        QString host;
        int port = 22;
        QString username;
        QString privateKeyPath;
        bool useKeyAuth = false;
    };
    QList<SavedConnection> m_saved;
};
