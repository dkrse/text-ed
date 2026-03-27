#include "MainWindow.h"
#include "Editor.h"
#include "Minimap.h"
#include "MarkdownPreview.h"
#include "SearchBar.h"
#include "LargeFileReader.h"
#include "CodeHighlighter.h"
#include "SettingsDialog.h"
#include "EditorTheme.h"
#include "SshSession.h"
#include "SshConnectDialog.h"
#include "RemoteFileBrowser.h"
#include "TitleBar.h"

#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QTabWidget>
#include <QLabel>
#include <QComboBox>
#include <QApplication>
#include <QTimer>
#include <QFileInfo>
#include <QTextStream>
#include <QProgressDialog>
#include <QTextBlock>
#include <QStyle>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QShortcut>
#include <QSettings>
#include <QCloseEvent>
#include <QMenu>
#include <QInputDialog>
#include <QToolButton>
#include <QToolBar>
#include <QTabBar>
#include <QWebEngineView>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);

    m_settings.editorFont = QFont("Monospace", 11);
    m_settings.guiFont = QApplication::font();
    loadSettings();
    QApplication::setFont(m_settings.guiFont);

    // Load themes from user config directory
    QString themesDir = QDir::homePath() + "/.config/ed/themes";
    EditorTheme::loadFromDirectory(themesDir);

    // Set window background early to avoid white flash on dark themes
    auto earlyTheme = EditorTheme::themeByName(m_settings.themeName);
    QPalette earlyPal = palette();
    earlyPal.setColor(QPalette::Window, earlyTheme.lineNumberBg);
    earlyPal.setColor(QPalette::Base, earlyTheme.background);
    setPalette(earlyPal);

    auto *centralContainer = new QWidget(this);
    auto *centralLayout = new QVBoxLayout(centralContainer);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);

    // Custom title bar
    m_titleBar = new TitleBar(this);
    centralLayout->addWidget(m_titleBar);
    connect(m_titleBar, &TitleBar::minimizeRequested, this, &QWidget::showMinimized);
    connect(m_titleBar, &TitleBar::maximizeRequested, this, [this]() {
        isMaximized() ? showNormal() : showMaximized();
    });
    connect(m_titleBar, &TitleBar::closeRequested, this, &QWidget::close);

    // Tab widget uses the title bar's tab bar
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(false);  // tabs managed via title bar
    m_tabWidget->setMovable(false);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setStyleSheet("QTabWidget::pane { border: none; margin: 0; padding: 0; }");
    m_tabWidget->tabBar()->hide();  // hide built-in tab bar

    // Sync title bar tab bar with tab widget
    connect(m_titleBar->tabBar(), &QTabBar::currentChanged, m_tabWidget, &QTabWidget::setCurrentIndex);
    connect(m_titleBar->tabBar(), &QTabBar::tabCloseRequested, this, &MainWindow::closeTab);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    centralLayout->addWidget(m_tabWidget, 1);

    setupSearchBar();
    centralLayout->addWidget(m_searchBar);

    setCentralWidget(centralContainer);

    loadRecentFiles();
    setupMenus();
    setupStatusBar();

    m_autoSaveTimer = new QTimer(this);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::autoSaveAll);
    updateAutoSaveTimer();

    setAcceptDrops(true);

    // Pre-initialize QWebEngine to avoid flicker on first MarkdownPreview
    auto *warmup = new QWebEngineView(this);
    warmup->setFixedSize(0, 0);
    warmup->setVisible(false);
    warmup->load(QUrl("about:blank"));
    connect(warmup, &QWebEngineView::loadFinished, warmup, &QObject::deleteLater);

    restoreSession();
    applyThemeToApp(EditorTheme::themeByName(m_settings.themeName));
    updateWindowTitle();
}

