#pragma once
#include <QMainWindow>
#include <QStringConverter>
#include "CodeHighlighter.h"
#include "SettingsDialog.h"

class Editor;
class SshSession;
class MarkdownPreview;
class QSplitter;
class QTabWidget;
class QLabel;
class QAction;
class QComboBox;

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
    void onTextChanged();
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
    static bool isMarkdownFile(const QString &path);
    void populateEncodingCombo();
    void populateLanguageCombo();

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

    SshSession *m_sshSession = nullptr;
    QAction *m_sshDisconnectAction = nullptr;
    QAction *m_sshOpenAction = nullptr;
    QLabel *m_sshLabel = nullptr;
};
