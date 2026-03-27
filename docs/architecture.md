# Architecture

## Overview

TextEd is a Qt 6 Widgets application written in C++17. It follows a modular design where each major feature is encapsulated in its own class. The application uses `QPlainTextEdit` as the core editor widget, `QWebEngineView` (wrapped in a `QWidget`) for Markdown preview rendering, `cmark-gfm` for Markdown parsing, and `libssh2` for SSH/SFTP remote file access.

## Component Diagram

```
MainWindow (frameless window)
├── TitleBar (custom title bar)
│   ├── QToolButton (hamburger menu ☰)
│   ├── QTabBar (document tabs with close buttons)
│   └── Window controls (minimize, maximize, close)
├── QTabWidget (multi-document content, tab bar hidden)
│   ├── QWidget container (per editor tab)
│   │   ├── Editor
│   │   │   ├── SpacedDocumentLayout (line spacing)
│   │   │   ├── LineNumberArea
│   │   │   ├── ScrollBarOverlay (search match markers)
│   │   │   ├── MarkdownHighlighter (for .md files, dark/light aware)
│   │   │   └── CodeHighlighter (for code files)
│   │   └── Minimap (VS Code-style code overview)
│   └── MarkdownPreview (per preview tab, QWidget + QWebEngineView)
│       └── cmark-gfm (Markdown → HTML)
├── SearchBar (find & replace bar)
├── SshSession (SSH/SFTP connection)
└── AppSettings / EditorTheme
```

## Source Files

### Core

| File | Description |
|---|---|
| `main.cpp` | Application entry point, initializes QtWebEngine and opens files from command line arguments |
| `MainWindow.h/cpp` | Main application window (frameless) with custom title bar, tab management, multiple preview tabs, status bar, settings, SSH integration, session restore, recent files, auto-save, drag & drop, WebEngine warmup, theme system via `setStyleSheet()` |
| `TitleBar.h/cpp` | Custom title bar widget with hamburger menu (☰), QTabBar with close buttons, window controls (minimize, maximize, close), mouse drag to move, double-click to maximize, theme-aware styling |
| `Editor.h/cpp` | `QPlainTextEdit` subclass with `SpacedDocumentLayout` for line spacing, line number gutter, current line highlighting, font zoom, theme support, search match highlighting, auto-indent, bracket matching |

### Search & Replace

| File | Description |
|---|---|
| `SearchBar.h/cpp` | Find/Replace bar widget with match navigation, match count display, case-sensitive toggle, replace and replace all |

### Minimap

| File | Description |
|---|---|
| `Minimap.h/cpp` | VS Code-style minimap widget. Renders each line as a 2px high colored strip proportional to text content. Shows visible area overlay. Debounced rebuild (150ms) for performance. |

### Syntax Highlighting

| File | Description |
|---|---|
| `MarkdownHighlighter.h/cpp` | `QSyntaxHighlighter` for Markdown syntax with `setDarkTheme()` support; rebuilds color rules for dark/light themes (headings, bold, italic, code, links, LaTeX, fenced blocks) |
| `CodeHighlighter.h/cpp` | `QSyntaxHighlighter` for 28 programming languages with theme-aware colors via `applyThemeColors()` |

### Markdown Preview

| File | Description |
|---|---|
| `MarkdownPreview.h/cpp` | `QWidget` containing a `QWebEngineView` with internal debounce timer (400ms). Uses **cmark-gfm** library for spec-compliant Markdown → HTML conversion with GFM extensions (table, strikethrough, autolink, tagfilter). Math expressions are injected into the cmark-gfm AST as HTML inline nodes. Supports dark/light mode with separate highlight.js CSS, zoom, and advanced PDF export with page numbering, margins, orientation, and page borders via QPdfDocument/QPdfWriter post-processing. |

### File I/O

| File | Description |
|---|---|
| `LargeFileReader.h/cpp` | Chunked file reader with BOM-based encoding detection, progress callback, supports files of any size |

### SSH Remote

| File | Description |
|---|---|
| `SshSession.h/cpp` | Wrapper around libssh2 for SSH connection, authentication (password/key), and SFTP operations (list, read, write, mkdir, rmdir, unlink, rename). Includes 512 MB remote file size limit. |
| `SshConnectDialog.h/cpp` | Frameless connection dialog with custom title bar, host/port/username fields, password/key authentication options, saved connection profiles (persisted via QSettings, passwords are never stored) |
| `RemoteFileBrowser.h/cpp` | Remote filesystem browser dialog with tree view, navigation, file selection, and file management (create, rename, delete files and directories) |

### Settings and Themes

