#include "MarkdownPreview.h"
#include <QRegularExpression>
#include <QUrl>
#include <QDir>
#include <QFile>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QPageLayout>
#include <QPageSize>

MarkdownPreview::MarkdownPreview(QWidget *parent)
    : QWebEngineView(parent)
{
    setContextMenuPolicy(Qt::NoContextMenu);

    QWebEngineProfile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
    QWebEngineProfile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    QWebEngineProfile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);

    deployResources();
    m_htmlPath = m_tempDir.path() + "/preview.html";
}

void MarkdownPreview::deployResources()
{
    // Extract bundled katex/mermaid from qrc to temp dir
    struct Entry { const char *qrc; const char *rel; };
    static const Entry entries[] = {
        {":/resources/katex/katex.min.js", "katex/katex.min.js"},
        {":/resources/katex/katex.min.css", "katex/katex.min.css"},
        {":/resources/mermaid/mermaid.min.js", "mermaid/mermaid.min.js"},
        {":/resources/hljs/highlight.min.js", "hljs/highlight.min.js"},
        {":/resources/hljs/github.min.css", "hljs/github.min.css"},
    };

    QString base = m_tempDir.path();

    QDir(base).mkpath("katex/fonts");
    QDir(base).mkpath("mermaid");
    QDir(base).mkpath("hljs");

    // Copy main files
    for (const auto &e : entries) {
        QFile src(e.qrc);
        if (src.open(QIODevice::ReadOnly)) {
            QFile dst(base + "/" + e.rel);
            if (dst.open(QIODevice::WriteOnly)) {
                dst.write(src.readAll());
            }
        }
    }

    // Copy fonts
    QDir fontsQrc(":/resources/katex/fonts");
    for (const QString &name : fontsQrc.entryList(QDir::Files)) {
        QFile src(":/resources/katex/fonts/" + name);
        if (src.open(QIODevice::ReadOnly)) {
            QFile dst(base + "/katex/fonts/" + name);
            if (dst.open(QIODevice::WriteOnly)) {
                dst.write(src.readAll());
            }
        }
    }
}

void MarkdownPreview::updatePreview(const QString &markdownText)
{
    QString html = buildHtml(markdownText);

    QFile f(m_htmlPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(html.toUtf8());
        f.close();
    }

    setUrl(QUrl::fromLocalFile(m_htmlPath));
}

void MarkdownPreview::exportToPdf(const QString &filePath)
{
    QPageLayout layout(QPageSize(QPageSize::A4), QPageLayout::Portrait,
                       QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);
    page()->printToPdf(filePath, layout);
}

