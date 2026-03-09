#include "EditorTheme.h"

const QVector<EditorTheme> &EditorTheme::builtinThemes()
{
    static const QVector<EditorTheme> themes = {
        // Default Light (GitHub-inspired)
        {"Default Light",
         "#ffffff", "#24292f", "#e8f0fe", "#f0f0f0", "#999999", "#0969da",
         "#d73a49", "#6f42c1", "#032f62", "#6a737d", "#005cc5", "#6f42c1", "#22863a", "#d73a49"},

        // Solarized Light
        {"Solarized Light",
         "#fdf6e3", "#657b83", "#eee8d5", "#eee8d5", "#93a1a1", "#268bd2",
         "#859900", "#b58900", "#2aa198", "#93a1a1", "#d33682", "#268bd2", "#cb4b16", "#859900"},

        // Monokai
        {"Monokai",
         "#272822", "#f8f8f2", "#3e3d32", "#2d2d2d", "#75715e", "#49483e",
         "#f92672", "#66d9ef", "#e6db74", "#75715e", "#ae81ff", "#a6e22e", "#fd971f", "#f92672"},

        // Dracula
        {"Dracula",
         "#282a36", "#f8f8f2", "#44475a", "#343746", "#6272a4", "#44475a",
         "#ff79c6", "#8be9fd", "#f1fa8c", "#6272a4", "#bd93f9", "#50fa7b", "#ffb86c", "#ff79c6"},

        // One Dark
        {"One Dark",
         "#282c34", "#abb2bf", "#2c313c", "#2e3440", "#636d83", "#3e4451",
         "#c678dd", "#e5c07b", "#98c379", "#5c6370", "#d19a66", "#61afef", "#e06c75", "#c678dd"},

        // Nord
        {"Nord",
         "#2e3440", "#d8dee9", "#3b4252", "#3b4252", "#4c566a", "#434c5e",
         "#81a1c1", "#8fbcbb", "#a3be8c", "#616e88", "#b48ead", "#88c0d0", "#d08770", "#81a1c1"},

        // Gruvbox Dark
        {"Gruvbox Dark",
         "#282828", "#ebdbb2", "#3c3836", "#3c3836", "#928374", "#504945",
         "#fb4934", "#fabd2f", "#b8bb26", "#928374", "#d3869b", "#8ec07c", "#fe8019", "#fb4934"},

        // Gruvbox Light
        {"Gruvbox Light",
         "#fbf1c7", "#3c3836", "#ebdbb2", "#ebdbb2", "#928374", "#d5c4a1",
         "#9d0006", "#b57614", "#79740e", "#928374", "#8f3f71", "#427b58", "#af3a03", "#9d0006"},

        // Tomorrow Night
        {"Tomorrow Night",
         "#1d1f21", "#c5c8c6", "#282a2e", "#282a2e", "#969896", "#373b41",
         "#b294bb", "#f0c674", "#b5bd68", "#969896", "#de935f", "#81a2be", "#cc6666", "#b294bb"},

        // GitHub Dark
        {"GitHub Dark",
         "#0d1117", "#c9d1d9", "#161b22", "#161b22", "#484f58", "#1f2937",
         "#ff7b72", "#d2a8ff", "#a5d6ff", "#8b949e", "#79c0ff", "#d2a8ff", "#ffa657", "#ff7b72"},

        // Catppuccin Mocha
        {"Catppuccin Mocha",
         "#1e1e2e", "#cdd6f4", "#313244", "#313244", "#585b70", "#45475a",
         "#cba6f7", "#f9e2af", "#a6e3a1", "#6c7086", "#fab387", "#89b4fa", "#f38ba8", "#cba6f7"},

        // Catppuccin Latte
        {"Catppuccin Latte",
         "#eff1f5", "#4c4f69", "#e6e9ef", "#e6e9ef", "#9ca0b0", "#ccd0da",
         "#8839ef", "#df8e1d", "#40a02b", "#9ca0b0", "#fe640b", "#1e66f5", "#d20f39", "#8839ef"},
    };
    return themes;
}

const EditorTheme &EditorTheme::themeByName(const QString &name)
{
    for (const auto &t : builtinThemes()) {
        if (t.name == name) return t;
    }
    return builtinThemes().first();
}

QStringList EditorTheme::themeNames()
{
    QStringList names;
    for (const auto &t : builtinThemes())
        names << t.name;
    return names;
}