void MainWindow::setupMenus()
{
    menuBar()->hide();

    m_hamburgerButton = m_titleBar->hamburgerButton();

    auto *menu = new QMenu(this);
    menu->setStyleSheet(
        "QMenu { padding: 4px 0; }"
        "QMenu::item { padding: 4px 20px 4px 20px; }"
        "QMenu::item:selected { background: palette(highlight); color: palette(highlighted-text); }"
    );
    m_hamburgerButton->setMenu(menu);

    // --- File ---
    auto *fileMenu = menu->addMenu(tr("&File"));
    fileMenu->addAction(tr("&New"), QKeySequence::New, this, &MainWindow::newFile);
    fileMenu->addAction(tr("&Open..."), QKeySequence::Open, this, &MainWindow::open);
    fileMenu->addAction(tr("&Save"), QKeySequence::Save, this, &MainWindow::save);
    fileMenu->addAction(tr("Save &As..."), QKeySequence::SaveAs, this, &MainWindow::saveAs);
    fileMenu->addSeparator();
    m_recentFilesMenu = fileMenu->addMenu(tr("Recent &Files"));
    updateRecentFilesMenu();
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Close Tab"), QKeySequence(Qt::CTRL | Qt::Key_W), this, [this]() {
        closeTab(m_tabWidget->currentIndex());
    });
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Quit"), QKeySequence::Quit, this, &QWidget::close);

    // --- Edit ---
    auto *editMenu = menu->addMenu(tr("&Edit"));
    editMenu->addAction(tr("&Undo"), QKeySequence::Undo, this, [this]() { if (auto *e = currentEditor()) e->undo(); });
    editMenu->addAction(tr("&Redo"), QKeySequence::Redo, this, [this]() { if (auto *e = currentEditor()) e->redo(); });
    editMenu->addSeparator();
    editMenu->addAction(tr("Cu&t"), QKeySequence::Cut, this, [this]() { if (auto *e = currentEditor()) e->cut(); });
    editMenu->addAction(tr("&Copy"), QKeySequence::Copy, this, [this]() { if (auto *e = currentEditor()) e->copy(); });
    editMenu->addAction(tr("&Paste"), QKeySequence::Paste, this, [this]() { if (auto *e = currentEditor()) e->paste(); });
    editMenu->addAction(tr("Select &All"), QKeySequence::SelectAll, this, [this]() { if (auto *e = currentEditor()) e->selectAll(); });
    editMenu->addSeparator();
    editMenu->addAction(tr("&Find..."), QKeySequence::Find, this, [this]() { showSearchBar(false); });
    editMenu->addAction(tr("Find and &Replace..."), QKeySequence(Qt::CTRL | Qt::Key_H), this, [this]() { showSearchBar(true); });
    editMenu->addAction(tr("&Go to Line..."), QKeySequence(Qt::CTRL | Qt::Key_G), this, [this]() {
        auto *e = currentEditor();
        if (!e) return;
        bool ok;
        int line = QInputDialog::getInt(this, tr("Go to Line"), tr("Line number:"),
            e->textCursor().blockNumber() + 1, 1, e->document()->blockCount(), 1, &ok);
        if (ok) {
            QTextCursor c(e->document()->findBlockByNumber(line - 1));
            e->setTextCursor(c);
            e->centerCursor();
            e->setFocus();
        }
    });

    // --- View ---
    auto *viewMenu = menu->addMenu(tr("&View"));
    viewMenu->addAction(tr("Zoom &In"), QKeySequence(Qt::CTRL | Qt::Key_Plus), this, [this]() {
        if (auto *mdp = currentMarkdownPreview()) mdp->zoomIn();
        else if (auto *e = currentEditor()) e->zoomInEditor();
    });
    viewMenu->addAction(tr("Zoom &Out"), QKeySequence(Qt::CTRL | Qt::Key_Minus), this, [this]() {
        if (auto *mdp = currentMarkdownPreview()) mdp->zoomOut();
        else if (auto *e = currentEditor()) e->zoomOutEditor();
    });
    viewMenu->addSeparator();
    m_previewAction = viewMenu->addAction(tr("Markdown &Preview"));
    m_previewAction->setCheckable(true);
    m_previewAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));
    connect(m_previewAction, &QAction::toggled, this, &MainWindow::togglePreview);

    viewMenu->addAction(tr("Export Preview to &PDF..."), this, [this]() {
        auto *preview = currentMarkdownPreview();
        if (!preview && !m_mdPreviews.isEmpty())
            preview = m_mdPreviews.first();
        if (!preview) {
            QMessageBox::information(this, tr("Export PDF"), tr("Preview must be open first."));
            return;
        }
        QString path = QFileDialog::getSaveFileName(this, tr("Export to PDF"), {},
                            tr("PDF Files (*.pdf)"));
        if (path.isEmpty()) return;
        if (!path.endsWith(".pdf", Qt::CaseInsensitive)) path += ".pdf";

        // PDF options dialog
        QDialog dlg(this);
        dlg.setWindowTitle(tr("PDF Export Options"));
        auto *fl = new QFormLayout(&dlg);

        auto *marginLeft = new QSpinBox(&dlg);
        marginLeft->setRange(0, 50); marginLeft->setValue(15); marginLeft->setSuffix(" mm");
        fl->addRow(tr("Left Margin:"), marginLeft);

        auto *marginRight = new QSpinBox(&dlg);
        marginRight->setRange(0, 50); marginRight->setValue(15); marginRight->setSuffix(" mm");
        fl->addRow(tr("Right Margin:"), marginRight);

        auto *pageNum = new QComboBox(&dlg);
        pageNum->addItem(tr("None"), "none");
        pageNum->addItem(tr("Page number"), "page");
        pageNum->addItem(tr("Page / Total"), "page/total");
        fl->addRow(tr("Page Numbers:"), pageNum);

        auto *orientation = new QComboBox(&dlg);
        orientation->addItem(tr("Portrait"), false);
        orientation->addItem(tr("Landscape"), true);
        fl->addRow(tr("Orientation:"), orientation);

        auto *pageBorder = new QCheckBox(tr("Draw page border"), &dlg);
        fl->addRow(pageBorder);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
        fl->addRow(buttons);
        connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

        if (dlg.exec() != QDialog::Accepted) return;

        preview->exportToPdf(path, marginLeft->value(), marginRight->value(),
                             pageNum->currentData().toString(),
                             orientation->currentData().toBool(),
                             pageBorder->isChecked());
    });

    // --- Remote ---
    auto *remoteMenu = menu->addMenu(tr("&Remote"));
    remoteMenu->addAction(tr("&Connect to SSH..."), this, &MainWindow::sshConnect);
    m_sshDisconnectAction = remoteMenu->addAction(tr("&Disconnect"), this, &MainWindow::sshDisconnect);
    m_sshDisconnectAction->setEnabled(false);
    remoteMenu->addSeparator();
    m_sshOpenAction = remoteMenu->addAction(tr("&Open Remote File..."), this, &MainWindow::sshOpenFile);
    m_sshOpenAction->setEnabled(false);

    // --- Settings ---
    menu->addSeparator();
    menu->addAction(tr("&Preferences..."), QKeySequence(Qt::CTRL | Qt::Key_Comma), this, &MainWindow::openSettings);
}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel(tr("Ln 1, Col 1"));
    m_fontSizeLabel = new QLabel(QString("Font: %1pt").arg(m_settings.editorFont.pointSize()));

    m_languageCombo = new QComboBox(this);
    m_languageCombo->setMaximumWidth(120);
    populateLanguageCombo();
    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onLanguageChanged);

    m_encodingCombo = new QComboBox(this);
    m_encodingCombo->setMaximumWidth(100);
    populateEncodingCombo();
    connect(m_encodingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onEncodingChanged);

    m_sshLabel = new QLabel;
    m_sshLabel->hide();

    statusBar()->addWidget(m_statusLabel);
    statusBar()->addPermanentWidget(m_sshLabel);
    statusBar()->addPermanentWidget(m_languageCombo);
    statusBar()->addPermanentWidget(m_fontSizeLabel);
    statusBar()->addPermanentWidget(m_encodingCombo);
}

void MainWindow::populateEncodingCombo()
{
    m_encodingCombo->blockSignals(true);
    m_encodingCombo->clear();
    for (const auto &e : LargeFileReader::supportedEncodings())
        m_encodingCombo->addItem(e.name, static_cast<int>(e.encoding));
    m_encodingCombo->blockSignals(false);
}

void MainWindow::populateLanguageCombo()
{
    m_languageCombo->blockSignals(true);
    m_languageCombo->clear();
    for (int i = 0; i <= static_cast<int>(CodeHighlighter::Asm); ++i) {
        auto lang = static_cast<CodeHighlighter::Language>(i);
        m_languageCombo->addItem(CodeHighlighter::languageName(lang), i);
    }
    m_languageCombo->blockSignals(false);
}

int MainWindow::createTab(const QString &title)
{
    auto *container = new QWidget(this);
    auto *hbox = new QHBoxLayout(container);
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(0);

    auto *editor = new Editor(container);
    editor->applySettings(m_settings);
    editor->applyTheme(EditorTheme::themeByName(m_settings.themeName));
    connectEditor(editor);

    auto *minimap = new Minimap(editor, container);
    minimap->setVisible(m_settings.minimap);
    minimap->setPalette(editor->palette());

    hbox->addWidget(editor, 1);
    hbox->addWidget(minimap);

    m_minimaps.insert(editor, minimap);

    TabData td;
    QString tabTitle = title.isEmpty() ? tr("Untitled") : title;
    int index = m_tabWidget->addTab(container, tabTitle);
    m_titleBar->tabBar()->addTab(tabTitle);
    m_tabs.insert(index, td);
    m_tabWidget->setCurrentIndex(index);
    m_titleBar->tabBar()->setCurrentIndex(index);
    return index;
}

