#pragma once
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>

class MarkdownHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit MarkdownHighlighter(QTextDocument *parent = nullptr);

    void setDarkTheme(bool dark);

protected:
    void highlightBlock(const QString &text) override;

private:
    void buildRules();

    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<Rule> m_rules;
    bool m_dark = false;

    QTextCharFormat m_codeBlockFormat;
    QTextCharFormat m_mermaidBlockFormat;
    QRegularExpression m_codeBlockStart;
    QRegularExpression m_mermaidBlockStart;
    QRegularExpression m_codeBlockEnd;
};
