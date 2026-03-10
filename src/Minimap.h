#pragma once
#include <QWidget>
#include <QPixmap>

class QPlainTextEdit;
class QTimer;

class Minimap : public QWidget
{
    Q_OBJECT
public:
    explicit Minimap(QPlainTextEdit *editor, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void scheduleRebuild();
    void rebuildPixmap();

private:
    void scrollToY(int y);

    QPlainTextEdit *m_editor;
    QPixmap m_pixmap;
    QTimer *m_rebuildTimer;
    bool m_dirty = true;
};
