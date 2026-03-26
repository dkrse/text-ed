#include "TitleBar.h"
#include <QHBoxLayout>
#include <QToolButton>
#include <QTabBar>
#include <QLabel>
#include <QMouseEvent>
#include <QWindow>

TitleBar::TitleBar(QWidget *parent) : QWidget(parent)
{
    setFixedHeight(32);
    setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Hamburger menu button
    m_hamburgerButton = new QToolButton(this);
    m_hamburgerButton->setText("\u2630");
    m_hamburgerButton->setPopupMode(QToolButton::InstantPopup);
    m_hamburgerButton->setFixedSize(40, 32);
    layout->addWidget(m_hamburgerButton);

    // Tab bar
    m_tabBar = new QTabBar(this);
    m_tabBar->setTabsClosable(true);
    m_tabBar->setMovable(false);
    m_tabBar->setDocumentMode(true);
    m_tabBar->setExpanding(false);
    m_tabBar->setDrawBase(false);
    layout->addWidget(m_tabBar);

    // Title label (shown when no tabs)
    m_titleLabel = new QLabel("TextEd", this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->hide();

    // Drag spacer — catches mouse events for window dragging
    m_dragSpacer = new QWidget(this);
    m_dragSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_dragSpacer->installEventFilter(this);
    layout->addWidget(m_dragSpacer);
    layout->addWidget(m_titleLabel);
    auto *spacer2 = new QWidget(this);
    spacer2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    spacer2->installEventFilter(this);
    layout->addWidget(spacer2);

    // Window control buttons
    m_minButton = new QToolButton(this);
    m_minButton->setText("\u2013");  // en-dash as minimize
    m_minButton->setFixedSize(46, 32);
    connect(m_minButton, &QToolButton::clicked, this, &TitleBar::minimizeRequested);
    layout->addWidget(m_minButton);

    m_maxButton = new QToolButton(this);
    m_maxButton->setText("\u25A1");  // square
    m_maxButton->setFixedSize(46, 32);
    connect(m_maxButton, &QToolButton::clicked, this, &TitleBar::maximizeRequested);
    layout->addWidget(m_maxButton);

    m_closeButton = new QToolButton(this);
    m_closeButton->setText("\u2715");  // multiplication x
    m_closeButton->setFixedSize(46, 32);
    connect(m_closeButton, &QToolButton::clicked, this, &TitleBar::closeRequested);
    layout->addWidget(m_closeButton);
}

void TitleBar::applyTheme(const QColor &bg, const QColor &fg, const QColor &activeBg,
                           const QColor &activeFg, const QColor &hoverBg)
{
    QString baseBg = bg.name();
    QString baseFg = fg.name();
    QString actBg = activeBg.name();
    QString actFg = activeFg.name();
    QString hovBg = hoverBg.name();

    setStyleSheet(QString("TitleBar { background: %1; }").arg(baseBg));

    QString btnStyle = QString(
        "QToolButton { border: none; background: %1; color: %2; font-size: 14px; }"
        "QToolButton:hover { background: %3; }"
        "QToolButton::menu-indicator { image: none; }"
    ).arg(baseBg, baseFg, hovBg);

    m_hamburgerButton->setStyleSheet(btnStyle + QString(
        "QToolButton { font-size: 16px; }"));
    m_minButton->setStyleSheet(btnStyle);
    m_maxButton->setStyleSheet(btnStyle);
    m_closeButton->setStyleSheet(btnStyle + QString(
        "QToolButton:hover { background: #e81123; color: white; }"));

    m_tabBar->setStyleSheet(QString(
        "QTabBar { background: %1; }"
        "QTabBar::tab { background: %1; color: %2; padding: 6px 12px; border: none; min-width: 60px; }"
        "QTabBar::tab:selected { background: %3; color: %4; }"
        "QTabBar::tab:hover { background: %5; }"
        "QTabBar::close-button { image: url(none); }"
    ).arg(baseBg, baseFg, actBg, actFg, hovBg));

    m_titleLabel->setStyleSheet(QString("QLabel { color: %1; }").arg(baseFg));
}

bool TitleBar::eventFilter(QObject *obj, QEvent *event)
{
    // Handle drag on spacer widgets
    if (event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            if (auto *win = window()->windowHandle())
                win->startSystemMove();
            return true;
        }
    }
    if (event->type() == QEvent::MouseButtonDblClick) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            emit maximizeRequested();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit maximizeRequested();
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (auto *win = window()->windowHandle())
            win->startSystemMove();
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent *) {}
void TitleBar::mouseReleaseEvent(QMouseEvent *) {}