void MainWindow::connectEditor(Editor *editor)
{
    connect(editor->document(), &QTextDocument::contentsChanged, this, [this, editor]() {
        if (m_ignoreTextChanged) return;
        // Find the tab index for this editor
        int tabIdx = -1;
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            auto *w = m_tabWidget->widget(i);
            if (w && w->findChild<Editor *>() == editor) { tabIdx = i; break; }
        }
        if (tabIdx < 0 || tabIdx >= m_tabs.size()) return;
        auto *td = &m_tabs[tabIdx];
        if (td->isPreview) return;
        QString saved = editor->property("savedContent").toString();
        bool dirty = (editor->toPlainText() != saved);
        td->modified = dirty;
        updateTabTitle(tabIdx);
        updateWindowTitle();
    });
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::updateStatusBar);
    connect(editor, &Editor::fontSizeChanged, this, [this](int size) {
        m_fontSizeLabel->setText(QString("Font: %1pt").arg(size));
    });
}

void MainWindow::disconnectEditor(Editor *editor)
{
    disconnect(editor, nullptr, this, nullptr);
    if (editor->document())
        disconnect(editor->document(), nullptr, this, nullptr);
}

Editor *MainWindow::currentEditor() const
{
    int idx = m_tabWidget->currentIndex();
    if (isPreviewTab(idx)) {
        int srcIdx = sourceIndexForPreview(idx);
        if (srcIdx >= 0) {
            auto *w = m_tabWidget->widget(srcIdx);
            return w ? w->findChild<Editor *>() : nullptr;
        }
        return nullptr;
    }
    QWidget *w = m_tabWidget->currentWidget();
    if (!w) return nullptr;
    return w->findChild<Editor *>();
}

TabData *MainWindow::currentTabData()
{
    int i = m_tabWidget->currentIndex();
    if (isPreviewTab(i)) {
        int si = sourceIndexForPreview(i);
        return (si >= 0 && si < m_tabs.size()) ? &m_tabs[si] : nullptr;
    }
    return (i >= 0 && i < m_tabs.size()) ? &m_tabs[i] : nullptr;
}

TabData *MainWindow::tabData(int index) { return (index >= 0 && index < m_tabs.size()) ? &m_tabs[index] : nullptr; }

int MainWindow::findTabByPath(const QString &path) const
{
    for (int i = 0; i < m_tabs.size(); ++i)
        if (m_tabs[i].filePath == path) return i;
    return -1;
}

bool MainWindow::isMarkdownFile(const QString &path)
{
    return path.endsWith(".md", Qt::CaseInsensitive) || path.endsWith(".markdown", Qt::CaseInsensitive);
}

void MainWindow::openFile(const QString &path)
{
    int existing = findTabByPath(path);
    if (existing >= 0) { m_tabWidget->setCurrentIndex(existing); return; }

    auto *td = currentTabData();
    int idx;
    if (td && td->filePath.isEmpty() && !td->modified && currentEditor() && currentEditor()->document()->isEmpty())
        idx = m_tabWidget->currentIndex();
    else
        idx = createTab();

    loadFileIntoTab(idx, path);
}

void MainWindow::newFile() { createTab(); }

void MainWindow::open()
{
    QStringList paths = QFileDialog::getOpenFileNames(this, tr("Open File"), {},
                    tr("All Files (*);;Markdown (*.md *.markdown);;Text (*.txt)"));
    for (const QString &path : paths) openFile(path);
}

void MainWindow::save()
{
    auto *td = currentTabData();
    if (!td) return;
    if (td->isRemote && !td->remotePath.isEmpty()) {
        if (!m_sshSession || !m_sshSession->isConnected()) {
            QMessageBox::warning(this, tr("Error"), tr("SSH connection lost."));
            return;
        }
        QByteArray data = currentEditor()->toPlainText().toUtf8();
        QApplication::setOverrideCursor(Qt::WaitCursor);
        QString errorMsg;
        bool ok = m_sshSession->writeFile(td->remotePath, data, &errorMsg);
        QApplication::restoreOverrideCursor();
        if (!ok) { QMessageBox::critical(this, tr("Error"), errorMsg); return; }
        td->modified = false;
        currentEditor()->setProperty("savedContent", currentEditor()->toPlainText());
        updateTabTitle(m_tabWidget->currentIndex());
        updateWindowTitle();
        return;
    }
    if (td->filePath.isEmpty()) saveAs();
    else saveFileFromTab(m_tabWidget->currentIndex(), td->filePath);
}

void MainWindow::saveAs()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save File"), {},
                    tr("All Files (*);;Markdown (*.md *.markdown);;Text (*.txt)"));
    if (!path.isEmpty()) saveFileFromTab(m_tabWidget->currentIndex(), path);
}

void MainWindow::closeTab(int index)
{
    // Closing a preview tab
    if (isPreviewTab(index)) {
        closePreviewTab(index);
        return;
    }

    if (index < 0 || index >= m_tabs.size()) return;

    // If closing the source tab of an open preview, close its preview first
    QWidget *srcWidget = m_tabWidget->widget(index);
    int pi = previewIndexForSource(index);
    if (pi >= 0) {
        closePreviewTab(pi);
        // Recalculate index (preview was after source, so source index stays same)
    }

    auto &td = m_tabs[index];
    if (td.modified) {
        m_tabWidget->setCurrentIndex(index);
        auto r = QMessageBox::question(this, tr("Unsaved Changes"),
                    tr("Save changes to %1?").arg(td.filePath.isEmpty() ? tr("Untitled") : QFileInfo(td.filePath).fileName()),
                    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (r == QMessageBox::Save) {
            if (td.filePath.isEmpty()) saveAs(); else saveFileFromTab(index, td.filePath);
        } else if (r == QMessageBox::Cancel) return;
    }

    QWidget *w = m_tabWidget->widget(index);
    auto *editor = w ? w->findChild<Editor *>() : nullptr;
    if (editor) {
        m_minimaps.remove(editor);
        disconnectEditor(editor);
    }
    m_tabWidget->removeTab(index);
    m_titleBar->tabBar()->removeTab(index);
    m_tabs.remove(index);

    if (w) w->deleteLater();

    // Check if only preview tabs remain
    bool hasEditorTab = false;
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (!m_tabs[i].isPreview) { hasEditorTab = true; break; }
    }
    if (!hasEditorTab) createTab();
}

