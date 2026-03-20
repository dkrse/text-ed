#include "MarkdownPreview.h"
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QUrl>
#include <QDir>
#include <QFile>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QPageLayout>
#include <QPageSize>

MarkdownPreview::MarkdownPreview(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_webView = new QWebEngineView(this);
    m_webView->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    m_webView->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
    m_webView->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    m_webView->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, false);
    m_webView->settings()->setAttribute(QWebEngineSettings::PdfViewerEnabled, false);
    m_webView->settings()->setAttribute(QWebEngineSettings::NavigateOnDropEnabled, false);
    layout->addWidget(m_webView);

    deployResources();
    m_htmlPath = m_tempDir + "/preview.html";

    m_debounce = new QTimer(this);
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(400);
    connect(m_debounce, &QTimer::timeout, this, &MarkdownPreview::render);
}

void MarkdownPreview::deployResources()
{
    m_tempDir = QDir::tempPath() + "/text-ed-preview";
    QDir().mkpath(m_tempDir + "/katex/fonts");
    QDir().mkpath(m_tempDir + "/mermaid");
    QDir().mkpath(m_tempDir + "/hljs");

    auto extract = [](const QString &resPath, const QString &outPath) {
        QFile res(resPath);
        if (!res.open(QIODevice::ReadOnly)) return;
        QByteArray data = res.readAll();
        if (QFile::exists(outPath)) {
            QFile existing(outPath);
            if (existing.open(QIODevice::ReadOnly) && existing.readAll() == data)
                return;
        }
        QFile out(outPath);
        if (out.open(QIODevice::WriteOnly))
            out.write(data);
    };

    struct Entry { const char *qrc; const char *rel; };
    static const Entry entries[] = {
        {":/resources/katex/katex.min.js",   "katex/katex.min.js"},
        {":/resources/katex/katex.min.css",  "katex/katex.min.css"},
        {":/resources/mermaid/mermaid.min.js","mermaid/mermaid.min.js"},
        {":/resources/hljs/highlight.min.js","hljs/highlight.min.js"},
        {":/resources/hljs/github.min.css", "hljs/github.min.css"},
    };

    for (const auto &e : entries)
        extract(e.qrc, m_tempDir + "/" + e.rel);

    QDir fontsQrc(":/resources/katex/fonts");
    for (const QString &name : fontsQrc.entryList(QDir::Files))
        extract(":/resources/katex/fonts/" + name, m_tempDir + "/katex/fonts/" + name);
}

void MarkdownPreview::setDarkMode(bool dark)
{
    m_dark = dark;
    if (!m_pendingMd.isEmpty())
        render();
}

void MarkdownPreview::zoomIn()
{
    m_webView->setZoomFactor(qMin(m_webView->zoomFactor() + 0.1, 5.0));
}

void MarkdownPreview::zoomOut()
{
    m_webView->setZoomFactor(qMax(m_webView->zoomFactor() - 0.1, 0.25));
}

void MarkdownPreview::updatePreview(const QString &markdownText)
{
    m_pendingMd = markdownText;
    m_debounce->start();
}

void MarkdownPreview::render()
{
    QString body = markdownToHtml(m_pendingMd);

    // Convert mermaid code blocks to <div class="mermaid">
    body.replace(QRegularExpression(
        R"(<pre><code class="language-mermaid">([\s\S]*?)</code></pre>)"),
        R"(<div class="mermaid">\1</div>)");

    QString html = buildHtml(body);

    QFile f(m_htmlPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(html.toUtf8());
        f.close();
    }
    m_webView->load(QUrl::fromLocalFile(m_htmlPath));
}

void MarkdownPreview::exportToPdf(const QString &filePath)
{
    QPageLayout layout(QPageSize(QPageSize::A4), QPageLayout::Portrait,
                       QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);
    m_webView->page()->printToPdf(filePath, layout);
}

// ── Markdown → HTML ─────────────────────────────────────────────────

