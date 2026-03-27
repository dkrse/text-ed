#include "MarkdownHighlighter.h"

MarkdownHighlighter::MarkdownHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    m_codeBlockStart = QRegularExpression(QStringLiteral("^```"));
    m_mermaidBlockStart = QRegularExpression(QStringLiteral("^```mermaid"), QRegularExpression::CaseInsensitiveOption);
    m_codeBlockEnd = QRegularExpression(QStringLiteral("^```\\s*$"));
    buildRules();
}

void MarkdownHighlighter::setDarkTheme(bool dark)
{
    m_dark = dark;
    buildRules();
    rehighlight();
}

void MarkdownHighlighter::buildRules()
{
    m_rules.clear();

    QColor headingC   = m_dark ? QColor("#e5c07b") : QColor("#1a73e8");
    QColor codeC      = m_dark ? QColor("#98c379") : QColor("#d63384");
    QColor codeBgC    = m_dark ? QColor("#2d2d2d") : QColor("#f0f0f0");
    QColor linkC      = m_dark ? QColor("#61afef") : QColor("#0969da");
    QColor quoteC     = m_dark ? QColor("#5c6370") : QColor("#636c76");
    QColor listC      = m_dark ? QColor("#d19a66") : QColor("#8250df");
    QColor hrC        = m_dark ? QColor("#5c6370") : QColor("#888888");
    QColor mathC      = m_dark ? QColor("#56b6c2") : QColor("#b5651d");
    QColor boldC      = m_dark ? QColor("#e06c75") : QColor();
    QColor italicC    = m_dark ? QColor("#c678dd") : QColor();
    QColor codeBlkBg  = m_dark ? QColor("#2d2d2d") : QColor("#f6f8fa");
    QColor codeBlkFg  = m_dark ? QColor("#abb2bf") : QColor("#24292f");
    QColor mermaidBg  = m_dark ? QColor("#1e3a5f") : QColor("#f0f7ff");
    QColor mermaidFg  = m_dark ? QColor("#61afef") : QColor("#0969da");

    // Headings
    QTextCharFormat headingFmt;
    headingFmt.setFontWeight(QFont::Bold);
    headingFmt.setForeground(headingC);
    m_rules.append({QRegularExpression(QStringLiteral("^#{1,6}\\s.*")), headingFmt});

    // Bold
    QTextCharFormat boldFmt;
    boldFmt.setFontWeight(QFont::Bold);
    if (boldC.isValid()) boldFmt.setForeground(boldC);
    m_rules.append({QRegularExpression(QStringLiteral("\\*\\*(.+?)\\*\\*")), boldFmt});
    m_rules.append({QRegularExpression(QStringLiteral("__(.+?)__")), boldFmt});

    // Italic
    QTextCharFormat italicFmt;
    italicFmt.setFontItalic(true);
    if (italicC.isValid()) italicFmt.setForeground(italicC);
    m_rules.append({QRegularExpression(QStringLiteral("(?<!\\*)\\*(?!\\*)(.+?)(?<!\\*)\\*(?!\\*)")), italicFmt});
    m_rules.append({QRegularExpression(QStringLiteral("(?<!_)_(?!_)(.+?)(?<!_)_(?!_)")), italicFmt});

    // Inline code
    QTextCharFormat codeFmt;
    codeFmt.setFontFamilies({QStringLiteral("Monospace")});
    codeFmt.setBackground(codeBgC);
    codeFmt.setForeground(codeC);
    m_rules.append({QRegularExpression(QStringLiteral("`[^`]+`")), codeFmt});

    // Links
    QTextCharFormat linkFmt;
    linkFmt.setForeground(linkC);
    linkFmt.setFontUnderline(true);
    m_rules.append({QRegularExpression(QStringLiteral("\\[([^\\]]+)\\]\\([^)]+\\)")), linkFmt});

    // Block quotes
    QTextCharFormat quoteFmt;
    quoteFmt.setForeground(quoteC);
    quoteFmt.setFontItalic(true);
    m_rules.append({QRegularExpression(QStringLiteral("^>\\s.*")), quoteFmt});

    // Lists
    QTextCharFormat listFmt;
    listFmt.setForeground(listC);
    m_rules.append({QRegularExpression(QStringLiteral("^\\s*[-*+]\\s")), listFmt});
    m_rules.append({QRegularExpression(QStringLiteral("^\\s*\\d+\\.\\s")), listFmt});

    // Horizontal rule
    QTextCharFormat hrFmt;
    hrFmt.setForeground(hrC);
    m_rules.append({QRegularExpression(QStringLiteral("^(---|\\*\\*\\*|___)\\s*$")), hrFmt});

    // Math
    QTextCharFormat mathFmt;
    mathFmt.setForeground(mathC);
    mathFmt.setFontFamilies({QStringLiteral("Monospace")});
    m_rules.append({QRegularExpression(QStringLiteral("\\$\\$.+?\\$\\$")), mathFmt});
    m_rules.append({QRegularExpression(QStringLiteral("(?<!\\$)\\$(?!\\$).+?(?<!\\$)\\$(?!\\$)")), mathFmt});

    // Fenced code blocks
    m_codeBlockFormat.setFontFamilies({QStringLiteral("Monospace")});
    m_codeBlockFormat.setBackground(codeBlkBg);
    m_codeBlockFormat.setForeground(codeBlkFg);

    // Mermaid blocks
    m_mermaidBlockFormat.setFontFamilies({QStringLiteral("Monospace")});
    m_mermaidBlockFormat.setBackground(mermaidBg);
    m_mermaidBlockFormat.setForeground(mermaidFg);
}

void MarkdownHighlighter::highlightBlock(const QString &text)
{
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

    for (const auto &rule : m_rules) {
        auto it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            auto match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}
