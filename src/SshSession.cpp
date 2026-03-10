#include "SshSession.h"

#include <QDir>
#include <QFile>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>

bool SshSession::s_libInitialized = false;

SshSession::SshSession()
{
    if (!s_libInitialized) {
        libssh2_init(0);
        s_libInitialized = true;
    }
}

SshSession::~SshSession()
{
    disconnect();
}

bool SshSession::connect(const SshHostInfo &info, QString *errorMsg)
{
    disconnect();
    m_hostInfo = info;

    // Resolve hostname
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    QByteArray hostBytes = info.host.toUtf8();
    QByteArray portBytes = QString::number(info.port).toUtf8();

    int gai = getaddrinfo(hostBytes.constData(), portBytes.constData(), &hints, &res);
    if (gai != 0 || !res) {
        if (errorMsg) *errorMsg = QString("Could not resolve host: %1").arg(gai_strerror(gai));
        return false;
    }

    m_socket = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (m_socket < 0) {
        if (errorMsg) *errorMsg = QString("Socket creation failed: %1").arg(strerror(errno));
        freeaddrinfo(res);
        return false;
    }

    // Set a connection timeout of 10 seconds
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if (::connect(m_socket, res->ai_addr, res->ai_addrlen) != 0) {
        if (errorMsg) *errorMsg = QString("Connection failed: %1").arg(strerror(errno));
        freeaddrinfo(res);
        ::close(m_socket);
        m_socket = -1;
        return false;
    }
    freeaddrinfo(res);

    // Create SSH session
    m_session = libssh2_session_init();
    if (!m_session) {
        if (errorMsg) *errorMsg = "Failed to create SSH session";
        ::close(m_socket);
        m_socket = -1;
        return false;
    }

    libssh2_session_set_blocking(m_session, 1);
    libssh2_session_set_timeout(m_session, 15000);

    if (libssh2_session_handshake(m_session, m_socket) != 0) {
        if (errorMsg) *errorMsg = QString("SSH handshake failed: %1").arg(lastSsh2Error());
        disconnect();
        return false;
    }

    // Authenticate
    QByteArray user = info.username.toUtf8();
    int authRc = -1;

    if (info.useKeyAuth && !info.privateKeyPath.isEmpty()) {
        QByteArray keyPath = info.privateKeyPath.toUtf8();
        QByteArray pass = info.password.toUtf8();
        authRc = libssh2_userauth_publickey_fromfile(
            m_session, user.constData(),
            nullptr, keyPath.constData(),
            pass.isEmpty() ? nullptr : pass.constData());
    } else {
        QByteArray pass = info.password.toUtf8();
        authRc = libssh2_userauth_password(m_session, user.constData(), pass.constData());
    }

    if (authRc != 0) {
        if (errorMsg) *errorMsg = QString("Authentication failed: %1").arg(lastSsh2Error());
        disconnect();
        return false;
    }

    // Init SFTP
    if (!initSftp(errorMsg)) {
        disconnect();
        return false;
    }

    // Determine home directory
    m_homeDir = "/home/" + info.username;
    if (info.username == "root") m_homeDir = "/root";

    // Use a channel to run echo $HOME
    LIBSSH2_CHANNEL *ch = libssh2_channel_open_session(m_session);
    if (ch) {
        if (libssh2_channel_exec(ch, "echo $HOME") == 0) {
            char buf[512];
            int n = libssh2_channel_read(ch, buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = '\0';
                QString home = QString::fromUtf8(buf).trimmed();
                if (!home.isEmpty()) m_homeDir = home;
            }
        }
        libssh2_channel_close(ch);
        libssh2_channel_free(ch);
    }

    return true;
}

void SshSession::disconnect()
{
    if (m_sftp) {
        libssh2_sftp_shutdown(m_sftp);
        m_sftp = nullptr;
    }
    if (m_session) {
        libssh2_session_disconnect(m_session, "Normal shutdown");
        libssh2_session_free(m_session);
        m_session = nullptr;
    }
    if (m_socket >= 0) {
        ::close(m_socket);
        m_socket = -1;
    }
    m_homeDir.clear();
}

bool SshSession::isConnected() const
{
    return m_session && m_sftp && m_socket >= 0;
}

QString SshSession::hostLabel() const
{
    if (!isConnected()) return QString();
    return m_hostInfo.username + "@" + m_hostInfo.host;
}

bool SshSession::initSftp(QString *errorMsg)
{
    m_sftp = libssh2_sftp_init(m_session);
    if (!m_sftp) {
        if (errorMsg) *errorMsg = QString("SFTP init failed: %1").arg(lastSsh2Error());
        return false;
    }
    return true;
}