void MainWindow::onTabChanged(int index)
{
    // Sync title bar tab selection
    if (m_titleBar->tabBar()->currentIndex() != index)
        m_titleBar->tabBar()->setCurrentIndex(index);

    if (isPreviewTab(index)) {
        m_previewAction->blockSignals(true);
        m_previewAction->setChecked(true);
        m_previewAction->blockSignals(false);
        return;
    }

    if (index < 0 || index >= m_tabs.size()) return;
    if (m_tabs[index].isPreview) return;

    auto &td = m_tabs[index];

    m_previewAction->blockSignals(true);
    m_previewAction->setChecked(previewIndexForSource(index) >= 0);
    m_previewAction->blockSignals(false);

    m_encodingCombo->blockSignals(true);
    int encIdx = m_encodingCombo->findText(LargeFileReader::encodingName(td.encoding));
    if (encIdx >= 0) m_encodingCombo->setCurrentIndex(encIdx);
    m_encodingCombo->blockSignals(false);

    m_languageCombo->blockSignals(true);
    if (td.isMarkdown)
        m_languageCombo->setCurrentIndex(m_languageCombo->findText("Plain Text"));
    else
        m_languageCombo->setCurrentIndex(static_cast<int>(td.language));
    m_languageCombo->blockSignals(false);

    updateStatusBar();
    updateWindowTitle();

    // Re-apply search highlights if search bar is visible
    if (m_searchBar->isVisible()) {
        auto *editor = currentEditor();
        if (editor) {
            editor->highlightSearchMatches(m_searchBar->searchText(), m_searchBar->isCaseSensitive());
            updateSearchMatchLabel();
        }
    }
}

void MainWindow::loadFileIntoTab(int index, const QString &path)
{
    QWidget *w = m_tabWidget->widget(index);
    auto *editor = w ? w->findChild<Editor *>() : nullptr;
    if (!editor || index >= m_tabs.size()) return;

    auto encoding = LargeFileReader::detectEncoding(path);

    qint64 size = LargeFileReader::fileSize(path);
    QProgressDialog *progress = nullptr;
    if (size > LargeFileReader::CHUNK_SIZE) {
        progress = new QProgressDialog(tr("Loading large file..."), tr("Cancel"), 0, 100, this);
        progress->setWindowModality(Qt::WindowModal);
        progress->show();
    }

    QString content = LargeFileReader::readAll(path, encoding, [progress](int pct) {
        if (progress) { progress->setValue(pct); QApplication::processEvents(); }
    });
    delete progress;

    if (content.isNull() && size > 0) {
        QMessageBox::warning(this, tr("Error"), tr("Could not open file."));
        return;
    }

    auto &td = m_tabs[index];
    td.filePath = path;
    td.isMarkdown = isMarkdownFile(path);
    td.encoding = encoding;
    td.language = td.isMarkdown ? CodeHighlighter::None : CodeHighlighter::detectLanguage(path);

    m_ignoreTextChanged = true;

    editor->clearHighlighter();
    editor->setPlainText(content);

    if (m_settings.syntaxHighlighting) {
        if (td.isMarkdown) editor->setMarkdownMode(true);
        else if (td.language != CodeHighlighter::None) editor->setLanguage(td.language);
    }

    // Re-apply theme so newly created highlighter gets themed colors
    editor->applyTheme(EditorTheme::themeByName(m_settings.themeName));

    m_ignoreTextChanged = false;

    td.modified = false;
    editor->setProperty("savedContent", editor->toPlainText());
    addToRecentFiles(path);
    updateTabTitle(index);
    if (td.isMarkdown)
        addPreviewButton(index);

    if (m_tabWidget->currentIndex() == index) onTabChanged(index);
    updateWindowTitle();
}

void MainWindow::reloadWithEncoding(QStringConverter::Encoding enc)
{
    auto *td = currentTabData();
    auto *editor = currentEditor();
    if (!td || !editor || td->filePath.isEmpty()) return;

    td->encoding = enc;
    QString content = LargeFileReader::readAll(td->filePath, enc);
    if (content.isNull()) {
        QMessageBox::warning(this, tr("Error"), tr("Could not reload file."));
        return;
    }

    m_ignoreTextChanged = true;

    editor->clearHighlighter();
    editor->setPlainText(content);

    if (m_settings.syntaxHighlighting) {
        if (td->isMarkdown) editor->setMarkdownMode(true);
        else if (td->language != CodeHighlighter::None) editor->setLanguage(td->language);
    }

    editor->applyTheme(EditorTheme::themeByName(m_settings.themeName));

    m_ignoreTextChanged = false;

    td->modified = false;
    editor->setProperty("savedContent", editor->toPlainText());
    updateTabTitle(m_tabWidget->currentIndex());
}

void MainWindow::saveFileFromTab(int index, const QString &path)
{
    QWidget *w = m_tabWidget->widget(index);
    auto *editor = w ? w->findChild<Editor *>() : nullptr;
    if (!editor || index >= m_tabs.size()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not save file."));
        return;
    }

    auto &td = m_tabs[index];
    QTextStream out(&file);
    out.setEncoding(td.encoding);
    QTextBlock block = editor->document()->begin();
    while (block.isValid()) {
        out << block.text();
        block = block.next();
        if (block.isValid()) out << '\n';
    }

    td.filePath = path;
    td.modified = false;
    editor->setProperty("savedContent", editor->toPlainText());

    bool wasMd = td.isMarkdown;
    td.isMarkdown = isMarkdownFile(path);
    auto newLang = td.isMarkdown ? CodeHighlighter::None : CodeHighlighter::detectLanguage(path);

    if (wasMd != td.isMarkdown || td.language != newLang) {
        td.language = newLang;
        m_ignoreTextChanged = true;
        editor->clearHighlighter();
        if (m_settings.syntaxHighlighting) {
            if (td.isMarkdown) editor->setMarkdownMode(true);
            else if (td.language != CodeHighlighter::None) editor->setLanguage(td.language);
        }
        editor->applyTheme(EditorTheme::themeByName(m_settings.themeName));
        m_ignoreTextChanged = false;
        if (td.isMarkdown)
            addPreviewButton(index);
    }

    updateTabTitle(index);
    updateWindowTitle();
}

void MainWindow::togglePreview()
{
    int idx = m_tabWidget->currentIndex();
    if (isPreviewTab(idx)) {
        closePreviewTab(idx);
    } else {
        // Check if this source already has a preview open
        QWidget *srcWidget = m_tabWidget->widget(idx);
        int existingPreview = previewIndexForSource(idx);
        if (existingPreview >= 0)
            closePreviewTab(existingPreview);
        else
            openPreviewTab(idx);
    }
}

