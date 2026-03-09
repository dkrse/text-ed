#pragma once
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>

class MarkdownHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit MarkdownHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<Rule> m_rules;

    QTextCharFormat m_codeBlockFormat;
    QTextCharFormat m_mermaidBlockFormat;
    QRegularExpression m_codeBlockStart;
    QRegularExpression m_mermaidBlockStart;
    QRegularExpression m_codeBlockEnd;
};