QString MarkdownPreview::markdownToHtml(const QString &md) const
{
    QString html;
    html.reserve(md.size());

    const QStringList lines = md.split('\n');
    bool inCodeBlock = false;
    bool inMermaid = false;
    QString codeContent;
    QString codeLang;
    bool inList = false;

    static const QRegularExpression reHead(QStringLiteral("^(#{1,6})\\s+(.*)"));
    static const QRegularExpression reHr(QStringLiteral("^(---|\\*\\*\\*|___)\\s*$"));
    static const QRegularExpression reUl(QStringLiteral("^\\s*[-*+]\\s+(.*)"));

    auto closePendingList = [&]() {
        if (inList) { html += QStringLiteral("</ul>\n"); inList = false; }
    };

    for (const QString &line : lines) {
        if (line.trimmed().startsWith(QStringLiteral("```"))) {
            if (!inCodeBlock) {
                closePendingList();
                inCodeBlock = true;
                codeLang = line.trimmed().mid(3).trimmed();
                inMermaid = (codeLang.compare(QStringLiteral("mermaid"), Qt::CaseInsensitive) == 0);
                codeContent.clear();
            } else {
                if (inMermaid) {
                    html += QStringLiteral("<div class=\"mermaid\">\n") + codeContent + QStringLiteral("</div>\n");
                } else if (!codeLang.isEmpty()) {
                    html += QStringLiteral("<pre><code class=\"language-") + codeLang.toHtmlEscaped() +
                            QStringLiteral("\">") + codeContent.toHtmlEscaped() + QStringLiteral("</code></pre>\n");
                } else {
                    html += QStringLiteral("<pre><code>") + codeContent.toHtmlEscaped() + QStringLiteral("</code></pre>\n");
                }
                inCodeBlock = false;
                inMermaid = false;
            }
            continue;
        }

        if (inCodeBlock) {
            codeContent += line + '\n';
            continue;
        }

        if (line.trimmed().isEmpty()) {
            closePendingList();
            html += QStringLiteral("<br>\n");
            continue;
        }

        auto mHead = reHead.match(line);
        if (mHead.hasMatch()) {
            closePendingList();
            int level = mHead.captured(1).length();
            html += QStringLiteral("<h%1>%2</h%1>\n").arg(level).arg(processInline(mHead.captured(2)));
            continue;
        }

        if (reHr.match(line.trimmed()).hasMatch()) {
            closePendingList();
            html += QStringLiteral("<hr>\n");
            continue;
        }

        if (line.trimmed().startsWith(QStringLiteral("> "))) {
            closePendingList();
            html += QStringLiteral("<blockquote>") + processInline(line.trimmed().mid(2)) + QStringLiteral("</blockquote>\n");
            continue;
        }

        auto mUl = reUl.match(line);
        if (mUl.hasMatch()) {
            if (!inList) { html += QStringLiteral("<ul>\n"); inList = true; }
            html += QStringLiteral("<li>") + processInline(mUl.captured(1)) + QStringLiteral("</li>\n");
            continue;
        }

        closePendingList();

        html += QStringLiteral("<p>") + processInline(line) + QStringLiteral("</p>\n");
    }

    closePendingList();
    return html;
}

QString MarkdownPreview::processInline(const QString &text)
{
    static const QRegularExpression reDispMath(QStringLiteral("\\$\\$(.+?)\\$\\$"));
    static const QRegularExpression reInlineMath(QStringLiteral("(?<!\\$)\\$(?!\\$)(.+?)(?<!\\$)\\$(?!\\$)"));
    static const QRegularExpression reBold(QStringLiteral("\\*\\*(.+?)\\*\\*"));
    static const QRegularExpression reItalic(QStringLiteral("(?<!\\*)\\*(?!\\*)(.+?)(?<!\\*)\\*(?!\\*)"));
    static const QRegularExpression reCode(QStringLiteral("`([^`]+)`"));
    static const QRegularExpression reLink(QStringLiteral("\\[([^\\]]+)\\]\\(([^)]+)\\)"));
    static const QRegularExpression reImg(QStringLiteral("!\\[([^\\]]*)]\\(([^)]+)\\)"));

    // Extract LaTeX placeholders before HTML-escaping
    struct Placeholder { qsizetype pos; qsizetype len; QString replacement; };
    QVector<Placeholder> placeholders;

    auto itD = reDispMath.globalMatch(text);
    while (itD.hasNext()) {
        auto m = itD.next();
        placeholders.append({m.capturedStart(), m.capturedLength(),
            QStringLiteral("<span class=\"katex-display\">") + m.captured(1) + QStringLiteral("</span>")});
    }

    auto itI = reInlineMath.globalMatch(text);
    while (itI.hasNext()) {
        auto m = itI.next();
        bool overlaps = false;
        for (const auto &p : placeholders) {
            if (m.capturedStart() >= p.pos && m.capturedStart() < p.pos + p.len) { overlaps = true; break; }
        }
        if (!overlaps) {
            placeholders.append({m.capturedStart(), m.capturedLength(),
                QStringLiteral("<span class=\"katex-inline\">") + m.captured(1) + QStringLiteral("</span>")});
        }
    }

    QString result;
    if (placeholders.isEmpty()) {
        result = text.toHtmlEscaped();
    } else {
        std::sort(placeholders.begin(), placeholders.end(),
                  [](const Placeholder &a, const Placeholder &b) { return a.pos < b.pos; });
        qsizetype lastEnd = 0;
        for (const auto &p : placeholders) {
            result += text.mid(lastEnd, p.pos - lastEnd).toHtmlEscaped();
            result += p.replacement;
            lastEnd = p.pos + p.len;
        }
        result += text.mid(lastEnd).toHtmlEscaped();
    }

    result.replace(reBold, QStringLiteral("<strong>\\1</strong>"));
    result.replace(reItalic, QStringLiteral("<em>\\1</em>"));
    result.replace(reCode, QStringLiteral("<code>\\1</code>"));
    result.replace(reLink, QStringLiteral("<a href=\"\\2\">\\1</a>"));
    result.replace(reImg, QStringLiteral("<img src=\"\\2\" alt=\"\\1\">"));

    return result;
}