void MainWindow::openPreviewTab(int sourceIndex)
{
    QWidget *srcWidget = m_tabWidget->widget(sourceIndex);
    if (!srcWidget || isPreviewTab(sourceIndex)) return;

    // Already has preview?
    if (previewIndexForSource(sourceIndex) >= 0) return;

    auto *editor = srcWidget->findChild<Editor *>();
    if (!editor) return;

    auto *preview = new MarkdownPreview(this);
    preview->setProperty("sourceEditor", QVariant::fromValue<QWidget *>(srcWidget));

    // Set dark mode based on current theme
    auto theme = EditorTheme::themeByName(m_settings.themeName);
    preview->setDarkMode(theme.background.lightnessF() < 0.5);

    m_mdPreviews.append(preview);

    // Live updates: direct connect from editor to preview
    connect(editor, &QPlainTextEdit::textChanged, preview, [preview, editor]() {
        preview->updatePreview(editor->toPlainText());
    });

    auto &srcTd = m_tabs[sourceIndex];
    QString srcName = srcTd.filePath.isEmpty() ? tr("Untitled") : QFileInfo(srcTd.filePath).fileName();

    int insertAt = sourceIndex + 1;
    QString previewTitle = tr("Preview: %1").arg(srcName);
    int previewIdx = m_tabWidget->insertTab(insertAt, preview, previewTitle);
    m_titleBar->tabBar()->insertTab(insertAt, previewTitle);
    m_tabWidget->setTabToolTip(previewIdx, tr("Markdown Preview"));
    m_tabWidget->setTabIcon(previewIdx,
        style()->standardIcon(QStyle::SP_FileDialogDetailedView));

    TabData previewTd;
    previewTd.isPreview = true;
    m_tabs.insert(previewIdx, previewTd);

    m_tabWidget->setCurrentIndex(previewIdx);

    // Initial render
    preview->updatePreview(editor->toPlainText());
}

void MainWindow::closePreviewTab(int previewIndex)
{
    if (previewIndex < 0 || previewIndex >= m_tabs.size()) return;
    if (!m_tabs[previewIndex].isPreview) return;

    auto *preview = qobject_cast<MarkdownPreview *>(m_tabWidget->widget(previewIndex));

    m_tabWidget->removeTab(previewIndex);
    m_titleBar->tabBar()->removeTab(previewIndex);
    m_tabs.remove(previewIndex);

    if (preview) {
        m_mdPreviews.removeOne(preview);
        preview->deleteLater();
    }
}

int MainWindow::sourceIndexForPreview(int previewIndex) const
{
    if (previewIndex < 0 || previewIndex >= m_tabs.size()) return -1;
    if (!m_tabs[previewIndex].isPreview) return -1;

    auto *preview = qobject_cast<MarkdownPreview *>(m_tabWidget->widget(previewIndex));
    if (!preview) return -1;

    auto *srcWidget = preview->property("sourceEditor").value<QWidget *>();
    if (!srcWidget) return -1;

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (m_tabWidget->widget(i) == srcWidget) return i;
    }
    return -1;
}

int MainWindow::previewIndexForSource(int sourceIndex) const
{
    QWidget *srcWidget = m_tabWidget->widget(sourceIndex);
    if (!srcWidget) return -1;

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *mdp = qobject_cast<MarkdownPreview *>(m_tabWidget->widget(i));
        if (mdp && mdp->property("sourceEditor").value<QWidget *>() == srcWidget)
            return i;
    }
    return -1;
}

bool MainWindow::isPreviewTab(int index) const
{
    return index >= 0 && index < m_tabs.size() && m_tabs[index].isPreview;
}

MarkdownPreview *MainWindow::currentMarkdownPreview() const
{
    return qobject_cast<MarkdownPreview *>(m_tabWidget->currentWidget());
}

void MainWindow::addPreviewButton(int tabIndex)
{
    auto *btn = new QToolButton(m_tabWidget);
    btn->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    btn->setToolTip(tr("Toggle Markdown Preview"));
    btn->setAutoRaise(true);
    btn->setIconSize(QSize(14, 14));
    btn->setFixedSize(20, 20);
    QWidget *tabWidget = m_tabWidget->widget(tabIndex);
    connect(btn, &QToolButton::clicked, this, [this, tabWidget]() {
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            if (m_tabWidget->widget(i) == tabWidget) {
                int pi = previewIndexForSource(i);
                if (pi >= 0)
                    closePreviewTab(pi);
                else
                    openPreviewTab(i);
                return;
            }
        }
    });
    m_titleBar->tabBar()->setTabButton(tabIndex, QTabBar::LeftSide, btn);
}

void MainWindow::openSettings()
{
    // Reload themes from disk (picks up any external changes)
    QString themesDir = QDir::homePath() + "/.config/ed/themes";
    EditorTheme::loadFromDirectory(themesDir);

    SettingsDialog dlg(m_settings, this);
    if (dlg.exec() == QDialog::Accepted) {
        AppSettings newSettings = dlg.settings();

        bool highlightChanged = newSettings.syntaxHighlighting != m_settings.syntaxHighlighting;
        m_settings = newSettings;

        // Apply GUI font
        QApplication::setFont(m_settings.guiFont);

        // Apply to all editors
        m_ignoreTextChanged = true;
        applySettingsToAllEditors();

        // If highlighting toggled, re-apply or remove highlighters
        if (highlightChanged) {
            for (int i = 0; i < m_tabWidget->count(); ++i) {
                auto *editor = m_tabWidget->widget(i)->findChild<Editor *>();
                auto *td = tabData(i);
                if (!editor || !td) continue;

                editor->clearHighlighter();
                if (m_settings.syntaxHighlighting) {
                    if (td->isMarkdown) editor->setMarkdownMode(true);
                    else if (td->language != CodeHighlighter::None) editor->setLanguage(td->language);
                }
            }
        }
        m_ignoreTextChanged = false;

        // Apply theme (must be after highlighters are set up)
        applyThemeToApp(EditorTheme::themeByName(m_settings.themeName));

        m_fontSizeLabel->setText(QString("Font: %1pt").arg(m_settings.editorFont.pointSize()));
        updateAutoSaveTimer();
        updateMinimaps();
        saveSettings();
    }
}

void MainWindow::applySettingsToAllEditors()
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *editor = m_tabWidget->widget(i)->findChild<Editor *>();
        if (editor) editor->applySettings(m_settings);
    }
}

