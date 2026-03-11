#include "SshConnectDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QRadioButton>
#include <QPushButton>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QListWidget>
#include <QSettings>
#include <QInputDialog>
#include <QMessageBox>
#include <QSplitter>

SshConnectDialog::SshConnectDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Connect to SSH Server"));
    setMinimumSize(600, 420);

    auto *mainLayout = new QHBoxLayout(this);

    // Left: saved connections
    auto *leftWidget = new QWidget;
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    auto *savedGroup = new QGroupBox(tr("Saved Connections"));
    auto *savedLayout = new QVBoxLayout(savedGroup);

    m_connectionList = new QListWidget;
    connect(m_connectionList, &QListWidget::itemDoubleClicked, this, &SshConnectDialog::loadSelectedConnection);
    connect(m_connectionList, &QListWidget::currentRowChanged, this, [this](int row) {
        m_deleteBtn->setEnabled(row >= 0);
        if (row >= 0) loadSelectedConnection();
    });
    savedLayout->addWidget(m_connectionList);

    auto *btnLayout = new QHBoxLayout;
    auto *saveBtn = new QPushButton(tr("Save Current"));
    connect(saveBtn, &QPushButton::clicked, this, &SshConnectDialog::saveConnection);
    btnLayout->addWidget(saveBtn);

    m_deleteBtn = new QPushButton(tr("Delete"));
    m_deleteBtn->setEnabled(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &SshConnectDialog::deleteConnection);
    btnLayout->addWidget(m_deleteBtn);
    savedLayout->addLayout(btnLayout);

    leftLayout->addWidget(savedGroup);
    mainLayout->addWidget(leftWidget);

    // Right: connection form
    auto *rightWidget = new QWidget;
    auto *layout = new QVBoxLayout(rightWidget);
    layout->setContentsMargins(0, 0, 0, 0);

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

    mainLayout->addWidget(rightWidget, 1);

    loadSavedConnections();
    populateList();
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

void SshConnectDialog::saveConnection()
{
    QString host = m_host->text().trimmed();
    QString user = m_username->text().trimmed();
    if (host.isEmpty() || user.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Host and username are required."));
        return;
    }

    QString defaultName = user + "@" + host;
    bool ok;
    QString name = QInputDialog::getText(this, tr("Save Connection"),
        tr("Connection name:"), QLineEdit::Normal, defaultName, &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    name = name.trimmed();

    // Check for duplicate name and replace
    for (int i = 0; i < m_saved.size(); ++i) {
        if (m_saved[i].name == name) {
            m_saved.removeAt(i);
            break;
        }
    }

    SavedConnection c;
    c.name = name;
    c.host = host;
    c.port = m_port->value();
    c.username = user;
    c.useKeyAuth = m_keyAuth->isChecked();
    c.privateKeyPath = m_keyPath->text().trimmed();
    m_saved.append(c);

    // Persist
    QSettings settings("TextEd", "TextEd");
    settings.beginWriteArray("sshConnections", m_saved.size());
    for (int i = 0; i < m_saved.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("name", m_saved[i].name);
        settings.setValue("host", m_saved[i].host);
        settings.setValue("port", m_saved[i].port);
        settings.setValue("username", m_saved[i].username);
        settings.setValue("useKeyAuth", m_saved[i].useKeyAuth);
        settings.setValue("privateKeyPath", m_saved[i].privateKeyPath);
    }
    settings.endArray();

    populateList();
}

void SshConnectDialog::deleteConnection()
{
    int row = m_connectionList->currentRow();
    if (row < 0 || row >= m_saved.size()) return;

    m_saved.removeAt(row);

    QSettings settings("TextEd", "TextEd");
    settings.beginWriteArray("sshConnections", m_saved.size());
    for (int i = 0; i < m_saved.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("name", m_saved[i].name);
        settings.setValue("host", m_saved[i].host);
        settings.setValue("port", m_saved[i].port);
        settings.setValue("username", m_saved[i].username);
        settings.setValue("useKeyAuth", m_saved[i].useKeyAuth);
        settings.setValue("privateKeyPath", m_saved[i].privateKeyPath);
    }
    settings.endArray();

    populateList();
}

void SshConnectDialog::loadSelectedConnection()
{
    int row = m_connectionList->currentRow();
    if (row < 0 || row >= m_saved.size()) return;

    const auto &c = m_saved[row];
    m_host->setText(c.host);
    m_port->setValue(c.port);
    m_username->setText(c.username);
    m_password->clear();
    if (c.useKeyAuth) {
        m_keyAuth->setChecked(true);
        m_keyPath->setText(c.privateKeyPath);
    } else {
        m_passwordAuth->setChecked(true);
        m_keyPath->setText(c.privateKeyPath);
    }
}

void SshConnectDialog::loadSavedConnections()
{
    m_saved.clear();
    QSettings settings("TextEd", "TextEd");
    int count = settings.beginReadArray("sshConnections");
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        SavedConnection c;
        c.name = settings.value("name").toString();
        c.host = settings.value("host").toString();
        c.port = settings.value("port", 22).toInt();
        c.username = settings.value("username").toString();
        c.useKeyAuth = settings.value("useKeyAuth", false).toBool();
        c.privateKeyPath = settings.value("privateKeyPath").toString();
        m_saved.append(c);
    }
    settings.endArray();
}

void SshConnectDialog::populateList()
{
    m_connectionList->clear();
    for (const auto &c : m_saved) {
        m_connectionList->addItem(c.name);
    }
}
