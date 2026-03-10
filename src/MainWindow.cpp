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

#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
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
#include <QShortcut>
#include <QSettings>
#include <QCloseEvent>
#include <QMenu>
#include <QInputDialog>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    m_settings.editorFont = QFont("Monospace", 11);
    m_settings.guiFont = QApplication::font();

    auto *centralContainer = new QWidget(this);
    auto *centralLayout = new QVBoxLayout(centralContainer);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);

    m_splitter = new QSplitter(Qt::Horizontal, this);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    m_preview = new MarkdownPreview(this);
    m_preview->hide();

    m_splitter->addWidget(m_tabWidget);
    m_splitter->addWidget(m_preview);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);

    centralLayout->addWidget(m_splitter, 1);

    setupSearchBar();
    centralLayout->addWidget(m_searchBar);

    setCentralWidget(centralContainer);

    m_previewTimer = new QTimer(this);
    m_previewTimer->setSingleShot(true);
    m_previewTimer->setInterval(300);
    connect(m_previewTimer, &QTimer::timeout, this, &MainWindow::updatePreviewContent);

    loadRecentFiles();
    setupMenus();
    setupStatusBar();

    m_autoSaveTimer = new QTimer(this);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::autoSaveAll);
    updateAutoSaveTimer();

    setAcceptDrops(true);

    restoreSession();
    updateWindowTitle();
}

void MainWindow::setupMenus()
{
    auto *fileMenu = menuBar()->addMenu(tr("&File"));
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

    auto *editMenu = menuBar()->addMenu(tr("&Edit"));
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

    auto *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(tr("Zoom &In"), QKeySequence(Qt::CTRL | Qt::Key_Plus), this, [this]() { if (auto *e = currentEditor()) e->zoomInEditor(); });
    viewMenu->addAction(tr("Zoom &Out"), QKeySequence(Qt::CTRL | Qt::Key_Minus), this, [this]() { if (auto *e = currentEditor()) e->zoomOutEditor(); });
    viewMenu->addSeparator();
    m_previewAction = viewMenu->addAction(tr("Markdown &Preview"));
    m_previewAction->setCheckable(true);
    m_previewAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));
    connect(m_previewAction, &QAction::toggled, this, &MainWindow::togglePreview);

    viewMenu->addAction(tr("Export Preview to &PDF..."), this, [this]() {
        if (!m_preview->isVisible()) {
            QMessageBox::information(this, tr("Export PDF"), tr("Preview must be open first."));
            return;
        }
        QString path = QFileDialog::getSaveFileName(this, tr("Export to PDF"), {},
                            tr("PDF Files (*.pdf)"));
        if (!path.isEmpty()) {
            if (!path.endsWith(".pdf", Qt::CaseInsensitive)) path += ".pdf";
            m_preview->exportToPdf(path);
        }
    });

    auto *remoteMenu = menuBar()->addMenu(tr("&Remote"));
    remoteMenu->addAction(tr("&Connect to SSH..."), this, &MainWindow::sshConnect);
    m_sshDisconnectAction = remoteMenu->addAction(tr("&Disconnect"), this, &MainWindow::sshDisconnect);
    m_sshDisconnectAction->setEnabled(false);
    remoteMenu->addSeparator();
    m_sshOpenAction = remoteMenu->addAction(tr("&Open Remote File..."), this, &MainWindow::sshOpenFile);
    m_sshOpenAction->setEnabled(false);

    auto *settingsMenu = menuBar()->addMenu(tr("&Settings"));
    settingsMenu->addAction(tr("&Preferences..."), QKeySequence(Qt::CTRL | Qt::Key_Comma), this, &MainWindow::openSettings);
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
    int index = m_tabWidget->addTab(container, title.isEmpty() ? tr("Untitled") : title);
    m_tabs.insert(index, td);
    m_tabWidget->setCurrentIndex(index);
    return index;
}

void MainWindow::connectEditor(Editor *editor)
{
    connect(editor, &QPlainTextEdit::textChanged, this, &MainWindow::onTextChanged);
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::updateStatusBar);
    connect(editor, &Editor::fontSizeChanged, this, [this](int size) {
        m_fontSizeLabel->setText(QString("Font: %1pt").arg(size));
    });
}

void MainWindow::disconnectEditor(Editor *editor) { disconnect(editor, nullptr, this, nullptr); }
Editor *MainWindow::currentEditor() const
{
    QWidget *w = m_tabWidget->currentWidget();
    if (!w) return nullptr;
    return w->findChild<Editor *>();
}

TabData *MainWindow::currentTabData()
{
    int i = m_tabWidget->currentIndex();
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
    if (index < 0 || index >= m_tabs.size()) return;

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
    m_tabs.remove(index);
    if (w) w->deleteLater();
    if (m_tabWidget->count() == 0) createTab();
}

