#include "EditorTheme.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

static QVector<EditorTheme> s_themes;

static QColor parseColor(const QJsonValue &val, const QColor &fallback = QColor())
{
    if (val.isNull() || !val.isString()) return fallback;
    QString s = val.toString();
    if (s.isEmpty()) return fallback;
    QColor c(s);
    return c.isValid() ? c : fallback;
}

static EditorTheme parseZedTheme(const QJsonObject &themeObj)
{
    EditorTheme t;
    t.name = themeObj["name"].toString();
    t.isDark = (themeObj["appearance"].toString() == "dark");

    QJsonObject s = themeObj["style"].toObject();
    QJsonObject syn = s["syntax"].toObject();

    // Editor colors
    t.background    = parseColor(s["editor.background"], parseColor(s["background"], t.isDark ? "#1e1e2e" : "#ffffff"));
    t.foreground    = parseColor(s["editor.foreground"], parseColor(s["text"], t.isDark ? "#cdd6f4" : "#24292f"));
    t.currentLine   = parseColor(s["editor.active_line.background"], t.isDark ? QColor(255,255,255,13) : QColor(0,0,0,13));
    t.lineNumberBg  = parseColor(s["editor.gutter.background"], t.background);
    t.lineNumberFg  = parseColor(s["editor.line_number"], parseColor(s["text.muted"], t.isDark ? "#585b70" : "#999999"));
    t.selection     = parseColor(s["element.selected"], t.isDark ? "#45475a" : "#ccd0da");

    // Player selection (first player) can override selection
    QJsonArray players = s["players"].toArray();
    if (!players.isEmpty()) {
        QColor psel = parseColor(players[0].toObject()["selection"]);
        if (psel.isValid()) t.selection = psel;
    }

    // UI chrome
    t.titleBarBg      = parseColor(s["title_bar.background"], parseColor(s["background"], t.background));
    t.tabBarBg        = parseColor(s["tab_bar.background"], t.titleBarBg);
    t.tabInactiveBg   = parseColor(s["tab.inactive_background"], t.tabBarBg);
    t.tabActiveBg     = parseColor(s["tab.active_background"], t.background);
    t.statusBarBg     = parseColor(s["status_bar.background"], t.titleBarBg);
    t.scrollTrackBg   = parseColor(s["scrollbar.track.background"], t.background);
    t.scrollThumbBg   = parseColor(s["scrollbar.thumb.background"], t.lineNumberFg);
    t.scrollThumbHover = parseColor(s["scrollbar.thumb.hover_background"], t.lineNumberFg);
    t.panelBg         = parseColor(s["panel.background"], t.titleBarBg);
    t.border          = parseColor(s["border"], t.isDark ? "#414559" : "#d0d0d0");
    t.elementHover    = parseColor(s["element.hover"], t.isDark ? QColor(255,255,255,30) : QColor(0,0,0,30));
    t.textMuted       = parseColor(s["text.muted"], t.lineNumberFg);

    // Syntax colors
    t.keyword       = parseColor(syn["keyword"].toObject()["color"], t.isDark ? "#cba6f7" : "#8839ef");
    t.type          = parseColor(syn["type"].toObject()["color"], t.isDark ? "#f9e2af" : "#df8e1d");
    t.string        = parseColor(syn["string"].toObject()["color"], t.isDark ? "#a6e3a1" : "#40a02b");
    t.comment       = parseColor(syn["comment"].toObject()["color"], t.isDark ? "#6c7086" : "#8c8fa1");
    t.number        = parseColor(syn["number"].toObject()["color"], parseColor(syn["constant"].toObject()["color"], t.isDark ? "#fab387" : "#fe640b"));
    t.function      = parseColor(syn["function"].toObject()["color"], t.isDark ? "#89b4fa" : "#1e66f5");
    t.preprocessor  = parseColor(syn["predoc"].toObject()["color"], parseColor(syn["attribute"].toObject()["color"], t.isDark ? "#f38ba8" : "#d20f39"));
    t.operatorColor = parseColor(syn["operator"].toObject()["color"], t.isDark ? "#89dceb" : "#04a5e5");

    return t;
}

void EditorTheme::loadFromDirectory(const QString &dirPath)
{
    s_themes.clear();

    QDir dir(dirPath);
    if (!dir.exists()) return;

    QStringList filters;
    filters << "*.json";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    for (const auto &fi : files) {
        QFile f(fi.absoluteFilePath());
        if (!f.open(QIODevice::ReadOnly)) continue;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
        f.close();
        if (err.error != QJsonParseError::NoError) continue;

        QJsonObject root = doc.object();
        QJsonArray themesArray = root["themes"].toArray();

        for (const auto &tv : themesArray) {
            EditorTheme t = parseZedTheme(tv.toObject());
            if (!t.name.isEmpty())
                s_themes.append(t);
        }
    }
}

const QVector<EditorTheme> &EditorTheme::themes()
{
    return s_themes;
}

const EditorTheme &EditorTheme::themeByName(const QString &name)
{
    for (const auto &t : s_themes) {
        if (t.name == name) return t;
    }
    if (!s_themes.isEmpty()) return s_themes.first();
    static EditorTheme fallback{"Fallback", false,
        "#ffffff", "#000000", "#f0f0f0", "#f0f0f0", "#999999", "#ccd0da",
        "#f0f0f0", "#f0f0f0", "#f0f0f0", "#ffffff", "#f0f0f0", "#f0f0f0",
        "#999999", "#666666", "#e0e0e0", "#d0d0d0", "#e0e0e0", "#666666",
        "#d73a49", "#6f42c1", "#032f62", "#6a737d", "#005cc5", "#6f42c1", "#22863a", "#d73a49"};
    return fallback;
}

QStringList EditorTheme::themeNames()
{
    QStringList names;
    for (const auto &t : s_themes)
        names << t.name;
    return names;
}
