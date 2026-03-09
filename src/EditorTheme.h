#pragma once
#include <QColor>
#include <QString>
#include <QVector>

struct EditorTheme {
    QString name;

    // Editor colors
    QColor background;
    QColor foreground;
    QColor currentLine;
    QColor lineNumberBg;
    QColor lineNumberFg;
    QColor selection;

    // Syntax highlighting colors
    QColor keyword;
    QColor type;
    QColor string;
    QColor comment;
    QColor number;
    QColor function;
    QColor preprocessor;
    QColor operatorColor;

    static const QVector<EditorTheme> &builtinThemes();
    static const EditorTheme &themeByName(const QString &name);
    static QStringList themeNames();
};
