#pragma once
#include <QDialog>
#include <QFont>

class QFontComboBox;
class QSpinBox;
class QCheckBox;
class QComboBox;
class QTabWidget;

struct AppSettings {
    QFont editorFont = QFont("Monospace", 11);
    QFont guiFont;
    bool syntaxHighlighting = true;
    bool lineNumbers = true;
    bool lineWrap = false;
    bool highlightCurrentLine = true;
    bool showWhitespace = false;
    int tabWidth = 4;
    QString themeName = "Default Light";
    bool autoIndent = true;
    bool bracketMatching = true;
    bool autoSave = false;
    int autoSaveInterval = 60;
    bool minimap = false;
    bool showRuler = false;
    int rulerColumn = 80;
};

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(const AppSettings &settings, QWidget *parent = nullptr);
    AppSettings settings() const;

private:
    QWidget *createEditorPage();
    QWidget *createAppearancePage();

    QFontComboBox *m_editorFontCombo;
    QSpinBox *m_editorFontSize;
    QSpinBox *m_tabWidth;
    QCheckBox *m_syntaxHighlighting;
    QCheckBox *m_lineNumbers;
    QCheckBox *m_lineWrap;
    QCheckBox *m_highlightCurrentLine;
    QCheckBox *m_showWhitespace;
    QCheckBox *m_autoIndent;
    QCheckBox *m_bracketMatching;
    QCheckBox *m_autoSave;
    QSpinBox *m_autoSaveInterval;
    QCheckBox *m_minimap;
    QCheckBox *m_showRuler;
    QSpinBox *m_rulerColumn;

    QFontComboBox *m_guiFontCombo;
    QSpinBox *m_guiFontSize;
    QComboBox *m_themeCombo;

    AppSettings m_original;
};
