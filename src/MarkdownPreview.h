#pragma once
#include <QWidget>
#include <QWebEngineView>
#include <QTimer>

class MarkdownPreview : public QWidget
{
    Q_OBJECT
public:
    explicit MarkdownPreview(QWidget *parent = nullptr);

    void setDarkMode(bool dark);
    void zoomIn();
    void zoomOut();

public slots:
    void updatePreview(const QString &markdownText);
    void exportToPdf(const QString &filePath);

private:
    void deployResources();
    void render();
    QString markdownToHtml(const QString &md) const;
    QString buildHtml(const QString &body) const;
    static QString processInline(const QString &text);

    QWebEngineView *m_webView;
    QTimer *m_debounce;
    QString m_pendingMd;
    bool m_dark = false;

    QString m_tempDir;
    QString m_htmlPath;
};
