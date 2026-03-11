#include "RemoteFileBrowser.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QInputDialog>
#include <QMenu>

RemoteFileBrowser::RemoteFileBrowser(SshSession *ssh, QWidget *parent)
    : QDialog(parent), m_ssh(ssh)
{
    setWindowTitle(tr("Remote File Browser - %1").arg(ssh->hostLabel()));
    setMinimumSize(600, 450);

    auto *layout = new QVBoxLayout(this);

    // Path bar
    auto *pathLayout = new QHBoxLayout;
    auto *upBtn = new QPushButton(tr("Up"));
    connect(upBtn, &QPushButton::clicked, this, &RemoteFileBrowser::goUp);
    pathLayout->addWidget(upBtn);

    m_pathEdit = new QLineEdit;
    connect(m_pathEdit, &QLineEdit::returnPressed, this, [this]() {
        navigateTo(m_pathEdit->text().trimmed());
    });
    pathLayout->addWidget(m_pathEdit);

    auto *goBtn = new QPushButton(tr("Go"));
    connect(goBtn, &QPushButton::clicked, this, [this]() {
        navigateTo(m_pathEdit->text().trimmed());
    });
    pathLayout->addWidget(goBtn);

    layout->addLayout(pathLayout);

    // Toolbar for file operations
    auto *toolLayout = new QHBoxLayout;
    auto *newFileBtn = new QPushButton(tr("New File"));
    connect(newFileBtn, &QPushButton::clicked, this, &RemoteFileBrowser::createNewFile);
    toolLayout->addWidget(newFileBtn);

    auto *newDirBtn = new QPushButton(tr("New Folder"));
    connect(newDirBtn, &QPushButton::clicked, this, &RemoteFileBrowser::createNewDir);
    toolLayout->addWidget(newDirBtn);

    auto *renameBtn = new QPushButton(tr("Rename"));
    connect(renameBtn, &QPushButton::clicked, this, &RemoteFileBrowser::renameSelected);
    toolLayout->addWidget(renameBtn);

    auto *deleteBtn = new QPushButton(tr("Delete"));
    connect(deleteBtn, &QPushButton::clicked, this, &RemoteFileBrowser::deleteSelected);
    toolLayout->addWidget(deleteBtn);

    toolLayout->addStretch();
    layout->addLayout(toolLayout);

    // Tree
    m_tree = new QTreeWidget;
    m_tree->setHeaderLabels({tr("Name"), tr("Size"), tr("Permissions")});
    m_tree->setRootIsDecorated(false);
    m_tree->setAlternatingRowColors(true);
    m_tree->header()->setStretchLastSection(false);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, &RemoteFileBrowser::onItemDoubleClicked);
    connect(m_tree, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        auto *item = m_tree->itemAt(pos);
        if (!item || item->text(0) == "..") return;
        QMenu menu;
        menu.addAction(tr("Rename"), this, &RemoteFileBrowser::renameSelected);
        menu.addAction(tr("Delete"), this, &RemoteFileBrowser::deleteSelected);
        menu.exec(m_tree->viewport()->mapToGlobal(pos));
    });
    layout->addWidget(m_tree);

    // Filename
    auto *fnLayout = new QHBoxLayout;
    fnLayout->addWidget(new QLabel(tr("File name:")));
    m_fileNameEdit = new QLineEdit;
    fnLayout->addWidget(m_fileNameEdit);
    layout->addLayout(fnLayout);

    // Buttons
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel);
    m_okButton = buttons->addButton(tr("Open"), QDialogButtonBox::AcceptRole);
    connect(buttons, &QDialogButtonBox::accepted, this, &RemoteFileBrowser::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    // Connect tree selection to filename
    connect(m_tree, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem *item) {
        if (item && !item->data(0, Qt::UserRole + 1).toBool()) {
            m_fileNameEdit->setText(item->text(0));
        }
    });

    // Navigate to home
    m_currentDir = ssh->homeDir();
    navigateTo(m_currentDir);
}

void RemoteFileBrowser::setMode(Mode mode)
{
    m_mode = mode;
    m_okButton->setText(mode == Save ? tr("Save") : tr("Open"));
}

void RemoteFileBrowser::navigateTo(const QString &path)
{
    QString errorMsg;
    auto entries = m_ssh->listDir(path, &errorMsg);
    if (entries.isEmpty() && !errorMsg.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), errorMsg);
        return;
    }

    m_currentDir = path;
    m_pathEdit->setText(path);
    m_tree->clear();

    for (const auto &e : entries) {
        auto *item = new QTreeWidgetItem;
        item->setText(0, e.name);
        item->setText(1, e.isDir ? "" : formatSize(e.size));

        // Permission string
        QString perm;
        perm += (e.isDir ? 'd' : '-');
        perm += ((e.permissions & 0400) ? 'r' : '-');
        perm += ((e.permissions & 0200) ? 'w' : '-');
        perm += ((e.permissions & 0100) ? 'x' : '-');
        perm += ((e.permissions & 040) ? 'r' : '-');
        perm += ((e.permissions & 020) ? 'w' : '-');
        perm += ((e.permissions & 010) ? 'x' : '-');
        perm += ((e.permissions & 04) ? 'r' : '-');
        perm += ((e.permissions & 02) ? 'w' : '-');
        perm += ((e.permissions & 01) ? 'x' : '-');
        item->setText(2, perm);

        item->setData(0, Qt::UserRole, e.fullPath);
        item->setData(0, Qt::UserRole + 1, e.isDir);

        if (e.isDir) {
            item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
        } else {
            item->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
        }

        m_tree->addTopLevelItem(item);
    }
}

