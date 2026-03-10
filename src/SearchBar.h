#pragma once
#include <QWidget>

class QLineEdit;
class QPushButton;
class QLabel;
class QCheckBox;
class QHBoxLayout;

class SearchBar : public QWidget
{
    Q_OBJECT
public:
    explicit SearchBar(QWidget *parent = nullptr);

    void setReplaceVisible(bool visible);
    bool isReplaceVisible() const;

    QString searchText() const;
    QString replaceText() const;
    bool isCaseSensitive() const;

    void focusSearchField();
    void updateMatchLabel(int current, int total);

signals:
    void searchTextChanged(const QString &text, bool caseSensitive);
    void findNext();
    void findPrev();
    void replaceOne(const QString &replaceWith);
    void replaceAll(const QString &findText, const QString &replaceWith, bool caseSensitive);
    void closed();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onSearchTextEdited();

private:
    QLineEdit   *m_searchEdit;
    QLineEdit   *m_replaceEdit;
    QPushButton *m_prevBtn;
    QPushButton *m_nextBtn;
    QPushButton *m_replaceBtn;
    QPushButton *m_replaceAllBtn;
    QPushButton *m_closeBtn;
    QLabel      *m_matchLabel;
    QCheckBox   *m_caseCheck;
    QWidget     *m_replaceRow;
};
