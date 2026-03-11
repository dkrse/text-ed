#pragma once
#include <QString>
#include <QStringList>
#include <libssh2.h>
#include <libssh2_sftp.h>

struct SshHostInfo {
    QString host;
    int port = 22;
    QString username;
    QString password;
    QString privateKeyPath;
    bool useKeyAuth = false;
};

struct RemoteFileEntry {
    QString name;
    QString fullPath;
    bool isDir = false;
    qint64 size = 0;
    unsigned long permissions = 0;
};

class SshSession
{
public:
    SshSession();
    ~SshSession();

    SshSession(const SshSession &) = delete;
    SshSession &operator=(const SshSession &) = delete;

    bool connect(const SshHostInfo &info, QString *errorMsg = nullptr);
    void disconnect();
    bool isConnected() const;

    QString hostLabel() const;

    // SFTP operations
    QList<RemoteFileEntry> listDir(const QString &remotePath, QString *errorMsg = nullptr);
    QByteArray readFile(const QString &remotePath, QString *errorMsg = nullptr);
    bool writeFile(const QString &remotePath, const QByteArray &data, QString *errorMsg = nullptr);
    bool fileExists(const QString &remotePath);
    bool mkdir(const QString &remotePath, QString *errorMsg = nullptr);
    bool rmdir(const QString &remotePath, QString *errorMsg = nullptr);
    bool unlink(const QString &remotePath, QString *errorMsg = nullptr);
    bool rename(const QString &oldPath, const QString &newPath, QString *errorMsg = nullptr);
    QString homeDir();

private:
    bool initSftp(QString *errorMsg);
    QString lastSsh2Error() const;

    int m_socket = -1;
    LIBSSH2_SESSION *m_session = nullptr;
    LIBSSH2_SFTP *m_sftp = nullptr;
    SshHostInfo m_hostInfo;
    QString m_homeDir;

    static bool s_libInitialized;
};
