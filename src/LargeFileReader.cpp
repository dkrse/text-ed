#include "LargeFileReader.h"
#include <QFileInfo>

const QVector<LargeFileReader::EncodingInfo> &LargeFileReader::supportedEncodings()
{
    static const QVector<EncodingInfo> encodings = {
        {"UTF-8",         QStringConverter::Utf8},
        {"UTF-16",        QStringConverter::Utf16},
        {"UTF-16BE",      QStringConverter::Utf16BE},
        {"UTF-16LE",      QStringConverter::Utf16LE},
        {"UTF-32",        QStringConverter::Utf32},
        {"UTF-32BE",      QStringConverter::Utf32BE},
        {"UTF-32LE",      QStringConverter::Utf32LE},
        {"Latin-1",       QStringConverter::Latin1},
        {"System",        QStringConverter::System},
    };
    return encodings;
}

QString LargeFileReader::encodingName(QStringConverter::Encoding enc)
{
    for (const auto &e : supportedEncodings()) {
        if (e.encoding == enc) return e.name;
    }
    return "UTF-8";
}

QStringConverter::Encoding LargeFileReader::encodingFromName(const QString &name)
{
    for (const auto &e : supportedEncodings()) {
        if (name == e.name) return e.encoding;
    }
    return QStringConverter::Utf8;
}

QStringConverter::Encoding LargeFileReader::detectEncoding(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QStringConverter::Utf8;

    QByteArray header = file.read(4);
    file.close();

    if (header.size() < 2)
        return QStringConverter::Utf8;

    // Check BOM
    unsigned char b0 = header[0], b1 = header[1];
    if (b0 == 0xEF && b1 == 0xBB && header.size() >= 3 && (unsigned char)header[2] == 0xBF)
        return QStringConverter::Utf8;
    if (b0 == 0xFF && b1 == 0xFE) {
        if (header.size() >= 4 && header[2] == 0x00 && header[3] == 0x00)
            return QStringConverter::Utf32LE;
        return QStringConverter::Utf16LE;
    }
    if (b0 == 0xFE && b1 == 0xFF)
        return QStringConverter::Utf16BE;
    if (header.size() >= 4 && b0 == 0x00 && b1 == 0x00 &&
        (unsigned char)header[2] == 0xFE && (unsigned char)header[3] == 0xFF)
        return QStringConverter::Utf32BE;

    return QStringConverter::Utf8;
}

QString LargeFileReader::readAll(const QString &path,
                                  QStringConverter::Encoding encoding,
                                  std::function<void(int)> progressCb)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    const qint64 size = file.size();
    if (size == 0)
        return QString();

    QTextStream in(&file);
    in.setEncoding(encoding);

    if (size < CHUNK_SIZE) {
        return in.readAll();
    }

    QString result;
    result.reserve(static_cast<qsizetype>(size / 2));

    qint64 bytesRead = 0;
    int lastPct = 0;
    while (!in.atEnd()) {
        QString chunk = in.read(CHUNK_SIZE);
        result += chunk;

        bytesRead = file.pos();
        if (progressCb) {
            int pct = static_cast<int>(bytesRead * 100 / size);
            if (pct != lastPct) {
                lastPct = pct;
                progressCb(pct);
            }
        }
    }
    return result;
}

QByteArray LargeFileReader::readChunk(const QString &path, qint64 offset, qint64 length)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    file.seek(offset);
    return file.read(length);
}

qint64 LargeFileReader::fileSize(const QString &path)
{
    return QFileInfo(path).size();
}
