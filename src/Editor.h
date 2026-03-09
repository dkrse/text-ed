#pragma once
#include <QPlainTextEdit>
#include "CodeHighlighter.h"

class QLabel;
class MarkdownHighlighter;
struct AppSettings;
struct EditorTheme;

class Editor : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit Editor(QWidget *parent = nullptr);

    void setEditorFont(const QFont &font);
    QFont editorFont() const;

    void zoomInEditor();
    void zoomOutEditor();

    void setMarkdownMode(bool on);
    bool isMarkdownMode() const { return m_markdownMode; }

    void setLanguage(CodeHighlighter::Language lang);
    CodeHighlighter::Language language() const;
    void clearHighlighter();

    void setShowLineNumbers(bool on);
    bool showLineNumbers() const { return m_showLineNumbers; }

    void setHighlightCurrentLineEnabled(bool on);
    void setShowWhitespace(bool on);
    void setTabWidth(int width);

    void applySettings(const AppSettings &s);
    void applyTheme(const EditorTheme &theme);

    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

signals:
    void fontSizeChanged(int pointSize);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();

private:
    QWidget *m_lineNumberArea;
    MarkdownHighlighter *m_mdHighlighter = nullptr;
    CodeHighlighter *m_codeHighlighter = nullptr;
    bool m_markdownMode = false;
    bool m_showLineNumbers = true;
    bool m_highlightCurrentLine = true;
    QColor m_currentLineColor = QColor("#e8f0fe");
    QColor m_lineNumberBg = QColor("#f0f0f0");
    QColor m_lineNumberFg = QColor("#999999");
};

class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(Editor *editor) : QWidget(editor), m_editor(editor) {}
    QSize sizeHint() const override { return QSize(m_editor->lineNumberAreaWidth(), 0); }
protected:
    void paintEvent(QPaintEvent *event) override { m_editor->lineNumberAreaPaintEvent(event); }
private:
    Editor *m_editor;
};