void RemoteFileBrowser::onItemDoubleClicked(QTreeWidgetItem *item, int)
{
    if (!item) return;
    bool isDir = item->data(0, Qt::UserRole + 1).toBool();
    QString path = item->data(0, Qt::UserRole).toString();

    if (isDir) {
        navigateTo(path);
    } else {
        m_selectedPath = path;
        accept();
    }
}

void RemoteFileBrowser::onAccept()
{
    QString name = m_fileNameEdit->text().trimmed();
    if (name.isEmpty()) {
        auto *item = m_tree->currentItem();
        if (item && !item->data(0, Qt::UserRole + 1).toBool()) {
            m_selectedPath = item->data(0, Qt::UserRole).toString();
            accept();
        }
        return;
    }

    // Build full path
    QString path = m_currentDir;
    if (!path.endsWith('/')) path += '/';
    path += name;
    m_selectedPath = path;
    accept();
}

void RemoteFileBrowser::goUp()
{
    if (m_currentDir == "/") return;
    QString parent = m_currentDir;
    if (parent.endsWith('/')) parent.chop(1);
    int idx = parent.lastIndexOf('/');
    if (idx <= 0) parent = "/";
    else parent = parent.left(idx);
    navigateTo(parent);
}

void RemoteFileBrowser::createNewFile()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("New File"), tr("File name:"),
        QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    name = name.trimmed();

    QString fullPath = m_currentDir;
    if (!fullPath.endsWith('/')) fullPath += '/';
    fullPath += name;

    QString errorMsg;
    if (!m_ssh->writeFile(fullPath, QByteArray(), &errorMsg)) {
        QMessageBox::warning(this, tr("Error"), errorMsg);
        return;
    }
    navigateTo(m_currentDir);
}

void RemoteFileBrowser::createNewDir()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("New Folder"), tr("Folder name:"),
        QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    name = name.trimmed();

    QString fullPath = m_currentDir;
    if (!fullPath.endsWith('/')) fullPath += '/';
    fullPath += name;

    QString errorMsg;
    if (!m_ssh->mkdir(fullPath, &errorMsg)) {
        QMessageBox::warning(this, tr("Error"), errorMsg);
        return;
    }
    navigateTo(m_currentDir);
}

void RemoteFileBrowser::renameSelected()
{
    auto *item = m_tree->currentItem();
    if (!item || item->text(0) == "..") return;

    QString oldName = item->text(0);
    QString oldPath = item->data(0, Qt::UserRole).toString();

    bool ok;
    QString newName = QInputDialog::getText(this, tr("Rename"), tr("New name:"),
        QLineEdit::Normal, oldName, &ok);
    if (!ok || newName.trimmed().isEmpty() || newName.trimmed() == oldName) return;
    newName = newName.trimmed();

    QString newPath = m_currentDir;
    if (!newPath.endsWith('/')) newPath += '/';
    newPath += newName;

    QString errorMsg;
    if (!m_ssh->rename(oldPath, newPath, &errorMsg)) {
        QMessageBox::warning(this, tr("Error"), errorMsg);
        return;
    }
    navigateTo(m_currentDir);
}

void RemoteFileBrowser::deleteSelected()
{
    auto *item = m_tree->currentItem();
    if (!item || item->text(0) == "..") return;

    QString name = item->text(0);
    QString path = item->data(0, Qt::UserRole).toString();
    bool isDir = item->data(0, Qt::UserRole + 1).toBool();

    auto r = QMessageBox::question(this, tr("Delete"),
        tr("Delete '%1'?").arg(name), QMessageBox::Yes | QMessageBox::No);
    if (r != QMessageBox::Yes) return;

    QString errorMsg;
    bool ok;
    if (isDir)
        ok = m_ssh->rmdir(path, &errorMsg);
    else
        ok = m_ssh->unlink(path, &errorMsg);

    if (!ok) {
        QMessageBox::warning(this, tr("Error"), errorMsg);
        return;
    }
    navigateTo(m_currentDir);
}

QString RemoteFileBrowser::formatSize(qint64 size) const
{
    if (size < 1024) return QString::number(size) + " B";
    if (size < 1024 * 1024) return QString::number(size / 1024.0, 'f', 1) + " KB";
    if (size < 1024LL * 1024 * 1024) return QString::number(size / (1024.0 * 1024.0), 'f', 1) + " MB";
    return QString::number(size / (1024.0 * 1024.0 * 1024.0), 'f', 1) + " GB";
}