void MainWindow::applyThemeToApp(const EditorTheme &theme)
{
    bool isDark = theme.background.lightnessF() < 0.5;
    for (auto *mdp : m_mdPreviews)
        mdp->setDarkMode(isDark);

    // Style the custom title bar with theme colors
    m_titleBar->applyTheme(theme.lineNumberBg, theme.lineNumberFg,
                           theme.background, theme.foreground, theme.selection);

    m_tabWidget->setStyleSheet("QTabWidget::pane { border: none; }");

    // Style combo boxes in status bar with theme colors
    QString comboStyle = QString(
        "QComboBox { background: %1; color: %2; border: none; padding: 2px 6px; }"
        "QComboBox:hover { background: %3; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background: %1; color: %2; selection-background-color: %3; }"
    ).arg(theme.lineNumberBg.name(), theme.foreground.name(), theme.selection.name());
    m_languageCombo->setStyleSheet(comboStyle);
    m_encodingCombo->setStyleSheet(comboStyle);

    // Style all widgets via one setStyleSheet on main window (NOT qApp — that kills palettes)
    QColor sbBg = theme.lineNumberBg;
    QColor sbHandle = theme.lineNumberFg;
    QColor sbHover = theme.selection;
    setStyleSheet(QString(
        "QMainWindow { border: none; background: %6; }"
        "QStatusBar { background: %6; color: %7; }"
        "QStatusBar QLabel { color: %7; }"
        "QPlainTextEdit { background: %8; color: %7; }"
        "QScrollBar:vertical { background: %1; width: 12px; margin: 0; }"
        "QScrollBar::handle:vertical { background: %2; min-height: 20px; border-radius: 3px; margin: 2px; }"
        "QScrollBar::handle:vertical:hover { background: %3; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
        "QScrollBar:horizontal { background: %1; height: 12px; margin: 0; }"
        "QScrollBar::handle:horizontal { background: %2; min-width: 20px; border-radius: 3px; margin: 2px; }"
        "QScrollBar::handle:horizontal:hover { background: %3; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }"
        "#dialogTitleBar { background: %4; }"
        "#dialogTitleBar QLabel { color: %5; font-weight: bold; }"
        "#dialogCloseBtn { border: none; background: %4; color: %5; font-size: 14px; }"
        "#dialogCloseBtn:hover { background: #e81123; color: white; }"
    ).arg(sbBg.name(), sbHandle.name(), sbHover.name(),
          theme.lineNumberBg.name(), theme.foreground.name(),
          theme.lineNumberBg.name(), theme.foreground.name(),
          theme.background.name()));

    if (isDark) {
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, theme.lineNumberBg);
        darkPalette.setColor(QPalette::WindowText, theme.foreground);
        darkPalette.setColor(QPalette::Base, theme.background);
        darkPalette.setColor(QPalette::AlternateBase, theme.lineNumberBg);
        darkPalette.setColor(QPalette::Text, theme.foreground);
        darkPalette.setColor(QPalette::Button, theme.lineNumberBg);
        darkPalette.setColor(QPalette::ButtonText, theme.foreground);
        darkPalette.setColor(QPalette::BrightText, Qt::white);
        darkPalette.setColor(QPalette::Highlight, theme.selection);
        darkPalette.setColor(QPalette::HighlightedText, theme.foreground);
        darkPalette.setColor(QPalette::ToolTipBase, theme.lineNumberBg);
        darkPalette.setColor(QPalette::ToolTipText, theme.foreground);
        darkPalette.setColor(QPalette::Link, theme.keyword);
        darkPalette.setColor(QPalette::Disabled, QPalette::Text, theme.lineNumberFg);
        darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, theme.lineNumberFg);
        darkPalette.setColor(QPalette::Mid, theme.lineNumberBg);
        QApplication::setPalette(darkPalette);
    } else {
        QApplication::setPalette(QApplication::style()->standardPalette());
    }

    // Apply theme to all editors AFTER global palette is set
    // (QApplication::setPalette overrides widget palettes, so editors must come last)
    m_ignoreTextChanged = true;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *editor = m_tabWidget->widget(i)->findChild<Editor *>();
        if (editor) editor->applyTheme(theme);
    }
    m_ignoreTextChanged = false;

    // Force repaint of all widgets
    for (auto *w : findChildren<QWidget *>())
        w->update();
}

void MainWindow::updateStatusBar()
{
    auto *editor = currentEditor();
    if (!editor) return;
    QTextCursor c = editor->textCursor();
    m_statusLabel->setText(QString("Ln %1, Col %2").arg(c.blockNumber() + 1).arg(c.columnNumber() + 1));
}






void MainWindow::onEncodingChanged(int comboIndex)
{
    if (comboIndex < 0) return;
    auto enc = static_cast<QStringConverter::Encoding>(m_encodingCombo->itemData(comboIndex).toInt());
    auto *td = currentTabData();
    if (!td || td->encoding == enc) return;

    if (!td->filePath.isEmpty()) reloadWithEncoding(enc);
    else td->encoding = enc;
}

void MainWindow::onLanguageChanged(int comboIndex)
{
    if (comboIndex < 0) return;
    auto lang = static_cast<CodeHighlighter::Language>(m_languageCombo->itemData(comboIndex).toInt());
    auto *td = currentTabData();
    auto *editor = currentEditor();
    if (!td || !editor) return;

    td->language = lang;
    td->isMarkdown = false;

    m_ignoreTextChanged = true;
    editor->clearHighlighter();

    if (m_settings.syntaxHighlighting && lang != CodeHighlighter::None)
        editor->setLanguage(lang);

    editor->applyTheme(EditorTheme::themeByName(m_settings.themeName));
    m_ignoreTextChanged = false;
}

void MainWindow::updateTabTitle(int index)
{
    if (index < 0 || index >= m_tabs.size()) return;
    auto &td = m_tabs[index];
    QString name = td.filePath.isEmpty() ? tr("Untitled") : QFileInfo(td.filePath).fileName();
    if (td.isRemote) name = "[SSH] " + name;
    if (td.modified) name = "* " + name;
    m_tabWidget->setTabText(index, name);
    if (index < m_titleBar->tabBar()->count())
        m_titleBar->tabBar()->setTabText(index, name);
}

void MainWindow::updateWindowTitle()
{
    auto *td = currentTabData();
    QString title = "TextEd";
    if (td && !td->filePath.isEmpty())
        title = QFileInfo(td->filePath).fileName() + " - TextEd";
    else if (td)
        title = tr("Untitled") + " - TextEd";
    if (td && td->modified) title = "* " + title;
    setWindowTitle(title);
    m_titleBar->titleLabel()->setText(title);
}

void MainWindow::sshConnect()
{
    SshConnectDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    SshHostInfo info = dlg.hostInfo();
    if (info.host.isEmpty() || info.username.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Host and username are required."));
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);

    if (!m_sshSession)
        m_sshSession = new SshSession;
    else
        m_sshSession->disconnect();

    QString errorMsg;
    bool ok = m_sshSession->connect(info, &errorMsg);
    QApplication::restoreOverrideCursor();

    if (!ok) {
        QMessageBox::critical(this, tr("SSH Connection Failed"), errorMsg);
        return;
    }

    m_sshDisconnectAction->setEnabled(true);
    m_sshOpenAction->setEnabled(true);
    m_sshLabel->setText(QString(" SSH: %1 ").arg(m_sshSession->hostLabel()));
    m_sshLabel->setStyleSheet("QLabel { color: #ffffff; background: #2e7d32; border-radius: 3px; padding: 1px 4px; }");
    m_sshLabel->show();
}

void MainWindow::sshDisconnect()
{
    if (m_sshSession) {
        m_sshSession->disconnect();
        delete m_sshSession;
        m_sshSession = nullptr;
    }
    m_sshDisconnectAction->setEnabled(false);
    m_sshOpenAction->setEnabled(false);
    m_sshLabel->hide();
}