QString MarkdownPreview::markdownToHtml(const QString &md) const
{
    // Protect code blocks and math from line-by-line processing
    static QRegularExpression reCodeFence(R"(^```[\s\S]*?^```)", QRegularExpression::MultilineOption);
    static QRegularExpression reCodeSpan(R"(`[^`]+`)");
    static QRegularExpression reDisplayMath(R"(\$\$[\s\S]+?\$\$)");
    static QRegularExpression reInlineMathML(R"(\$(?!\s)(?:[^$\\]|\\.|\n)+?\$)");

    QVector<QString> protectedFragments;
    QString work = md;

    // Step 1: mask fenced code blocks, then inline code spans
    auto maskFragments = [&](const QRegularExpression &re) {
        QRegularExpressionMatchIterator it = re.globalMatch(work);
        QVector<QRegularExpressionMatch> matches;
        while (it.hasNext()) matches.append(it.next());
        for (int i = matches.size() - 1; i >= 0; --i) {
            const auto &m = matches[i];
            QString ph = QString("\x05PH%1\x05").arg(protectedFragments.size());
            protectedFragments.append(m.captured());
            work.replace(m.capturedStart(), m.capturedLength(), ph);
        }
    };
    maskFragments(reCodeFence);
    maskFragments(reCodeSpan);

    // Step 2: protect multi-line math (display $$...$$ and inline $...$)
    QVector<QString> mathFragments;
    auto protectMath = [&](const QRegularExpression &re) {
        QRegularExpressionMatchIterator it = re.globalMatch(work);
        QVector<QRegularExpressionMatch> matches;
        while (it.hasNext()) matches.append(it.next());
        for (int i = matches.size() - 1; i >= 0; --i) {
            const auto &m = matches[i];
            QString ph = QString("\x02MATH%1\x02").arg(mathFragments.size());
            mathFragments.append(m.captured());
            work.replace(m.capturedStart(), m.capturedLength(), ph);
        }
    };
    protectMath(reDisplayMath);
    protectMath(reInlineMathML);

    // Step 3: restore code blocks (they are safe from math replacement)
    for (int i = 0; i < protectedFragments.size(); ++i)
        work.replace(QString("\x05PH%1\x05").arg(i), protectedFragments[i]);

    // Now do line-by-line markdown conversion on 'work'
    QString html;
    html.reserve(work.size());

    const QStringList lines = work.split('\n');
    bool inCodeBlock = false;
    bool inMermaid = false;
    QString codeContent;
    QString codeLang;
    bool inUl = false;
    bool inOl = false;
    bool inBlockquote = false;
    bool inTable = false;
    bool hadTableSep = false;
    QVector<QString> tableAligns;

    static const QRegularExpression reHead(QStringLiteral("^(#{1,6})\\s+(.*)"));
    static const QRegularExpression reHr(QStringLiteral("^(---|\\*\\*\\*|___)\\s*$"));
    static const QRegularExpression reUl(QStringLiteral("^\\s*[-*+]\\s+(.*)"));
    static const QRegularExpression reOl(QStringLiteral("^\\s*\\d+\\.\\s+(.*)"));

    auto closePending = [&]() {
        if (inUl) { html += QStringLiteral("</ul>\n"); inUl = false; }
        if (inOl) { html += QStringLiteral("</ol>\n"); inOl = false; }
        if (inBlockquote) { html += QStringLiteral("</blockquote>\n"); inBlockquote = false; }
        if (inTable) {
            html += (hadTableSep ? QStringLiteral("</tbody>") : QString()) + QStringLiteral("</table>\n");
            inTable = false; hadTableSep = false;
        }
    };

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();

        // Fenced code blocks
        if (trimmed.startsWith(QStringLiteral("```"))) {
            if (!inCodeBlock) {
                closePending();
                inCodeBlock = true;
                codeLang = trimmed.mid(3).trimmed();
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
        if (inCodeBlock) { codeContent += line + '\n'; continue; }

        // Blank line
        if (trimmed.isEmpty()) { closePending(); html += QStringLiteral("<br>\n"); continue; }

        // Headings
        auto mHead = reHead.match(trimmed);
        if (mHead.hasMatch()) {
            closePending();
            int level = mHead.captured(1).length();
            html += QStringLiteral("<h%1>%2</h%1>\n").arg(level).arg(processInline(mHead.captured(2)));
            continue;
        }

        // Horizontal rule
        if (reHr.match(trimmed).hasMatch()) { closePending(); html += QStringLiteral("<hr>\n"); continue; }

        // Table
        if (trimmed.startsWith('|') && trimmed.endsWith('|')) {
            QStringList cells;
            for (const auto &cell : trimmed.split('|', Qt::SkipEmptyParts))
                cells << cell.trimmed();

            bool isSep = !cells.isEmpty();
            for (const auto &c : cells) {
                QString t = c; t.remove('-'); t.remove(':'); t.remove(' ');
                if (!t.isEmpty()) { isSep = false; break; }
            }

            if (!inTable) {
                closePending();
                inTable = true; hadTableSep = false; tableAligns.clear();
                html += QStringLiteral("<table><thead><tr>");
                for (const auto &h : cells)
                    html += QStringLiteral("<th>") + processInline(h) + QStringLiteral("</th>");
                html += QStringLiteral("</tr></thead>\n");
            } else if (isSep && !hadTableSep) {
                hadTableSep = true; tableAligns.clear();
                for (const auto &s : cells) {
                    QString t = s.trimmed();
                    if (t.startsWith(':') && t.endsWith(':')) tableAligns << "center";
                    else if (t.endsWith(':')) tableAligns << "right";
                    else tableAligns << "left";
                }
                html += QStringLiteral("<tbody>\n");
            } else if (hadTableSep) {
                html += QStringLiteral("<tr>");
                for (int c = 0; c < cells.size(); ++c) {
                    QString align = (c < tableAligns.size()) ? tableAligns[c] : "left";
                    html += QStringLiteral("<td style=\"text-align:") + align + QStringLiteral("\">") +
                            processInline(cells[c]) + QStringLiteral("</td>");
                }
                html += QStringLiteral("</tr>\n");
            }
            continue;
        }
        if (inTable) {
            html += (hadTableSep ? QStringLiteral("</tbody>") : QString()) + QStringLiteral("</table>\n");
            inTable = false; hadTableSep = false;
        }

        // Blockquote
        if (trimmed.startsWith('>')) {
            QString content = trimmed.mid(1);
            if (content.startsWith(' ')) content = content.mid(1);
            if (!inBlockquote) {
                if (inUl) { html += QStringLiteral("</ul>\n"); inUl = false; }
                if (inOl) { html += QStringLiteral("</ol>\n"); inOl = false; }
                inBlockquote = true;
                html += QStringLiteral("<blockquote>\n");
            }
            html += QStringLiteral("<p>") + processInline(content) + QStringLiteral("</p>\n");
            continue;
        }
        if (inBlockquote) { html += QStringLiteral("</blockquote>\n"); inBlockquote = false; }

        // Unordered list
        auto mUl = reUl.match(line);
        if (mUl.hasMatch()) {
            if (inOl) { html += QStringLiteral("</ol>\n"); inOl = false; }
            if (!inUl) { inUl = true; html += QStringLiteral("<ul>\n"); }
            html += QStringLiteral("<li>") + processInline(mUl.captured(1)) + QStringLiteral("</li>\n");
            continue;
        }

        // Ordered list
        auto mOl = reOl.match(line);
        if (mOl.hasMatch()) {
            if (inUl) { html += QStringLiteral("</ul>\n"); inUl = false; }
            if (!inOl) { inOl = true; html += QStringLiteral("<ol>\n"); }
            html += QStringLiteral("<li>") + processInline(mOl.captured(1)) + QStringLiteral("</li>\n");
            continue;
        }

        // Paragraph
        closePending();
        html += QStringLiteral("<p>") + processInline(trimmed) + QStringLiteral("</p>\n");
    }

    if (inCodeBlock) {
        if (inMermaid)
            html += QStringLiteral("<div class=\"mermaid\">\n") + codeContent + QStringLiteral("</div>\n");
        else
            html += QStringLiteral("<pre><code>") + codeContent.toHtmlEscaped() + QStringLiteral("</code></pre>\n");
    }
    closePending();

    // Restore math placeholders as KaTeX spans
    for (int i = 0; i < mathFragments.size(); ++i) {
        QString ph = QString("\x02MATH%1\x02").arg(i);
        QString raw = mathFragments[i];
        // Remove <p> wrapper if the placeholder got wrapped
        html.replace("<p>" + ph + "</p>", ph);

        if (raw.startsWith("$$") && raw.endsWith("$$")) {
            QString inner = raw.mid(2, raw.length() - 4);
            html.replace(ph, QStringLiteral("<span class=\"katex-display\">") + inner + QStringLiteral("</span>"));
        } else if (raw.startsWith("$") && raw.endsWith("$")) {
            QString inner = raw.mid(1, raw.length() - 2);
            html.replace(ph, QStringLiteral("<span class=\"katex-inline\">") + inner + QStringLiteral("</span>"));
        } else {
            html.replace(ph, raw.toHtmlEscaped());
        }
    }

    return html;
}

QString MarkdownPreview::processInline(const QString &text)
{
    static const QRegularExpression reDispMath(QStringLiteral("\\$\\$(.+?)\\$\\$"));
    static const QRegularExpression reInlineMath(QStringLiteral("(?<!\\$)\\$(?!\\$)(.+?)(?<!\\$)\\$(?!\\$)"));
    static const QRegularExpression reBold(QStringLiteral("\\*\\*(.+?)\\*\\*"));
    static const QRegularExpression reItalic(QStringLiteral("(?<!\\*)\\*(?!\\*)(.+?)(?<!\\*)\\*(?!\\*)"));
    static const QRegularExpression reCode(QStringLiteral("`([^`]+)`"));
    static const QRegularExpression reStrike(QStringLiteral("~~(.+?)~~"));
    static const QRegularExpression reImg(QStringLiteral("!\\[([^\\]]*)\\]\\(([^)]+)\\)"));
    static const QRegularExpression reLink(QStringLiteral("\\[([^\\]]+)\\]\\(([^)]+)\\)"));

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

    // Protect inline code from bold/italic processing
    QVector<QString> codeFragments;
    {
        QRegularExpressionMatchIterator it = reCode.globalMatch(result);
        QVector<QRegularExpressionMatch> codeMatches;
        while (it.hasNext()) codeMatches.append(it.next());
        for (int ci = codeMatches.size() - 1; ci >= 0; --ci) {
            const auto &m = codeMatches[ci];
            QString placeholder = QStringLiteral("\x04CODE") + QString::number(codeFragments.size()) + QStringLiteral("\x04");
            codeFragments.append(QStringLiteral("<code>") + m.captured(1) + QStringLiteral("</code>"));
            result.replace(m.capturedStart(), m.capturedLength(), placeholder);
        }
    }

    result.replace(reBold, QStringLiteral("<strong>\\1</strong>"));
    result.replace(reItalic, QStringLiteral("<em>\\1</em>"));
    result.replace(reStrike, QStringLiteral("<del>\\1</del>"));
    result.replace(reImg, QStringLiteral("<img src=\"\\2\" alt=\"\\1\" style=\"max-width:100%\">"));
    result.replace(reLink, QStringLiteral("<a href=\"\\2\">\\1</a>"));

    for (int ci = 0; ci < codeFragments.size(); ++ci)
        result.replace(QStringLiteral("\x04CODE") + QString::number(ci) + QStringLiteral("\x04"), codeFragments[ci]);

    return result;
}

QString MarkdownPreview::buildHtml(const QString &body) const
{
    QString bg      = m_dark ? "#1e1e1e" : "#ffffff";
    QString fg      = m_dark ? "#d4d4d4" : "#24292f";
    QString codeBg  = m_dark ? "#2d2d2d" : "#f6f8fa";
    QString codeFg  = m_dark ? "#ce9178" : "#c7254e";
    QString borderC = m_dark ? "#3e3e3e" : "#d0d0d0";
    QString linkC   = m_dark ? "#61afef" : "#0366d6";
    QString headC   = m_dark ? "#e5c07b" : "#24292f";
    QString bqBorder= m_dark ? "#565656" : "#dfe2e5";
    QString bqFg    = m_dark ? "#9e9e9e" : "#6a737d";
    QString hrC     = m_dark ? "#3e3e3e" : "#e1e4e8";
    QString thBg    = m_dark ? "#2d2d2d" : "#f6f8fa";
    QString mermaidTheme = m_dark ? "dark" : "default";

    QString base = QUrl::fromLocalFile(m_tempDir + "/").toString();
    QString katexCss = base + "katex/katex.min.css";
    QString katexJs  = base + "katex/katex.min.js";
    QString mermaidJs= base + "mermaid/mermaid.min.js";
    QString hljsJs   = base + "hljs/highlight.min.js";
    QString hljsCss  = base + "hljs/github.min.css";

    return QStringLiteral("<!DOCTYPE html>\n<html><head>\n<meta charset=\"utf-8\">\n"
        "<link rel=\"stylesheet\" href=\"") + katexCss + QStringLiteral("\">\n"
        "<link rel=\"stylesheet\" href=\"") + hljsCss + QStringLiteral("\">\n"
        "<script src=\"") + katexJs + QStringLiteral("\"></script>\n"
        "<script src=\"") + mermaidJs + QStringLiteral("\"></script>\n"
        "<script src=\"") + hljsJs + QStringLiteral("\"></script>\n"
        "<style>\n"
        "body { background: ") + bg + QStringLiteral("; color: ") + fg + QStringLiteral(";\n"
        "  font-family: -apple-system, 'Segoe UI', Helvetica, Arial, sans-serif;\n"
        "  font-size: 15px; line-height: 1.6; padding: 16px 24px; margin: 0; word-wrap: break-word; }\n"
        "h1,h2,h3,h4,h5,h6 { color: ") + headC + QStringLiteral("; margin-top: 24px; margin-bottom: 8px; font-weight: 600; }\n"
        "h1 { font-size: 2em; border-bottom: 1px solid ") + borderC + QStringLiteral("; padding-bottom: 0.3em; }\n"
        "h2 { font-size: 1.5em; border-bottom: 1px solid ") + borderC + QStringLiteral("; padding-bottom: 0.3em; }\n"
        "h3 { font-size: 1.25em; }\n"
        "p { margin: 8px 0; }\n"
        "a { color: ") + linkC + QStringLiteral("; text-decoration: none; }\n"
        "a:hover { text-decoration: underline; }\n"
        "code { background: ") + codeBg + QStringLiteral("; color: ") + codeFg + QStringLiteral(";\n"
        "  padding: 2px 6px; border-radius: 3px;\n"
        "  font-family: 'JetBrains Mono', 'Fira Code', monospace; font-size: 0.9em; }\n"
        "pre { background: ") + codeBg + QStringLiteral("; padding: 12px 16px;\n"
        "  border-radius: 6px; overflow-x: auto; border: 1px solid ") + borderC + QStringLiteral("; }\n"
        "pre code { background: none; padding: 0; font-size: 0.85em; color: ") + fg + QStringLiteral("; }\n"
        ".mermaid { text-align: center; }\n"
        "blockquote { border-left: 4px solid ") + bqBorder + QStringLiteral("; color: ") + bqFg + QStringLiteral(";\n"
        "  margin: 8px 0; padding: 4px 16px; }\n"
        "ul, ol { padding-left: 24px; margin: 8px 0; }\n"
        "li { margin: 4px 0; }\n"
        "hr { border: none; border-top: 1px solid ") + hrC + QStringLiteral("; margin: 24px 0; }\n"
        "table { border-collapse: collapse; width: 100%; margin: 8px 0; }\n"
        "th, td { border: 1px solid ") + borderC + QStringLiteral("; padding: 6px 12px; text-align: left; }\n"
        "th { background: ") + thBg + QStringLiteral("; font-weight: 600; }\n"
        "img { max-width: 100%; border-radius: 4px; }\n"
        "del { text-decoration: line-through; opacity: 0.6; }\n"
        "</style>\n</head><body>\n")
        + body +
        QStringLiteral("\n<script>\n"
        "if (typeof katex !== 'undefined') {\n"
        "    document.querySelectorAll('.katex-display').forEach(function(el) {\n"
        "        try { katex.render(el.textContent, el, { displayMode: true, throwOnError: false }); }\n"
        "        catch(e) { el.textContent = el.textContent; }\n"
        "    });\n"
        "    document.querySelectorAll('.katex-inline').forEach(function(el) {\n"
        "        try { katex.render(el.textContent, el, { displayMode: false, throwOnError: false }); }\n"
        "        catch(e) { el.textContent = el.textContent; }\n"
        "    });\n"
        "}\n"
        "if (typeof hljs !== 'undefined') { hljs.highlightAll(); }\n"
        "if (typeof mermaid !== 'undefined') {\n"
        "    mermaid.initialize({ startOnLoad: false, theme: '") + mermaidTheme + QStringLiteral("',\n"
        "        suppressErrors: true, securityLevel: 'loose',\n"
        "        flowchart: { useMaxWidth: true, htmlLabels: true } });\n"
        "    mermaid.run().catch(function(e) {\n"
        "        document.querySelectorAll('.mermaid').forEach(function(el) {\n"
        "            if (el.getAttribute('data-processed')) return;\n"
        "            el.style.color = '#b91c1c'; el.style.fontStyle = 'italic'; el.style.whiteSpace = 'pre-wrap';\n"
        "        });\n"
        "    });\n"
        "}\n"
        "</script>\n</body></html>");
}
