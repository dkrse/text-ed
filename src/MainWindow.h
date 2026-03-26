#pragma once
#include <QMainWindow>
#include <QStringConverter>
#include <QStringList>
#include <QMap>
#include "CodeHighlighter.h"
#include "SettingsDialog.h"

class Editor;
class Minimap;
class SshSession;
struct EditorTheme;
class MarkdownPreview;
class SearchBar;
class QTabWidget;
class QLabel;
class QAction;
class QComboBox;
class QTimer;
class QToolButton;
class TitleBar;

struct TabData {
    QString filePath;
    bool modified = false;
    bool isMarkdown = false;
    QStringConverter::Encoding encoding = QStringConverter::Utf8;
    CodeHighlighter::Language language = CodeHighlighter::None;
    bool isRemote = false;
    QString remotePath;
    bool isPreview = false;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    void openFile(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void newFile();
    void open();
    void save();
    void saveAs();
    void closeTab(int index);
    void onTabChanged(int index);
    void togglePreview();
    void openSettings();
    void sshConnect();
    void sshDisconnect();
    void sshOpenFile();
    void updateStatusBar();
    void onEncodingChanged(int comboIndex);
    void onLanguageChanged(int comboIndex);

private:
    void setupMenus();
    void setupStatusBar();
    int createTab(const QString &title = QString());
    void loadFileIntoTab(int index, const QString &path);
    void reloadWithEncoding(QStringConverter::Encoding enc);
    void saveFileFromTab(int index, const QString &path);
    void updateTabTitle(int index);
    void updateWindowTitle();
    Editor *currentEditor() const;
    TabData *currentTabData();
    TabData *tabData(int index);
    int findTabByPath(const QString &path) const;
    void connectEditor(Editor *editor);
    void disconnectEditor(Editor *editor);
    void applySettingsToAllEditors();
    void applyThemeToApp(const EditorTheme &theme);
    static bool isMarkdownFile(const QString &path);
    void populateEncodingCombo();
    void populateLanguageCombo();
    void setupSearchBar();
    void addToRecentFiles(const QString &path);
    void loadRecentFiles();
    void saveRecentFiles();
    void updateRecentFilesMenu();
    void saveSettings();
    void loadSettings();
    void saveSession();
    void restoreSession();
    void showSearchBar(bool withReplace);
    void onSearchTextChanged(const QString &text, bool caseSensitive);
    void onFindNext();
    void onFindPrev();
    void onReplaceOne(const QString &replaceWith);
    void onReplaceAll(const QString &findText, const QString &replaceWith, bool caseSensitive);
    void onSearchClosed();
    void updateSearchMatchLabel();
    void autoSaveAll();
    void updateAutoSaveTimer();
    void updateMinimaps();
    void addPreviewButton(int tabIndex);
    void openPreviewTab(int sourceIndex);
    void closePreviewTab(int previewIndex);
    int sourceIndexForPreview(int previewIndex) const;
    int previewIndexForSource(int sourceIndex) const;
    bool isPreviewTab(int index) const;
    MarkdownPreview *currentMarkdownPreview() const;

    QTabWidget *m_tabWidget;
    QList<MarkdownPreview*> m_mdPreviews;

    QVector<TabData> m_tabs;
    AppSettings m_settings;

    QLabel *m_statusLabel;
    QComboBox *m_encodingCombo;
    QComboBox *m_languageCombo;
    QLabel *m_fontSizeLabel;
    QAction *m_previewAction;

    QTimer *m_autoSaveTimer = nullptr;

    QMap<Editor*, Minimap*> m_minimaps;

    SshSession *m_sshSession = nullptr;
    QAction *m_sshDisconnectAction = nullptr;
    QAction *m_sshOpenAction = nullptr;
    QLabel *m_sshLabel = nullptr;

    SearchBar *m_searchBar = nullptr;

    QStringList m_recentFiles;
    QMenu *m_recentFilesMenu = nullptr;
    static constexpr int MaxRecentFiles = 5;

    QToolButton *m_hamburgerButton = nullptr;
    TitleBar *m_titleBar = nullptr;
    bool m_ignoreTextChanged = false;
};
