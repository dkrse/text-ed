#include "Minimap.h"
#include <QPainter>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QMouseEvent>
#include <QTextBlock>
#include <QTimer>
#include <QTextDocument>

Minimap::Minimap(QPlainTextEdit *editor, QWidget *parent)
    : QWidget(parent), m_editor(editor)
{
    setFixedWidth(100);
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(false);

    m_rebuildTimer = new QTimer(this);
    m_rebuildTimer->setSingleShot(true);
    m_rebuildTimer->setInterval(150);
    connect(m_rebuildTimer, &QTimer::timeout, this, &Minimap::rebuildPixmap);

    connect(editor->document(), &QTextDocument::contentsChanged, this, &Minimap::scheduleRebuild);
    connect(editor->verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() { update(); });
}

void Minimap::scheduleRebuild()
{
    m_dirty = true;
    m_rebuildTimer->start();
}

void Minimap::rebuildPixmap()
{
    const int w = width();
    const int totalBlocks = m_editor->document()->blockCount();
    if (totalBlocks == 0 || w == 0) {
        m_pixmap = QPixmap();
        m_dirty = false;
        update();
        return;
    }

    // Each line gets 2 pixels height in the pixmap
    const int lineH = 2;
    const int pixH = qMin(totalBlocks * lineH, 4000); // cap pixmap height
    const qreal scale = static_cast<qreal>(pixH) / (totalBlocks * lineH);

    m_pixmap = QPixmap(w, pixH);

    // Use editor background color
    QColor bgColor = m_editor->palette().color(QPalette::Base);
    m_pixmap.fill(bgColor);

    QPainter p(&m_pixmap);
    p.setRenderHint(QPainter::Antialiasing, false);

    // Get text color from editor
    QColor textColor = m_editor->palette().color(QPalette::Text);
    textColor.setAlpha(160);

    QTextBlock block = m_editor->document()->begin();
    int y = 0;
    while (block.isValid() && y < pixH) {
        QString text = block.text();
        if (!text.isEmpty()) {
            // Find leading whitespace
            int indent = 0;
            for (QChar ch : text) {
                if (ch == ' ') indent++;
                else if (ch == '\t') indent += 4;
                else break;
            }
            // Calculate text length (non-whitespace content)
            int contentLen = text.trimmed().length();
            if (contentLen > 0) {
                // Scale: 1 char ~ 1 pixel width, with indent offset
                int x0 = qMin(indent, w - 2);
                int x1 = qMin(x0 + contentLen, w);
                if (x1 > x0) {
                    p.fillRect(x0, y, x1 - x0, lineH, textColor);
                }
            }
        }
        y += lineH;
        block = block.next();
    }

    p.end();
    m_dirty = false;
    update();
}

void Minimap::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    // Fill background
    QColor bgColor = m_editor->palette().color(QPalette::Base);
    p.fillRect(rect(), bgColor);

    if (m_dirty || m_pixmap.isNull()) {
        rebuildPixmap();
    }

    int totalBlocks = m_editor->document()->blockCount();
    if (totalBlocks == 0 || m_pixmap.isNull()) return;

    // Draw the pixmap scaled to widget height
    p.drawPixmap(rect(), m_pixmap);

    // Draw visible area overlay
    QScrollBar *sb = m_editor->verticalScrollBar();
    int firstVisible = sb->value();
    int visibleLines = m_editor->viewport()->height() / qMax(1, m_editor->fontMetrics().lineSpacing());

    qreal lineRatio = static_cast<qreal>(height()) / qMax(1, totalBlocks);
    int rectTop = static_cast<int>(firstVisible * lineRatio);
    int rectHeight = qMax(static_cast<int>(visibleLines * lineRatio), 8);

    p.fillRect(0, rectTop, width(), rectHeight, QColor(100, 100, 255, 35));
    p.setPen(QColor(100, 100, 255, 80));
    p.drawRect(0, rectTop, width() - 1, rectHeight);
}

void Minimap::scrollToY(int y)
{
    int totalBlocks = m_editor->document()->blockCount();
    if (totalBlocks == 0) return;

    qreal ratio = static_cast<qreal>(y) / qMax(1, height());
    int targetBlock = static_cast<int>(ratio * totalBlocks);
    targetBlock = qBound(0, targetBlock, totalBlocks - 1);

    QTextCursor c(m_editor->document()->findBlockByNumber(targetBlock));
    m_editor->setTextCursor(c);
    m_editor->centerCursor();
}

void Minimap::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        scrollToY(event->pos().y());
}

void Minimap::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
        scrollToY(event->pos().y());
}

void Minimap::wheelEvent(QWheelEvent *event)
{
    m_editor->verticalScrollBar()->event(event);
}

void Minimap::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    scheduleRebuild();
}
