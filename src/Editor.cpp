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

// --- ScrollBarOverlay ---

ScrollBarOverlay::ScrollBarOverlay(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
}

void ScrollBarOverlay::setMarkerPositions(const QVector<qreal> &positions)
{
    m_positions = positions;
    update();
}

void ScrollBarOverlay::paintEvent(QPaintEvent *)
{
    if (m_positions.isEmpty()) return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#FF8C00")); // orange markers
    const int h = height();
    const int w = width();
    for (qreal pos : m_positions) {
        int y = qBound(0, static_cast<int>(pos * h), h - 2);
        p.drawRect(0, y, w, 2);
    }
}

// --- SpacedDocumentLayout ---

QRectF SpacedDocumentLayout::blockBoundingRect(const QTextBlock &block) const
{
    QRectF r = QPlainTextDocumentLayout::blockBoundingRect(block);
    if (m_factor > 1.01)
        r.setHeight(r.height() * m_factor);
    return r;
}

// --- Editor ---

Editor::Editor(QWidget *parent) : QPlainTextEdit(parent)
{
    setFrameShape(QFrame::NoFrame);

    m_spacedLayout = new SpacedDocumentLayout(document());
    document()->setDocumentLayout(m_spacedLayout);

    m_lineNumberArea = new LineNumberArea(this);
    m_scrollBarOverlay = new ScrollBarOverlay(this);

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
    setAutoIndent(s.autoIndent);
    setBracketMatching(s.bracketMatching);
    setShowRuler(s.showRuler);
    setRulerColumn(s.rulerColumn);
    setLineSpacing(s.lineSpacing);
}

void Editor::setLineSpacing(double factor)
{
    m_lineSpacing = factor;
    m_spacedLayout->setSpacingFactor(factor);
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
    viewport()->update();

    if (m_codeHighlighter) {
        m_codeHighlighter->applyThemeColors(theme.keyword, theme.type, theme.string,
            theme.comment, theme.number, theme.function, theme.preprocessor, theme.operatorColor);
    }
    if (m_mdHighlighter) {
        m_mdHighlighter->setDarkTheme(theme.isDark);
    }
}

void Editor::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);
    if (m_showRuler) {
        QPainter painter(viewport());
        int x = fontMetrics().horizontalAdvance(QLatin1Char(' ')) * m_rulerColumn
                + contentOffset().x();
        painter.setPen(QPen(m_rulerColor, 1));
        painter.drawLine(x, 0, x, viewport()->height());
    }
}

void Editor::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) { zoomInEditor(); return; }
        if (event->key() == Qt::Key_Minus) { zoomOutEditor(); return; }
    }
    if (m_autoIndent && (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && !(event->modifiers() & Qt::ShiftModifier)) {
        QTextCursor c = textCursor();
        QString lineText = c.block().text();
        QString indent;
        for (QChar ch : lineText) {
            if (ch == ' ' || ch == '\t') indent += ch;
            else break;
        }
        c.insertText("\n" + indent);
        setTextCursor(c);
        ensureCursorVisible();
        return;
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
    updateScrollBarOverlay();
}

void Editor::highlightCurrentLine()
{
    updateAllSelections();
}

void Editor::updateAllSelections()
{
    QList<QTextEdit::ExtraSelection> selections;

    // Current line highlight
    if (!isReadOnly() && m_highlightCurrentLine) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(m_currentLineColor);
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        sel.cursor = textCursor();
        sel.cursor.clearSelection();
        selections.append(sel);
    }

    // Bracket matching
    if (m_bracketMatching) {
        QTextCursor c = textCursor();
        int pos = c.position();
        QTextDocument *doc = document();
        auto matchBracket = [&](int p, QChar open, QChar close, int direction) -> int {
            int depth = 0;
            int i = p;
            while (i >= 0 && i < doc->characterCount()) {
                QChar ch = doc->characterAt(i);
                if (ch == open) depth++;
                if (ch == close) depth--;
                if (depth == 0) return i;
                i += direction;
            }
            return -1;
        };
        auto tryMatch = [&](int p) {
            if (p < 0 || p >= doc->characterCount()) return;
            QChar ch = doc->characterAt(p);
            int match = -1;
            if (ch == '(') match = matchBracket(p, '(', ')', 1);
            else if (ch == ')') match = matchBracket(p, ')', '(', -1);
            else if (ch == '{') match = matchBracket(p, '{', '}', 1);
            else if (ch == '}') match = matchBracket(p, '}', '{', -1);
            else if (ch == '[') match = matchBracket(p, '[', ']', 1);
            else if (ch == ']') match = matchBracket(p, ']', '[', -1);
            else return;
            if (match >= 0) {
                QTextCharFormat fmt;
                fmt.setBackground(QColor("#c8e6c9"));
                fmt.setForeground(QColor("#1b5e20"));
                fmt.setFontWeight(QFont::Bold);
                QTextEdit::ExtraSelection sel1;
                sel1.format = fmt;
                sel1.cursor = QTextCursor(doc);
                sel1.cursor.setPosition(p);
                sel1.cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
                selections.append(sel1);
                QTextEdit::ExtraSelection sel2;
                sel2.format = fmt;
                sel2.cursor = QTextCursor(doc);
                sel2.cursor.setPosition(match);
                sel2.cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
                selections.append(sel2);
            }
        };
        tryMatch(pos);
        if (pos > 0) tryMatch(pos - 1);
    }

    // Search highlights
    for (int i = 0; i < m_searchSelections.size(); ++i) {
        QTextEdit::ExtraSelection sel = m_searchSelections[i];
        if (i == m_currentMatch) {
            sel.format.setBackground(QColor("#FF8C00")); // orange for current
        }
        selections.append(sel);
    }

    setExtraSelections(selections);
}