void MainWindow::sshOpenFile()
{
    if (!m_sshSession || !m_sshSession->isConnected()) {
        QMessageBox::warning(this, tr("Error"), tr("Not connected to SSH server."));
        return;
    }

    RemoteFileBrowser browser(m_sshSession, this);
    browser.setMode(RemoteFileBrowser::Open);
    if (browser.exec() != QDialog::Accepted) return;

    QString remotePath = browser.selectedPath();
    if (remotePath.isEmpty()) return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString errorMsg;
    QByteArray data = m_sshSession->readFile(remotePath, &errorMsg);
    QApplication::restoreOverrideCursor();

    if (data.isNull() && !errorMsg.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), errorMsg);
        return;
    }

    QString content = QString::fromUtf8(data);

    // Reuse empty tab or create new
    auto *td = currentTabData();
    int idx;
    if (td && td->filePath.isEmpty() && !td->modified && currentEditor() && currentEditor()->document()->isEmpty())
        idx = m_tabWidget->currentIndex();
    else
        idx = createTab();

    auto *editor = m_tabWidget->widget(idx)->findChild<Editor *>();
    if (!editor) return;

    auto &tab = m_tabs[idx];
    tab.filePath = QFileInfo(remotePath).fileName();
    tab.isRemote = true;
    tab.remotePath = remotePath;
    tab.isMarkdown = isMarkdownFile(remotePath);
    tab.language = tab.isMarkdown ? CodeHighlighter::None : CodeHighlighter::detectLanguage(remotePath);

    m_ignoreTextChanged = true;

    editor->clearHighlighter();
    editor->setPlainText(content);

    if (m_settings.syntaxHighlighting) {
        if (tab.isMarkdown) editor->setMarkdownMode(true);
        else if (tab.language != CodeHighlighter::None) editor->setLanguage(tab.language);
    }

    editor->applyTheme(EditorTheme::themeByName(m_settings.themeName));

    m_ignoreTextChanged = false;

    tab.modified = false;
    editor->setProperty("savedContent", editor->toPlainText());

    updateTabTitle(idx);
    if (tab.isMarkdown)
        addPreviewButton(idx);
    m_tabWidget->setTabToolTip(idx, m_sshSession->hostLabel() + ":" + remotePath);

    if (m_tabWidget->currentIndex() == idx) onTabChanged(idx);
    updateWindowTitle();
}

void MainWindow::setupSearchBar()
{
    m_searchBar = new SearchBar(this);

    connect(m_searchBar, &SearchBar::searchTextChanged, this, &MainWindow::onSearchTextChanged);
    connect(m_searchBar, &SearchBar::findNext, this, &MainWindow::onFindNext);
    connect(m_searchBar, &SearchBar::findPrev, this, &MainWindow::onFindPrev);
    connect(m_searchBar, &SearchBar::replaceOne, this, &MainWindow::onReplaceOne);
    connect(m_searchBar, &SearchBar::replaceAll, this, &MainWindow::onReplaceAll);
    connect(m_searchBar, &SearchBar::closed, this, &MainWindow::onSearchClosed);

    // F3 / Shift+F3 shortcuts work globally
    auto *f3 = new QShortcut(QKeySequence(Qt::Key_F3), this);
    connect(f3, &QShortcut::activated, this, &MainWindow::onFindNext);
    auto *sf3 = new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F3), this);
    connect(sf3, &QShortcut::activated, this, &MainWindow::onFindPrev);
}

void MainWindow::showSearchBar(bool withReplace)
{
    m_searchBar->setReplaceVisible(withReplace);
    m_searchBar->show();
    m_searchBar->focusSearchField();

    // If there is selected text, use it as search text
    auto *editor = currentEditor();
    if (editor) {
        QString sel = editor->textCursor().selectedText();
        if (!sel.isEmpty() && !sel.contains(QChar::ParagraphSeparator)) {
            // Set text in the search field (will trigger searchTextChanged via signal)
            // We need to access the line edit -- use the public searchText after setting
            // Actually, SearchBar doesn't expose setText. Let's just trigger the search
            // with what's already there.
        }
    }
}

void MainWindow::onSearchTextChanged(const QString &text, bool caseSensitive)
{
    auto *editor = currentEditor();
    if (!editor) return;
    editor->highlightSearchMatches(text, caseSensitive);
    updateSearchMatchLabel();
}

void MainWindow::onFindNext()
{
    auto *editor = currentEditor();
    if (!editor) return;
    editor->goToNextMatch();
    updateSearchMatchLabel();
}

void MainWindow::onFindPrev()
{
    auto *editor = currentEditor();
    if (!editor) return;
    editor->goToPrevMatch();
    updateSearchMatchLabel();
}

void MainWindow::onReplaceOne(const QString &replaceWith)
{
    auto *editor = currentEditor();
    if (!editor) return;
    editor->replaceCurrentMatch(replaceWith);
    updateSearchMatchLabel();
}

void MainWindow::onReplaceAll(const QString &findText, const QString &replaceWith, bool caseSensitive)
{
    auto *editor = currentEditor();
    if (!editor) return;
    editor->replaceAllMatches(findText, replaceWith, caseSensitive);
    updateSearchMatchLabel();
}

void MainWindow::onSearchClosed()
{
    auto *editor = currentEditor();
    if (editor) {
        editor->clearSearchHighlights();
        editor->setFocus();
    }
}

void MainWindow::updateSearchMatchLabel()
{
    auto *editor = currentEditor();
    if (!editor) return;
    int total = editor->searchMatchCount();
    int current = (total > 0) ? editor->currentMatchIndex() + 1 : 0;
    m_searchBar->updateMatchLabel(current, total);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs[i].isPreview || !m_tabs[i].modified) continue;
        m_tabWidget->setCurrentIndex(i);
        QString name = m_tabs[i].filePath.isEmpty() ? tr("Untitled") : QFileInfo(m_tabs[i].filePath).fileName();
        auto r = QMessageBox::question(this, tr("Unsaved Changes"),
                    tr("Save changes to %1?").arg(name),
                    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (r == QMessageBox::Save) {
            if (m_tabs[i].filePath.isEmpty()) {
                saveAs();
                if (m_tabs[i].modified) { event->ignore(); return; }
            } else {
                saveFileFromTab(i, m_tabs[i].filePath);
            }
        } else if (r == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }
    saveSettings();
    saveSession();
    saveRecentFiles();
    event->accept();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    for (const QUrl &url : event->mimeData()->urls()) {
        if (url.isLocalFile())
            openFile(url.toLocalFile());
    }
}

void MainWindow::autoSaveAll()
{
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs[i].isPreview) continue;
        if (m_tabs[i].modified && !m_tabs[i].filePath.isEmpty() && !m_tabs[i].isRemote) {
            saveFileFromTab(i, m_tabs[i].filePath);
        }
    }
}