| File | Description |
|---|---|
| `SettingsDialog.h/cpp` | Frameless settings dialog with Editor and Appearance tabs. Defines `AppSettings` struct with all configurable options including line spacing, vertical ruler, and its column position. |
| `EditorTheme.h/cpp` | Loads Zed editor themes from `~/.config/ed/themes/` JSON files. Parses theme colors for editor, UI chrome, and syntax highlighting. |

## Key Design Decisions

### Tab-based multi-document model

Each editor tab contains a QWidget container with an `Editor` and optional `Minimap` in an HBoxLayout. A `TabData` struct stores per-tab state (file path, encoding, language, modified flag, remote file info, preview flag). The `MainWindow` maintains a `QVector<TabData>` parallel to the `QTabWidget` indices. Preview tabs have `isPreview = true` and are skipped by save/session/autosave logic.

### Custom title bar

The application uses a frameless window (`Qt::FramelessWindowHint`) with a custom `TitleBar` widget that combines the hamburger menu (☰), document tabs (`QTabBar` with close buttons), and window controls (minimize, maximize, close) into a single 32px-high bar — similar to VS Code and Zed. The title bar supports mouse drag to move the window and double-click to maximize/restore. The `QTabWidget`'s built-in tab bar is hidden; tab state is synced between the title bar's `QTabBar` and the `QTabWidget` via signals. The traditional menu bar is hidden via `menuBar()->hide()`, and all menus are organized as submenus inside the hamburger button with `InstantPopup` mode.

### Line spacing

Line spacing is implemented via a custom `SpacedDocumentLayout` class that extends `QPlainTextDocumentLayout`. The `blockBoundingRect()` override multiplies block heights by a configurable spacing factor (1.0 - 2.0). The layout is set on the editor's document in the `Editor` constructor.

### Theme system

Themes are loaded from `~/.config/ed/themes/` in Zed editor JSON format via `EditorTheme::loadFromDirectory()`. Themes are reloaded when opening the Settings dialog (no file watcher — avoids issues with atomic saves on Linux).

The theme is applied via `MainWindow::applyThemeToApp()` which sets a stylesheet on the MainWindow (not `qApp` — using `qApp->setStyleSheet()` causes Qt to ignore `setPalette()` on all widgets). The stylesheet covers `QPlainTextEdit`, `QStatusBar`, `QLabel`, `QScrollBar`, and dialog elements. After the global palette is set, individual editors receive `applyTheme()` calls to set their specific palette and update syntax highlighter colors. A cascade `update()` on all child widgets ensures full repaint.

### Markdown preview with cmark-gfm

The preview uses GitHub's cmark-gfm library (fetched via CMake FetchContent, built statically) for spec-compliant Markdown parsing with GFM extensions. LaTeX math expressions are detected in text nodes of the AST and replaced with HTML inline nodes containing KaTeX spans, avoiding regex-based post-processing. The rendered HTML includes theme-aware CSS, KaTeX for math, Mermaid.js for diagrams, and highlight.js with dark/light CSS variants.

### PDF export with post-processing

PDF export supports configurable margins, page numbering, orientation, and page borders. When post-processing is needed (page numbers or borders), the PDF is first rendered to a byte array, then loaded via `QPdfDocument`, and re-rendered through `QPdfWriter` with `QPainter` to add decorations. Print CSS is injected before export and removed after.

### Modification tracking

File modification is tracked by comparing the current editor content against a saved baseline, stored as a Qt dynamic property `savedContent` on each `Editor` instance. When a file is opened or saved, `editor->setProperty("savedContent", editor->toPlainText())` captures the baseline. On every `contentsChanged` signal, the current text is compared against the baseline — if they match, the tab is marked clean (no asterisk). This approach correctly handles undo/redo: undoing all changes back to the original state clears the modified flag. Programmatic changes (theme application, highlighter setup, settings changes) are guarded by an `m_ignoreTextChanged` flag to prevent false modification detection.

### Markdown preview as separate tabs

Each markdown file can have its own preview tab, opened right next to the source tab. Multiple previews can be open simultaneously. Each `MarkdownPreview` instance stores a `sourceEditor` property pointing to the source editor widget. Live updates are achieved via a direct `connect(editor, textChanged, preview, ...)` signal — each preview has its own internal 400ms debounce timer. The preview renders to a temp HTML file and loads it via `QWebEngineView::load()` to avoid JavaScript string escaping issues. Dark/light mode is auto-detected from the editor theme.

### WebEngine warmup

On startup, a hidden 0x0 `QWebEngineView` loads `about:blank` and is deleted on completion. This pre-initializes the Chromium subprocess so that the first Markdown preview opens without a visible flicker.

### Context-aware zoom

Ctrl+Plus/Minus and Ctrl+scroll are context-aware: when a preview tab is active, they zoom the preview (`QWebEngineView::setZoomFactor`); otherwise they zoom the editor font.

### Settings persistence

