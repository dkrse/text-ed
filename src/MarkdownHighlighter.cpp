#include "MarkdownHighlighter.h"

MarkdownHighlighter::MarkdownHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // Headings
    QTextCharFormat headingFmt;
    headingFmt.setFontWeight(QFont::Bold);
    headingFmt.setForeground(QColor("#1a73e8"));
    m_rules.append({QRegularExpression(QStringLiteral("^#{1,6}\\s.*")), headingFmt});

    // Bold **text** or __text__
    QTextCharFormat boldFmt;
    boldFmt.setFontWeight(QFont::Bold);
    m_rules.append({QRegularExpression(QStringLiteral("\\*\\*(.+?)\\*\\*")), boldFmt});
    m_rules.append({QRegularExpression(QStringLiteral("__(.+?)__")), boldFmt});

    // Italic *text* or _text_
    QTextCharFormat italicFmt;
    italicFmt.setFontItalic(true);
    m_rules.append({QRegularExpression(QStringLiteral("(?<!\\*)\\*(?!\\*)(.+?)(?<!\\*)\\*(?!\\*)")), italicFmt});
    m_rules.append({QRegularExpression(QStringLiteral("(?<!_)_(?!_)(.+?)(?<!_)_(?!_)")), italicFmt});

    // Inline code `text`
    QTextCharFormat codeFmt;
    codeFmt.setFontFamilies({QStringLiteral("Monospace")});
    codeFmt.setBackground(QColor("#f0f0f0"));
    codeFmt.setForeground(QColor("#d63384"));
    m_rules.append({QRegularExpression(QStringLiteral("`[^`]+`")), codeFmt});

    // Links [text](url)
    QTextCharFormat linkFmt;
    linkFmt.setForeground(QColor("#0969da"));
    linkFmt.setFontUnderline(true);
    m_rules.append({QRegularExpression(QStringLiteral("\\[([^\\]]+)\\]\\([^)]+\\)")), linkFmt});

    // Block quotes > text
    QTextCharFormat quoteFmt;
    quoteFmt.setForeground(QColor("#636c76"));
    quoteFmt.setFontItalic(true);
    m_rules.append({QRegularExpression(QStringLiteral("^>\\s.*")), quoteFmt});

    // Lists - item, * item, + item, 1. item
    QTextCharFormat listFmt;
    listFmt.setForeground(QColor("#8250df"));
    m_rules.append({QRegularExpression(QStringLiteral("^\\s*[-*+]\\s")), listFmt});
    m_rules.append({QRegularExpression(QStringLiteral("^\\s*\\d+\\.\\s")), listFmt});

    // Horizontal rule
    QTextCharFormat hrFmt;
    hrFmt.setForeground(QColor("#888888"));
    m_rules.append({QRegularExpression(QStringLiteral("^(---|\\*\\*\\*|___)\\s*$")), hrFmt});

    // LaTeX display math $$...$$
    QTextCharFormat mathFmt;
    mathFmt.setForeground(QColor("#b5651d"));
    mathFmt.setFontFamilies({QStringLiteral("Monospace")});
    m_rules.append({QRegularExpression(QStringLiteral("\\$\\$.+?\\$\\$")), mathFmt});

    // LaTeX inline math $...$
    m_rules.append({QRegularExpression(QStringLiteral("(?<!\\$)\\$(?!\\$).+?(?<!\\$)\\$(?!\\$)")), mathFmt});

    // Fenced code blocks
    m_codeBlockFormat.setFontFamilies({QStringLiteral("Monospace")});
    m_codeBlockFormat.setBackground(QColor("#f6f8fa"));
    m_codeBlockFormat.setForeground(QColor("#24292f"));

    // Mermaid blocks
    m_mermaidBlockFormat.setFontFamilies({QStringLiteral("Monospace")});
    m_mermaidBlockFormat.setBackground(QColor("#f0f7ff"));
    m_mermaidBlockFormat.setForeground(QColor("#0969da"));

    m_codeBlockStart = QRegularExpression(QStringLiteral("^```"));
    m_mermaidBlockStart = QRegularExpression(QStringLiteral("^```mermaid"), QRegularExpression::CaseInsensitiveOption);
    m_codeBlockEnd = QRegularExpression(QStringLiteral("^```\\s*$"));
}

void MarkdownHighlighter::highlightBlock(const QString &text)
{
    // Handle fenced code blocks (multi-line state)
    // State: 0 = normal, 1 = inside code block, 2 = inside mermaid block
    int prevState = previousBlockState();
    if (prevState == -1) prevState = 0;

    if (prevState == 1 || prevState == 2) {
        const auto &fmt = (prevState == 2) ? m_mermaidBlockFormat : m_codeBlockFormat;
        setFormat(0, text.length(), fmt);
        if (m_codeBlockEnd.match(text).hasMatch()) {
            setCurrentBlockState(0);
        } else {
            setCurrentBlockState(prevState);
        }
        return;
    }

    if (m_mermaidBlockStart.match(text).hasMatch()) {
        setFormat(0, text.length(), m_mermaidBlockFormat);
        setCurrentBlockState(2);
        return;
    }

    if (m_codeBlockStart.match(text).hasMatch()) {
        setFormat(0, text.length(), m_codeBlockFormat);
        setCurrentBlockState(1);
        return;
    }

    setCurrentBlockState(0);

    // Apply inline rules
    for (const auto &rule : m_rules) {
        auto it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            auto match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}