QString SshSession::lastSsh2Error() const
{
    if (!m_session) return "No session";
    char *msg = nullptr;
    int len = 0;
    libssh2_session_last_error(m_session, &msg, &len, 0);
    return QString::fromUtf8(msg, len);
}

QList<RemoteFileEntry> SshSession::listDir(const QString &remotePath, QString *errorMsg)
{
    QList<RemoteFileEntry> entries;
    if (!m_sftp) {
        if (errorMsg) *errorMsg = "Not connected";
        return entries;
    }

    QByteArray pathBytes = remotePath.toUtf8();
    LIBSSH2_SFTP_HANDLE *dir = libssh2_sftp_opendir(m_sftp, pathBytes.constData());
    if (!dir) {
        if (errorMsg) *errorMsg = QString("Cannot open directory: %1").arg(lastSsh2Error());
        return entries;
    }

    char buf[4096];
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    while (libssh2_sftp_readdir(dir, buf, sizeof(buf), &attrs) > 0) {
        QString name = QString::fromUtf8(buf);
        if (name == ".") continue;

        RemoteFileEntry e;
        e.name = name;
        e.fullPath = remotePath;
        if (!e.fullPath.endsWith('/')) e.fullPath += '/';
        e.fullPath += name;
        e.isDir = LIBSSH2_SFTP_S_ISDIR(attrs.permissions);
        e.size = static_cast<qint64>(attrs.filesize);
        e.permissions = attrs.permissions;
        entries.append(e);
    }

    libssh2_sftp_closedir(dir);

    // Sort: directories first, then by name
    std::sort(entries.begin(), entries.end(), [](const RemoteFileEntry &a, const RemoteFileEntry &b) {
        if (a.name == "..") return true;
        if (b.name == "..") return false;
        if (a.isDir != b.isDir) return a.isDir;
        return a.name.toLower() < b.name.toLower();
    });

    return entries;
}

QByteArray SshSession::readFile(const QString &remotePath, QString *errorMsg)
{
    if (!m_sftp) {
        if (errorMsg) *errorMsg = "Not connected";
        return {};
    }

    QByteArray pathBytes = remotePath.toUtf8();
    LIBSSH2_SFTP_HANDLE *file = libssh2_sftp_open(m_sftp, pathBytes.constData(),
        LIBSSH2_FXF_READ, 0);
    if (!file) {
        if (errorMsg) *errorMsg = QString("Cannot open file: %1").arg(lastSsh2Error());
        return {};
    }

    QByteArray data;
    char buf[32768];
    ssize_t n;
    constexpr qint64 maxFileSize = 512 * 1024 * 1024; // 512 MB limit
    while ((n = libssh2_sftp_read(file, buf, sizeof(buf))) > 0) {
        data.append(buf, static_cast<int>(n));
        if (data.size() > maxFileSize) {
            if (errorMsg) *errorMsg = "File too large (>512 MB)";
            libssh2_sftp_close(file);
            return {};
        }
    }

    if (n < 0) {
        if (errorMsg) *errorMsg = QString("Read error: %1").arg(lastSsh2Error());
    }

    libssh2_sftp_close(file);
    return data;
}

bool SshSession::writeFile(const QString &remotePath, const QByteArray &data, QString *errorMsg)
{
    if (!m_sftp) {
        if (errorMsg) *errorMsg = "Not connected";
        return false;
    }

    QByteArray pathBytes = remotePath.toUtf8();
    LIBSSH2_SFTP_HANDLE *file = libssh2_sftp_open(m_sftp, pathBytes.constData(),
        LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
        LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR | LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH);
    if (!file) {
        if (errorMsg) *errorMsg = QString("Cannot open file for writing: %1").arg(lastSsh2Error());
        return false;
    }

    const char *ptr = data.constData();
    qint64 remaining = data.size();
    while (remaining > 0) {
        ssize_t written = libssh2_sftp_write(file, ptr, static_cast<size_t>(remaining));
        if (written < 0) {
            if (errorMsg) *errorMsg = QString("Write error: %1").arg(lastSsh2Error());
            libssh2_sftp_close(file);
            return false;
        }
        ptr += written;
        remaining -= written;
    }

    libssh2_sftp_close(file);
    return true;
}

bool SshSession::fileExists(const QString &remotePath)
{
    if (!m_sftp) return false;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    QByteArray pathBytes = remotePath.toUtf8();
    return libssh2_sftp_stat(m_sftp, pathBytes.constData(), &attrs) == 0;
}

QString SshSession::homeDir()
{
    return m_homeDir;
}
