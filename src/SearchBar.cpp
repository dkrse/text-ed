#include "SearchBar.h"
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QPainter>
#include <QStyleOption>

SearchBar::SearchBar(QWidget *parent) : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(2);

    // Search row
    auto *searchRow = new QHBoxLayout;
    searchRow->setSpacing(4);

    m_closeBtn = new QPushButton(tr("X"));
    m_closeBtn->setFixedSize(24, 24);
    m_closeBtn->setToolTip(tr("Close (Escape)"));
    connect(m_closeBtn, &QPushButton::clicked, this, [this]() {
        hide();
        emit closed();
    });

    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(tr("Find..."));
    m_searchEdit->setMinimumWidth(200);
    m_searchEdit->installEventFilter(this);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &SearchBar::onSearchTextEdited);

    m_matchLabel = new QLabel(tr("No results"));
    m_matchLabel->setMinimumWidth(70);

    m_prevBtn = new QPushButton(tr("Prev"));
    m_prevBtn->setToolTip(tr("Previous match (Shift+F3)"));
    connect(m_prevBtn, &QPushButton::clicked, this, &SearchBar::findPrev);

    m_nextBtn = new QPushButton(tr("Next"));
    m_nextBtn->setToolTip(tr("Next match (F3 / Enter)"));
    connect(m_nextBtn, &QPushButton::clicked, this, &SearchBar::findNext);

    m_caseCheck = new QCheckBox(tr("Match case"));
    connect(m_caseCheck, &QCheckBox::toggled, this, [this]() {
        emit searchTextChanged(m_searchEdit->text(), m_caseCheck->isChecked());
    });

    searchRow->addWidget(m_closeBtn);
    searchRow->addWidget(m_searchEdit);
    searchRow->addWidget(m_matchLabel);
    searchRow->addWidget(m_prevBtn);
    searchRow->addWidget(m_nextBtn);
    searchRow->addWidget(m_caseCheck);
    searchRow->addStretch();

    mainLayout->addLayout(searchRow);

    // Replace row
    m_replaceRow = new QWidget;
    auto *replaceLayout = new QHBoxLayout(m_replaceRow);
    replaceLayout->setContentsMargins(28, 0, 0, 0); // indent to align with search field
    replaceLayout->setSpacing(4);

    m_replaceEdit = new QLineEdit;
    m_replaceEdit->setPlaceholderText(tr("Replace with..."));
    m_replaceEdit->setMinimumWidth(200);
    m_replaceEdit->installEventFilter(this);

    m_replaceBtn = new QPushButton(tr("Replace"));
    connect(m_replaceBtn, &QPushButton::clicked, this, [this]() {
        emit replaceOne(m_replaceEdit->text());
    });

    m_replaceAllBtn = new QPushButton(tr("Replace All"));
    connect(m_replaceAllBtn, &QPushButton::clicked, this, [this]() {
        emit replaceAll(m_searchEdit->text(), m_replaceEdit->text(), m_caseCheck->isChecked());
    });

    replaceLayout->addWidget(m_replaceEdit);
    replaceLayout->addWidget(m_replaceBtn);
    replaceLayout->addWidget(m_replaceAllBtn);
    replaceLayout->addStretch();

    m_replaceRow->hide();
    mainLayout->addWidget(m_replaceRow);

    hide();
}

void SearchBar::setReplaceVisible(bool visible)
{
    m_replaceRow->setVisible(visible);
}

bool SearchBar::isReplaceVisible() const
{
    return m_replaceRow->isVisible();
}

QString SearchBar::searchText() const { return m_searchEdit->text(); }
QString SearchBar::replaceText() const { return m_replaceEdit->text(); }
bool SearchBar::isCaseSensitive() const { return m_caseCheck->isChecked(); }

void SearchBar::focusSearchField()
{
    m_searchEdit->setFocus();
    m_searchEdit->selectAll();
}

void SearchBar::updateMatchLabel(int current, int total)
{
    if (m_searchEdit->text().isEmpty())
        m_matchLabel->setText(QString());
    else if (total == 0)
        m_matchLabel->setText(tr("No results"));
    else
        m_matchLabel->setText(tr("%1 of %2").arg(current).arg(total));
}

void SearchBar::onSearchTextEdited()
{
    emit searchTextChanged(m_searchEdit->text(), m_caseCheck->isChecked());
}

bool SearchBar::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Escape) {
            hide();
            emit closed();
            return true;
        }
        if (obj == m_searchEdit) {
            if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
                if (ke->modifiers() & Qt::ShiftModifier)
                    emit findPrev();
                else
                    emit findNext();
                return true;
            }
            if (ke->key() == Qt::Key_F3) {
                if (ke->modifiers() & Qt::ShiftModifier)
                    emit findPrev();
                else
                    emit findNext();
                return true;
            }
        }
        if (obj == m_replaceEdit) {
            if (ke->key() == Qt::Key_F3) {
                if (ke->modifiers() & Qt::ShiftModifier)
                    emit findPrev();
                else
                    emit findNext();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void SearchBar::paintEvent(QPaintEvent *event)
{
    // Allow stylesheets to work on this custom QWidget
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    QWidget::paintEvent(event);
}
