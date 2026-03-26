#pragma once
#include <QWidget>

class QHBoxLayout;
class QToolButton;
class QTabBar;
class QLabel;

class TitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr);

    QTabBar *tabBar() const { return m_tabBar; }
    QToolButton *hamburgerButton() const { return m_hamburgerButton; }
    QLabel *titleLabel() const { return m_titleLabel; }

    void applyTheme(const QColor &bg, const QColor &fg, const QColor &activeBg,
                    const QColor &activeFg, const QColor &hoverBg);

signals:
    void minimizeRequested();
    void maximizeRequested();
    void closeRequested();

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QTabBar *m_tabBar;
    QToolButton *m_hamburgerButton;
    QToolButton *m_minButton;
    QToolButton *m_maxButton;
    QToolButton *m_closeButton;
    QLabel *m_titleLabel;
    QPoint m_dragPos;
    bool m_dragging = false;
};