void MainWindow::onTabChanged(int index)
{
    if (index < 0 || index >= m_tabs.size()) return;

    auto &td = m_tabs[index];

    m_previewAction->blockSignals(true);
    m_previewAction->setChecked(td.previewVisible);
    m_previewAction->blockSignals(false);

    m_preview->setVisible(td.previewVisible);
    if (td.previewVisible) {
        int total = m_splitter->width();
        m_splitter->setSizes({total / 2, total / 2});
        updatePreviewContent();
    }

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

    editor->clearHighlighter();
    editor->setPlainText(content);

    if (m_settings.syntaxHighlighting) {
        if (td.isMarkdown) editor->setMarkdownMode(true);
        else if (td.language != CodeHighlighter::None) editor->setLanguage(td.language);
    }

    td.modified = false;
    editor->document()->setModified(false);
    addToRecentFiles(path);
    updateTabTitle(index);
    m_tabWidget->setTabIcon(index, td.isMarkdown ?
        style()->standardIcon(QStyle::SP_FileDialogDetailedView) : QIcon());

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

    editor->clearHighlighter();
    editor->setPlainText(content);

    if (m_settings.syntaxHighlighting) {
        if (td->isMarkdown) editor->setMarkdownMode(true);
        else if (td->language != CodeHighlighter::None) editor->setLanguage(td->language);
    }

    td->modified = false;
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

    bool wasMd = td.isMarkdown;
    td.isMarkdown = isMarkdownFile(path);
    auto newLang = td.isMarkdown ? CodeHighlighter::None : CodeHighlighter::detectLanguage(path);

    if (wasMd != td.isMarkdown || td.language != newLang) {
        td.language = newLang;
        editor->clearHighlighter();
        if (m_settings.syntaxHighlighting) {
            if (td.isMarkdown) editor->setMarkdownMode(true);
            else if (td.language != CodeHighlighter::None) editor->setLanguage(td.language);
        }
        m_tabWidget->setTabIcon(index, td.isMarkdown ?
            style()->standardIcon(QStyle::SP_FileDialogDetailedView) : QIcon());
    }

    updateTabTitle(index);
    updateWindowTitle();
}

void MainWindow::togglePreview()
{
    bool show = m_previewAction->isChecked();
    auto *td = currentTabData();
    if (td) td->previewVisible = show;

    m_preview->setVisible(show);
    if (show) {
        int total = m_splitter->width();
        m_splitter->setSizes({total / 2, total / 2});
        updatePreviewContent();
    }
}

void MainWindow::openSettings()
{
    SettingsDialog dlg(m_settings, this);
    if (dlg.exec() == QDialog::Accepted) {
        AppSettings newSettings = dlg.settings();

        bool highlightChanged = newSettings.syntaxHighlighting != m_settings.syntaxHighlighting;
        m_settings = newSettings;

        // Apply GUI font
        QApplication::setFont(m_settings.guiFont);

        // Apply to all editors
        applySettingsToAllEditors();

        // Apply theme
        const EditorTheme &theme = EditorTheme::themeByName(m_settings.themeName);
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            auto *editor = m_tabWidget->widget(i)->findChild<Editor *>();
            if (editor) editor->applyTheme(theme);
        }

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

        m_fontSizeLabel->setText(QString("Font: %1pt").arg(m_settings.editorFont.pointSize()));
        updateAutoSaveTimer();
        updateMinimaps();
    }
}

void MainWindow::applySettingsToAllEditors()
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *editor = m_tabWidget->widget(i)->findChild<Editor *>();
        if (editor) editor->applySettings(m_settings);
    }
}

void MainWindow::updateStatusBar()
{
    auto *editor = currentEditor();
    if (!editor) return;
    QTextCursor c = editor->textCursor();
    m_statusLabel->setText(QString("Ln %1, Col %2").arg(c.blockNumber() + 1).arg(c.columnNumber() + 1));
}

void MainWindow::onTextChanged()
{
    auto *td = currentTabData();
    if (!td) return;
    td->modified = true;
    updateTabTitle(m_tabWidget->currentIndex());
    updateWindowTitle();
    if (m_preview->isVisible()) m_previewTimer->start();
}

void MainWindow::updatePreviewContent()
{
    auto *editor = currentEditor();
    if (editor) m_preview->updatePreview(editor->toPlainText());
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
    editor->clearHighlighter();

    if (m_settings.syntaxHighlighting && lang != CodeHighlighter::None)
        editor->setLanguage(lang);
}

void MainWindow::updateTabTitle(int index)
{
    if (index < 0 || index >= m_tabs.size()) return;
    auto &td = m_tabs[index];
    QString name = td.filePath.isEmpty() ? tr("Untitled") : QFileInfo(td.filePath).fileName();
    if (td.isRemote) name = "[SSH] " + name;
    if (td.modified) name = "* " + name;
    m_tabWidget->setTabText(index, name);
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
    tab.modified = false;
    tab.language = tab.isMarkdown ? CodeHighlighter::None : CodeHighlighter::detectLanguage(remotePath);

    editor->clearHighlighter();
    editor->setPlainText(content);

    if (m_settings.syntaxHighlighting) {
        if (tab.isMarkdown) editor->setMarkdownMode(true);
        else if (tab.language != CodeHighlighter::None) editor->setLanguage(tab.language);
    }

    updateTabTitle(idx);
    m_tabWidget->setTabIcon(idx, tab.isMarkdown ?
        style()->standardIcon(QStyle::SP_FileDialogDetailedView) : QIcon());
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
        if (!m_tabs[i].modified) continue;
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

void MainWindow::saveSession()
{
    QSettings settings("TextEd", "TextEd");
    QStringList sessionFiles;
    int activeTab = m_tabWidget->currentIndex();
    for (int i = 0; i < m_tabs.size(); ++i) {
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
