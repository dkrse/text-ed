#pragma once
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QStringConverter>
#include <functional>

class LargeFileReader
{
public:
    static QString readAll(const QString &path,
                           QStringConverter::Encoding encoding = QStringConverter::Utf8,
                           std::function<void(int)> progressCb = nullptr);

    static QByteArray readChunk(const QString &path, qint64 offset, qint64 length);
    static qint64 fileSize(const QString &path);

    // Try to auto-detect encoding from BOM or content
    static QStringConverter::Encoding detectEncoding(const QString &path);

    static constexpr qint64 CHUNK_SIZE = 4 * 1024 * 1024;

    // Encoding name helpers
    struct EncodingInfo {
        const char *name;
        QStringConverter::Encoding encoding;
    };
    static const QVector<EncodingInfo> &supportedEncodings();
    static QString encodingName(QStringConverter::Encoding enc);
    static QStringConverter::Encoding encodingFromName(const QString &name);
};
