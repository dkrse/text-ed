#include "SshConnectDialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QRadioButton>
#include <QPushButton>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QDir>

SshConnectDialog::SshConnectDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Connect to SSH Server"));
    setMinimumWidth(400);

    auto *layout = new QVBoxLayout(this);

    // Server group
    auto *serverGroup = new QGroupBox(tr("Server"));
    auto *serverLayout = new QFormLayout(serverGroup);

    m_host = new QLineEdit;
    m_host->setPlaceholderText(tr("hostname or IP"));
    serverLayout->addRow(tr("Host:"), m_host);

    m_port = new QSpinBox;
    m_port->setRange(1, 65535);
    m_port->setValue(22);
    serverLayout->addRow(tr("Port:"), m_port);

    m_username = new QLineEdit;
    m_username->setText(qEnvironmentVariable("USER"));
    serverLayout->addRow(tr("Username:"), m_username);

    layout->addWidget(serverGroup);

    // Auth group
    auto *authGroup = new QGroupBox(tr("Authentication"));
    auto *authLayout = new QVBoxLayout(authGroup);

    m_passwordAuth = new QRadioButton(tr("Password"));
    m_passwordAuth->setChecked(true);
    authLayout->addWidget(m_passwordAuth);

    m_password = new QLineEdit;
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setPlaceholderText(tr("Password"));
    auto *passLayout = new QFormLayout;
    passLayout->addRow(tr("Password:"), m_password);
    authLayout->addLayout(passLayout);

    m_keyAuth = new QRadioButton(tr("Private Key"));
    authLayout->addWidget(m_keyAuth);

    m_keyPath = new QLineEdit;
    m_keyPath->setPlaceholderText(QDir::homePath() + "/.ssh/id_rsa");
    m_keyPath->setEnabled(false);
    auto *keyLayout = new QHBoxLayout;
    keyLayout->addWidget(m_keyPath);
    auto *browseBtn = new QPushButton(tr("Browse..."));
    browseBtn->setEnabled(false);
    keyLayout->addWidget(browseBtn);
    authLayout->addLayout(keyLayout);

    connect(m_keyAuth, &QRadioButton::toggled, m_keyPath, &QLineEdit::setEnabled);
    connect(m_keyAuth, &QRadioButton::toggled, browseBtn, &QPushButton::setEnabled);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Select Private Key"),
                            QDir::homePath() + "/.ssh");
        if (!path.isEmpty()) m_keyPath->setText(path);
    });

    layout->addWidget(authGroup);

    // Buttons
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Connect"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

SshHostInfo SshConnectDialog::hostInfo() const
{
    SshHostInfo info;
    info.host = m_host->text().trimmed();
    info.port = m_port->value();
    info.username = m_username->text().trimmed();
    info.password = m_password->text();
    info.useKeyAuth = m_keyAuth->isChecked();
    info.privateKeyPath = m_keyPath->text().trimmed();
    if (info.useKeyAuth && info.privateKeyPath.isEmpty())
        info.privateKeyPath = QDir::homePath() + "/.ssh/id_rsa";
    return info;
}