All `AppSettings` values (fonts, theme, editor behavior, ruler, minimap, auto-save, line spacing) are persisted via `QSettings` in the `settings/` group. `loadSettings()` runs at startup before any UI is created. `saveSettings()` runs after the Settings dialog is accepted and on application close. This ensures the full editor configuration survives restarts.

### Session persistence

On application close, the list of open file paths and the active tab index are saved via `QSettings`. On next startup, `restoreSession()` re-opens those files automatically. Recent files (last 5) are also persisted and accessible from File > Recent Files.

### Search and replace

The search system uses `QTextEdit::ExtraSelection` to highlight all matches in yellow with the current match in orange. A `ScrollBarOverlay` widget is positioned on top of the vertical scrollbar to draw orange markers at match positions (hidden when scrollbar is not visible). Search results are capped at 10,000 matches to prevent UI lag on large files.

### Minimap

The minimap is a custom QWidget (not a QPlainTextEdit) that renders each line of code as a 2px high colored strip. Text content length and indentation are mapped to pixel positions, creating a dense bird's-eye view of the file structure. A semi-transparent blue overlay shows the currently visible area. The pixmap is rebuilt with a 150ms debounce timer. Clicking or dragging navigates the editor. Configurable in Settings.

### Auto-indent and bracket matching

Auto-indent preserves the leading whitespace of the current line when pressing Enter. Bracket matching highlights paired `()`, `{}`, `[]` characters in green when the cursor is adjacent. Both features are integrated into the `Editor` class and configurable in Settings.

### Auto-save

A `QTimer` periodically saves all modified tabs that have a file path (skipping untitled, remote, and preview tabs). The interval is configurable in Settings (5-600 seconds). The timer is stopped when auto-save is disabled.

### Drag & Drop

The main window accepts file drops via `dragEnterEvent`/`dropEvent`. Dropped files with `file://` URLs are opened as new tabs.

### Offline-first architecture

All third-party rendering libraries (KaTeX, Mermaid.js, highlight.js) are embedded in the Qt resource system (`resources.qrc`). At runtime, `MarkdownPreview::deployResources()` extracts them to a shared temp directory (`/tmp/text-ed-preview`) with integrity checking (re-extracts only if on-disk copy differs from bundled resource). KaTeX CSS font paths are rewritten to absolute `file://` URLs. `LocalContentCanAccessRemoteUrls` is set to `false` to block any network requests.

### Frameless dialogs

All dialogs (Settings, SSH Connect) use `Qt::FramelessWindowHint` with custom title bars matching the main window style. Title bars use `#dialogTitleBar` and `#dialogCloseBtn` object names for consistent stylesheet theming. Dragging is implemented via `eventFilter` + `QWindow::startSystemMove()`.

### Saved SSH connections

`SshConnectDialog` persists connection profiles (name, host, port, username, auth type, key path) via `QSettings` in a `sshConnections` array. Passwords are never stored. The dialog shows a saved connection list on the left; selecting an entry populates the form fields.

### Remote file management

`RemoteFileBrowser` provides toolbar buttons and a right-click context menu for creating files, creating directories, renaming, and deleting items. These operations use `SshSession::mkdir`, `rmdir`, `unlink`, and `rename` which wrap the corresponding libssh2 SFTP calls.

### Large file support

`LargeFileReader` reads files in 4 MB chunks with a progress callback. It auto-detects encoding from BOM (UTF-8, UTF-16 BE/LE, UTF-32 BE/LE) and falls back to UTF-8. The chunked approach prevents memory spikes and allows the UI to show a progress dialog.

### SSH remote editing

`SshSession` wraps libssh2 with a high-level API for connecting, listing directories, reading/writing files, and file management (mkdir, rmdir, unlink, rename) via SFTP. Remote files are loaded into regular tabs with an `isRemote` flag. Saving (Ctrl+S) detects remote files and writes back via SFTP. Remote file reads are limited to 512 MB to prevent out-of-memory crashes.

## Build System

CMake with `AUTOMOC` and `AUTORCC` enabled. cmark-gfm is fetched via `FetchContent` and built as a static library.

Dependencies:
- `Qt6::Widgets`, `Qt6::Core`, `Qt6::Gui`, `Qt6::WebEngineWidgets`, `Qt6::WebEngineQuick`, `Qt6::PdfWidgets`
- `libssh2`
- `libcmark-gfm_static`, `libcmark-gfm-extensions_static`

## Resource Bundle

`resources.qrc` includes:
- `resources/katex/katex.min.js`, `katex.min.css`, `fonts/*.woff2` (20+ font files)
- `resources/mermaid/mermaid.min.js`
- `resources/hljs/highlight.min.js`, `github.min.css`, `hljs-dark.min.css`, `hljs-light.min.css`
- `resources/icon.svg`
