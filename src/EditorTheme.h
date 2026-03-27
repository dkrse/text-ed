#pragma once
#include <QColor>
#include <QString>
#include <QVector>

struct EditorTheme {
    QString name;
    bool isDark = false;

    // Editor colors
    QColor background;
    QColor foreground;
    QColor currentLine;
    QColor lineNumberBg;
    QColor lineNumberFg;
    QColor selection;

    // UI chrome colors
    QColor titleBarBg;
    QColor tabBarBg;
    QColor tabInactiveBg;
    QColor tabActiveBg;
    QColor statusBarBg;
    QColor scrollTrackBg;
    QColor scrollThumbBg;
    QColor scrollThumbHover;
    QColor panelBg;
    QColor border;
    QColor elementHover;
    QColor textMuted;

    // Syntax highlighting colors
    QColor keyword;
    QColor type;
    QColor string;
    QColor comment;
    QColor number;
    QColor function;
    QColor preprocessor;
    QColor operatorColor;

    static void loadFromDirectory(const QString &dirPath);
    static const QVector<EditorTheme> &themes();
    static const EditorTheme &themeByName(const QString &name);
    static QStringList themeNames();
};