void MainWindow::updateAutoSaveTimer()
{
    if (m_settings.autoSave && m_settings.autoSaveInterval > 0) {
        m_autoSaveTimer->start(m_settings.autoSaveInterval * 1000);
    } else {
        m_autoSaveTimer->stop();
    }
}

void MainWindow::updateMinimaps()
{
    for (auto it = m_minimaps.begin(); it != m_minimaps.end(); ++it) {
        it.value()->setVisible(m_settings.minimap);
    }
}

void MainWindow::addToRecentFiles(const QString &path)
{
    m_recentFiles.removeAll(path);
    m_recentFiles.prepend(path);
    while (m_recentFiles.size() > MaxRecentFiles)
        m_recentFiles.removeLast();
    updateRecentFilesMenu();
    saveRecentFiles();
}

void MainWindow::loadRecentFiles()
{
    QSettings settings("TextEd", "TextEd");
    m_recentFiles = settings.value("recentFiles").toStringList();
    while (m_recentFiles.size() > MaxRecentFiles)
        m_recentFiles.removeLast();
}

void MainWindow::saveRecentFiles()
{
    QSettings settings("TextEd", "TextEd");
    settings.setValue("recentFiles", m_recentFiles);
}

void MainWindow::updateRecentFilesMenu()
{
    if (!m_recentFilesMenu) return;
    m_recentFilesMenu->clear();
    if (m_recentFiles.isEmpty()) {
        m_recentFilesMenu->addAction(tr("(empty)"))->setEnabled(false);
        return;
    }
    for (const QString &path : m_recentFiles) {
        m_recentFilesMenu->addAction(QFileInfo(path).fileName(), this, [this, path]() {
            if (QFileInfo::exists(path))
                openFile(path);
            else {
                QMessageBox::warning(this, tr("Error"), tr("File not found: %1").arg(path));
                m_recentFiles.removeAll(path);
                updateRecentFilesMenu();
                saveRecentFiles();
            }
        })->setToolTip(path);
    }
    m_recentFilesMenu->addSeparator();
    m_recentFilesMenu->addAction(tr("Clear Recent"), this, [this]() {
        m_recentFiles.clear();
        updateRecentFilesMenu();
        saveRecentFiles();
    });
}

void MainWindow::saveSettings()
{
    QSettings s("TextEd", "TextEd");
    s.beginGroup("settings");
    s.setValue("editorFontFamily", m_settings.editorFont.family());
    s.setValue("editorFontSize", m_settings.editorFont.pointSize());
    s.setValue("guiFontFamily", m_settings.guiFont.family());
    s.setValue("guiFontSize", m_settings.guiFont.pointSize());
    s.setValue("syntaxHighlighting", m_settings.syntaxHighlighting);
    s.setValue("lineNumbers", m_settings.lineNumbers);
    s.setValue("lineWrap", m_settings.lineWrap);
    s.setValue("highlightCurrentLine", m_settings.highlightCurrentLine);
    s.setValue("showWhitespace", m_settings.showWhitespace);
    s.setValue("tabWidth", m_settings.tabWidth);
    s.setValue("themeName", m_settings.themeName);
    s.setValue("autoIndent", m_settings.autoIndent);
    s.setValue("bracketMatching", m_settings.bracketMatching);
    s.setValue("autoSave", m_settings.autoSave);
    s.setValue("autoSaveInterval", m_settings.autoSaveInterval);
    s.setValue("minimap", m_settings.minimap);
    s.setValue("showRuler", m_settings.showRuler);
    s.setValue("rulerColumn", m_settings.rulerColumn);
    s.setValue("lineSpacing", m_settings.lineSpacing);
    s.endGroup();
}

void MainWindow::loadSettings()
{
    QSettings s("TextEd", "TextEd");
    s.beginGroup("settings");
    if (s.contains("editorFontFamily")) {
        QString family = s.value("editorFontFamily", "Monospace").toString();
        int size = s.value("editorFontSize", 11).toInt();
        m_settings.editorFont = QFont(family, size);
    }
    if (s.contains("guiFontFamily")) {
        QString family = s.value("guiFontFamily").toString();
        int size = s.value("guiFontSize", 10).toInt();
        m_settings.guiFont = QFont(family, size);
    }
    m_settings.syntaxHighlighting = s.value("syntaxHighlighting", m_settings.syntaxHighlighting).toBool();
    m_settings.lineNumbers = s.value("lineNumbers", m_settings.lineNumbers).toBool();
    m_settings.lineWrap = s.value("lineWrap", m_settings.lineWrap).toBool();
    m_settings.highlightCurrentLine = s.value("highlightCurrentLine", m_settings.highlightCurrentLine).toBool();
    m_settings.showWhitespace = s.value("showWhitespace", m_settings.showWhitespace).toBool();
    m_settings.tabWidth = s.value("tabWidth", m_settings.tabWidth).toInt();
    m_settings.themeName = s.value("themeName", m_settings.themeName).toString();
    m_settings.autoIndent = s.value("autoIndent", m_settings.autoIndent).toBool();
    m_settings.bracketMatching = s.value("bracketMatching", m_settings.bracketMatching).toBool();
    m_settings.autoSave = s.value("autoSave", m_settings.autoSave).toBool();
    m_settings.autoSaveInterval = s.value("autoSaveInterval", m_settings.autoSaveInterval).toInt();
    m_settings.minimap = s.value("minimap", m_settings.minimap).toBool();
    m_settings.showRuler = s.value("showRuler", m_settings.showRuler).toBool();
    m_settings.rulerColumn = s.value("rulerColumn", m_settings.rulerColumn).toInt();
    m_settings.lineSpacing = s.value("lineSpacing", m_settings.lineSpacing).toDouble();
    s.endGroup();
}

void MainWindow::saveSession()
{
    QSettings settings("TextEd", "TextEd");
    QStringList sessionFiles;
    int activeTab = m_tabWidget->currentIndex();
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs[i].isPreview) continue;
        if (!m_tabs[i].filePath.isEmpty() && !m_tabs[i].isRemote && QFileInfo::exists(m_tabs[i].filePath))
            sessionFiles.append(m_tabs[i].filePath);
    }
    settings.setValue("session/files", sessionFiles);
    settings.setValue("session/activeTab", activeTab);
}

void MainWindow::restoreSession()
{
    QSettings settings("TextEd", "TextEd");
    QStringList sessionFiles = settings.value("session/files").toStringList();
    int activeTab = settings.value("session/activeTab", 0).toInt();

    bool opened = false;
    for (const QString &path : sessionFiles) {
        if (QFileInfo::exists(path)) {
            openFile(path);
            opened = true;
        }
    }

    if (!opened)
        createTab();

    if (activeTab >= 0 && activeTab < m_tabWidget->count())
        m_tabWidget->setCurrentIndex(activeTab);
}
