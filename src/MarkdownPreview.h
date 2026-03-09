#pragma once
#include <QWebEngineView>
#include <QTemporaryDir>
#include <QTemporaryFile>

class MarkdownPreview : public QWebEngineView
{
    Q_OBJECT
public:
    explicit MarkdownPreview(QWidget *parent = nullptr);

public slots:
    void updatePreview(const QString &markdownText);
    void exportToPdf(const QString &filePath);

private:
    QString buildHtml(const QString &markdownText) const;
    QString markdownToHtml(const QString &md) const;
    static QString processInline(const QString &text);
    void deployResources();

    QTemporaryDir m_tempDir;
    QString m_htmlPath;
    QString m_resDir; // path to katex/mermaid resources
};
