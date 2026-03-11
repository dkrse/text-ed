#pragma once
#include <QDialog>
#include "SshSession.h"

class QTreeWidget;
class QTreeWidgetItem;
class QLineEdit;
class QPushButton;

class RemoteFileBrowser : public QDialog
{
    Q_OBJECT
public:
    explicit RemoteFileBrowser(SshSession *ssh, QWidget *parent = nullptr);

    QString selectedPath() const { return m_selectedPath; }
    enum Mode { Open, Save };
    void setMode(Mode mode);

private slots:
    void navigateTo(const QString &path);
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onAccept();
    void goUp();
    void createNewFile();
    void createNewDir();
    void renameSelected();
    void deleteSelected();

private:
    void populateList();
    QString formatSize(qint64 size) const;

    SshSession *m_ssh;
    QTreeWidget *m_tree;
    QLineEdit *m_pathEdit;
    QLineEdit *m_fileNameEdit;
    QPushButton *m_okButton;
    QString m_currentDir;
    QString m_selectedPath;
    Mode m_mode = Open;
};
