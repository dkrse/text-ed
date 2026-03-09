#pragma once
#include <QDialog>
#include "SshSession.h"

class QLineEdit;
class QSpinBox;
class QCheckBox;
class QRadioButton;

class SshConnectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SshConnectDialog(QWidget *parent = nullptr);
    SshHostInfo hostInfo() const;

private:
    QLineEdit *m_host;
    QSpinBox *m_port;
    QLineEdit *m_username;
    QLineEdit *m_password;
    QLineEdit *m_keyPath;
    QRadioButton *m_passwordAuth;
    QRadioButton *m_keyAuth;
};
