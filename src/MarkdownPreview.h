#pragma once

#include <QWidget>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QTimer>
#include <QPageLayout>
#include <functional>

class MarkdownPreview : public QWidget
{
    Q_OBJECT
public:
    explicit MarkdownPreview(QWidget *parent = nullptr);
    ~MarkdownPreview();

    void setDarkMode(bool dark);
    void zoomIn();
    void zoomOut();
    void exportToPdf(const QString &filePath, int marginLeft, int marginRight,
                     const QString &pageNumbering, bool landscape, bool pageBorder);

public slots:
    void updatePreview(const QString &markdownText);

private:
    void deployResources();
    void render();
    QString markdownToHtml(const QString &md);
    void injectPrintCss(std::function<void()> then);
    void removePrintCss();
    void postProcessPdf(const QByteArray &pdfData, const QString &filePath,
                        const QPageLayout &layout, const QString &pageNumbering, bool pageBorder);

    QWebEngineView *m_webView;
    QTimer *m_debounce;
    QString m_pendingMd;
    bool m_dark = false;

    QString m_tempDir;
    QString m_htmlPath;
};
