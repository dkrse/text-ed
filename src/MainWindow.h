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
class QSplitter;
class QTabWidget;
class QLabel;
class QAction;
class QComboBox;
class QTimer;
class QToolButton;

struct TabData {
    QString filePath;
    bool modified = false;
    bool isMarkdown = false;
    bool previewVisible = false;
    QStringConverter::Encoding encoding = QStringConverter::Utf8;
    CodeHighlighter::Language language = CodeHighlighter::None;
    bool isRemote = false;
    QString remotePath;
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
    void updatePreviewContent();
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

    QTabWidget *m_tabWidget;
    MarkdownPreview *m_preview;
    QSplitter *m_splitter;

    QVector<TabData> m_tabs;
    AppSettings m_settings;

    QLabel *m_statusLabel;
    QComboBox *m_encodingCombo;
    QComboBox *m_languageCombo;
    QLabel *m_fontSizeLabel;
    QAction *m_previewAction;

    QTimer *m_previewTimer;
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
    bool m_ignoreTextChanged = false;
};