void Editor::highlightSearchMatches(const QString &text, bool caseSensitive)
{
    m_searchSelections.clear();
    m_searchText = text;
    m_searchCaseSensitive = caseSensitive;
    m_currentMatch = -1;

    if (text.isEmpty()) {
        updateAllSelections();
        updateScrollBarOverlay();
        emit searchStateChanged();
        return;
    }

    QTextDocument::FindFlags flags;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;

    QTextCursor cursor(document());
    QTextCharFormat fmt;
    fmt.setBackground(QColor("#FFFF00")); // yellow

    constexpr int maxResults = 10000;
    while (true) {
        cursor = document()->find(text, cursor, flags);
        if (cursor.isNull()) break;
        QTextEdit::ExtraSelection sel;
        sel.cursor = cursor;
        sel.format = fmt;
        m_searchSelections.append(sel);
        if (m_searchSelections.size() >= maxResults) break;
    }

    // Set current match to the one at or after the cursor
    if (!m_searchSelections.isEmpty()) {
        int cursorPos = textCursor().position();
        m_currentMatch = 0;
        for (int i = 0; i < m_searchSelections.size(); ++i) {
            if (m_searchSelections[i].cursor.selectionStart() >= cursorPos) {
                m_currentMatch = i;
                break;
            }
        }
    }

    updateAllSelections();
    updateScrollBarOverlay();
    emit searchStateChanged();
}

void Editor::clearSearchHighlights()
{
    m_searchSelections.clear();
    m_searchText.clear();
    m_currentMatch = -1;
    updateAllSelections();
    updateScrollBarOverlay();
    emit searchStateChanged();
}

void Editor::goToNextMatch()
{
    if (m_searchSelections.isEmpty()) return;
    m_currentMatch = (m_currentMatch + 1) % m_searchSelections.size();
    QTextCursor c = m_searchSelections[m_currentMatch].cursor;
    setTextCursor(c);
    ensureCursorVisible();
    updateAllSelections();
    emit searchStateChanged();
}

void Editor::goToPrevMatch()
{
    if (m_searchSelections.isEmpty()) return;
    m_currentMatch--;
    if (m_currentMatch < 0) m_currentMatch = m_searchSelections.size() - 1;
    QTextCursor c = m_searchSelections[m_currentMatch].cursor;
    setTextCursor(c);
    ensureCursorVisible();
    updateAllSelections();
    emit searchStateChanged();
}

void Editor::replaceCurrentMatch(const QString &replaceWith)
{
    if (m_currentMatch < 0 || m_currentMatch >= m_searchSelections.size()) return;

    QTextCursor c = m_searchSelections[m_currentMatch].cursor;
    c.beginEditBlock();
    c.removeSelectedText();
    c.insertText(replaceWith);
    c.endEditBlock();

    // Re-search to update matches
    highlightSearchMatches(m_searchText, m_searchCaseSensitive);

    // Adjust current match index
    if (!m_searchSelections.isEmpty()) {
        if (m_currentMatch >= m_searchSelections.size())
            m_currentMatch = 0;
        QTextCursor nc = m_searchSelections[m_currentMatch].cursor;
        setTextCursor(nc);
        ensureCursorVisible();
    }
    updateAllSelections();
    emit searchStateChanged();
}

int Editor::replaceAllMatches(const QString &findText, const QString &replaceWith, bool caseSensitive)
{
    if (findText.isEmpty()) return 0;

    QTextDocument::FindFlags flags;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;

    QTextCursor cursor(document());
    cursor.beginEditBlock();
    int count = 0;

    while (true) {
        cursor = document()->find(findText, cursor, flags);
        if (cursor.isNull()) break;
        cursor.removeSelectedText();
        cursor.insertText(replaceWith);
        ++count;
    }

    cursor.endEditBlock();

    // Re-search (will find nothing if all replaced)
    highlightSearchMatches(m_searchText, m_searchCaseSensitive);
    return count;
}

QVector<qreal> Editor::getMatchPositions() const
{
    QVector<qreal> positions;
    int total = document()->blockCount();
    if (total == 0) return positions;

    for (const auto &sel : m_searchSelections) {
        int block = sel.cursor.block().blockNumber();
        positions.append(static_cast<qreal>(block) / total);
    }
    return positions;
}

void Editor::updateScrollBarOverlay()
{
    QScrollBar *sb = verticalScrollBar();
    if (!sb || !sb->isVisible()) {
        m_scrollBarOverlay->hide();
        return;
    }
    m_scrollBarOverlay->show();
    m_scrollBarOverlay->setGeometry(
        width() - sb->width(), 0, sb->width(), height());
    m_scrollBarOverlay->setMarkerPositions(getMatchPositions());
    m_scrollBarOverlay->raise();
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