QString MarkdownPreview::buildHtml(const QString &markdownText) const
{
    QString body = markdownToHtml(markdownText);

    QString base = QUrl::fromLocalFile(m_tempDir.path() + "/").toString();
    QString katexCss = base + "katex/katex.min.css";
    QString katexJs = base + "katex/katex.min.js";
    QString mermaidJs = base + "mermaid/mermaid.min.js";
    QString hljsJs = base + "hljs/highlight.min.js";
    QString hljsCss = base + "hljs/github.min.css";

    return QStringLiteral("<!DOCTYPE html>\n<html><head>\n<meta charset=\"utf-8\">\n"
        "<link rel=\"stylesheet\" href=\"") + katexCss + QStringLiteral("\">\n"
        "<link rel=\"stylesheet\" href=\"") + hljsCss + QStringLiteral("\">\n"
        "<script src=\"") + katexJs + QStringLiteral("\"></script>\n"
        "<script src=\"") + mermaidJs + QStringLiteral("\"></script>\n"
        "<script src=\"") + hljsJs + QStringLiteral("\"></script>\n"
        "<style>\n"
        "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;\n"
        "       max-width: 800px; margin: 0 auto; padding: 16px; line-height: 1.6; color: #24292f; }\n"
        "h1,h2,h3,h4,h5,h6 { color: #1a73e8; margin-top: 1em; }\n"
        "pre { background: #f6f8fa; padding: 12px; border-radius: 6px; overflow-x: auto; }\n"
        "code { background: #f0f0f0; padding: 2px 4px; border-radius: 3px; font-size: 0.9em; }\n"
        "pre code { background: none; padding: 0; }\n"
        "blockquote { border-left: 3px solid #ddd; margin-left: 0; padding-left: 12px; color: #636c76; }\n"
        "hr { border: none; border-top: 1px solid #ddd; }\n"
        "img { max-width: 100%; }\n"
        "a { color: #0969da; }\n"
        ".mermaid { text-align: center; }\n"
        ".mermaid[data-mermaid-error] { color: #b91c1c; font-style: italic; text-align: left; white-space: pre-wrap; }\n"
        "</style>\n</head><body>\n")
        + body +
        QStringLiteral("\n<script>\n"
        "// Render KaTeX\n"
        "document.querySelectorAll('.katex-display').forEach(function(el) {\n"
        "    try { katex.render(el.textContent, el, { displayMode: true, throwOnError: false }); }\n"
        "    catch(e) { el.textContent = el.textContent; }\n"
        "});\n"
        "document.querySelectorAll('.katex-inline').forEach(function(el) {\n"
        "    try { katex.render(el.textContent, el, { displayMode: false, throwOnError: false }); }\n"
        "    catch(e) { el.textContent = el.textContent; }\n"
        "});\n"
        "// Render code highlighting\n"
        "if (typeof hljs !== 'undefined') {\n"
        "    hljs.highlightAll();\n"
        "}\n"
        "// Render Mermaid\n"
        "if (typeof mermaid !== 'undefined') {\n"
        "    mermaid.initialize({ startOnLoad: false, theme: 'default', suppressErrors: true });\n"
        "    mermaid.run().catch(function(e) {\n"
        "        document.querySelectorAll('.mermaid').forEach(function(el) {\n"
        "            if (el.getAttribute('data-processed')) return;\n"
        "            el.style.color = '#b91c1c';\n"
        "            el.style.fontStyle = 'italic';\n"
        "            el.style.fontSize = '0.9em';\n"
        "            el.style.textAlign = 'left';\n"
        "            el.style.whiteSpace = 'pre-wrap';\n"
        "        });\n"
        "    });\n"
        "}\n"
        "</script>\n</body></html>");
}
