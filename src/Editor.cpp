#include "Editor.h"
#include "MarkdownHighlighter.h"
#include "CodeHighlighter.h"
#include "SettingsDialog.h"
#include "EditorTheme.h"
#include <QPainter>
#include <QTextBlock>
#include <QKeyEvent>
#include <QScrollBar>
#include <QTextOption>

Editor::Editor(QWidget *parent) : QPlainTextEdit(parent)
{
    m_lineNumberArea = new LineNumberArea(this);

    connect(this, &QPlainTextEdit::blockCountChanged, this, &Editor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &Editor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &Editor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);
    setLineWrapMode(QPlainTextEdit::NoWrap);
}

void Editor::setEditorFont(const QFont &font)
{
    setFont(font);
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);
    updateLineNumberAreaWidth(0);
}

QFont Editor::editorFont() const { return font(); }

void Editor::zoomInEditor()
{
    QFont f = font();
    f.setPointSize(qMin(f.pointSize() + 1, 72));
    setEditorFont(f);
    emit fontSizeChanged(f.pointSize());
}

void Editor::zoomOutEditor()
{
    QFont f = font();
    f.setPointSize(qMax(f.pointSize() - 1, 6));
    setEditorFont(f);
    emit fontSizeChanged(f.pointSize());
}

void Editor::clearHighlighter()
{
    if (m_mdHighlighter) { delete m_mdHighlighter; m_mdHighlighter = nullptr; }
    if (m_codeHighlighter) { delete m_codeHighlighter; m_codeHighlighter = nullptr; }
    m_markdownMode = false;
}

void Editor::setMarkdownMode(bool on)
{
    m_markdownMode = on;
    if (on) {
        if (m_codeHighlighter) { delete m_codeHighlighter; m_codeHighlighter = nullptr; }
        if (!m_mdHighlighter) m_mdHighlighter = new MarkdownHighlighter(document());
    } else {
        if (m_mdHighlighter) { delete m_mdHighlighter; m_mdHighlighter = nullptr; }
    }
}

void Editor::setLanguage(CodeHighlighter::Language lang)
{
    if (lang == CodeHighlighter::None) {
        if (m_codeHighlighter) { delete m_codeHighlighter; m_codeHighlighter = nullptr; }
        return;
    }
    if (m_mdHighlighter) { delete m_mdHighlighter; m_mdHighlighter = nullptr; }
    m_markdownMode = false;
    if (!m_codeHighlighter) m_codeHighlighter = new CodeHighlighter(document());
    m_codeHighlighter->setLanguage(lang);
}

CodeHighlighter::Language Editor::language() const
{
    return m_codeHighlighter ? m_codeHighlighter->language() : CodeHighlighter::None;
}

void Editor::setShowLineNumbers(bool on)
{
    m_showLineNumbers = on;
    m_lineNumberArea->setVisible(on);
    updateLineNumberAreaWidth(0);
}

void Editor::setHighlightCurrentLineEnabled(bool on)
{
    m_highlightCurrentLine = on;
    highlightCurrentLine();
}

void Editor::setShowWhitespace(bool on)
{
    QTextOption opt = document()->defaultTextOption();
    if (on)
        opt.setFlags(opt.flags() | QTextOption::ShowTabsAndSpaces);
    else
        opt.setFlags(opt.flags() & ~QTextOption::ShowTabsAndSpaces);
    document()->setDefaultTextOption(opt);
}

void Editor::setTabWidth(int width)
{
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * width);
}

void Editor::applySettings(const AppSettings &s)
{
    setEditorFont(s.editorFont);
    setShowLineNumbers(s.lineNumbers);
    setHighlightCurrentLineEnabled(s.highlightCurrentLine);
    setShowWhitespace(s.showWhitespace);
    setTabWidth(s.tabWidth);
    setLineWrapMode(s.lineWrap ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
}

void Editor::applyTheme(const EditorTheme &theme)
{
    QPalette p = palette();
    p.setColor(QPalette::Base, theme.background);
    p.setColor(QPalette::Text, theme.foreground);
    p.setColor(QPalette::Highlight, theme.selection);
    setPalette(p);

    m_currentLineColor = theme.currentLine;
    m_lineNumberBg = theme.lineNumberBg;
    m_lineNumberFg = theme.lineNumberFg;

    highlightCurrentLine();
    m_lineNumberArea->update();

    if (m_codeHighlighter) {
        m_codeHighlighter->applyThemeColors(theme.keyword, theme.type, theme.string,
            theme.comment, theme.number, theme.function, theme.preprocessor, theme.operatorColor);
    }
}

void Editor::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) { zoomInEditor(); return; }
        if (event->key() == Qt::Key_Minus) { zoomOutEditor(); return; }
    }
    QPlainTextEdit::keyPressEvent(event);
}

int Editor::lineNumberAreaWidth() const
{
    if (!m_showLineNumbers) return 0;
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) { max /= 10; ++digits; }
    return 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * (digits + 1) + 3;
}

void Editor::updateLineNumberAreaWidth(int) { setViewportMargins(lineNumberAreaWidth(), 0, 0, 0); }

void Editor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy) m_lineNumberArea->scroll(0, dy);
    else m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    if (rect.contains(viewport()->rect())) updateLineNumberAreaWidth(0);
}

void Editor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height());
}

void Editor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> selections;
    if (!isReadOnly() && m_highlightCurrentLine) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(m_currentLineColor);
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        sel.cursor = textCursor();
        sel.cursor.clearSelection();
        selections.append(sel);
    }
    setExtraSelections(selections);
}

void Editor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    if (!m_showLineNumbers) return;

    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), m_lineNumberBg);
    painter.setPen(m_lineNumberFg);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.drawText(0, top, m_lineNumberArea->width() - 3,
                           fontMetrics().height(), Qt::AlignRight,
                           QString::number(blockNumber + 1));
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}
