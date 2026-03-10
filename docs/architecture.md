# Architecture

## Overview

TextEd is a Qt 6 Widgets application written in C++17. It follows a modular design where each major feature is encapsulated in its own class. The application uses `QPlainTextEdit` as the core editor widget, `QWebEngineView` for Markdown preview rendering, and `libssh2` for SSH/SFTP remote file access.

## Component Diagram

```
MainWindow
├── QTabWidget (multi-document tabs)
│   └── QWidget container (per tab)
│       ├── Editor
│       │   ├── LineNumberArea
│       │   ├── ScrollBarOverlay (search match markers)
│       │   ├── MarkdownHighlighter (for .md files)
│       │   └── CodeHighlighter (for code files)
│       └── Minimap (VS Code-style code overview)
├── QSplitter
│   └── MarkdownPreview (QWebEngineView)
├── SearchBar (find & replace bar)
├── SshSession (SSH/SFTP connection)
└── AppSettings / EditorTheme
```

## Source Files

### Core

| File | Description |
|---|---|
| `main.cpp` | Application entry point, initializes QtWebEngine and opens files from command line arguments |
| `MainWindow.h/cpp` | Main application window with tab management, menus, status bar, settings, SSH integration, session restore, recent files, auto-save, drag & drop |
| `Editor.h/cpp` | `QPlainTextEdit` subclass with line number gutter, current line highlighting, font zoom, theme support, search match highlighting, auto-indent, bracket matching |

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
| `MarkdownHighlighter.h/cpp` | `QSyntaxHighlighter` for Markdown syntax (headings, bold, italic, code, links, LaTeX, fenced blocks) |
| `CodeHighlighter.h/cpp` | `QSyntaxHighlighter` for 28 programming languages with theme-aware colors |

### Markdown Preview

| File | Description |
|---|---|
| `MarkdownPreview.h/cpp` | `QWebEngineView` subclass that renders Markdown to HTML with KaTeX, Mermaid, and highlight.js. Supports PDF export. |

### File I/O

| File | Description |
|---|---|
| `LargeFileReader.h/cpp` | Chunked file reader with BOM-based encoding detection, progress callback, supports files of any size |

### SSH Remote

| File | Description |
|---|---|
| `SshSession.h/cpp` | Wrapper around libssh2 for SSH connection, authentication (password/key), and SFTP operations (list, read, write). Includes 512 MB remote file size limit. |
| `SshConnectDialog.h/cpp` | Connection dialog with host, port, username, password/key authentication options |
| `RemoteFileBrowser.h/cpp` | Remote filesystem browser dialog with tree view, navigation, file selection |

### Settings and Themes

| File | Description |
|---|---|
| `SettingsDialog.h/cpp` | Settings dialog with Editor and Appearance tabs. Defines `AppSettings` struct with all configurable options. |
| `EditorTheme.h/cpp` | Defines `EditorTheme` struct with 12 built-in color themes |

## Key Design Decisions

### Tab-based multi-document model

Each tab contains a QWidget container with an `Editor` and optional `Minimap` in an HBoxLayout. A `TabData` struct stores per-tab state (file path, encoding, language, modified flag, preview visibility, remote file info). The `MainWindow` maintains a `QVector<TabData>` parallel to the `QTabWidget` indices. Editors are retrieved from tab containers via `findChild<Editor*>()`.

### Session persistence

On application close, the list of open file paths and the active tab index are saved via `QSettings`. On next startup, `restoreSession()` re-opens those files automatically. Recent files (last 5) are also persisted and accessible from File > Recent Files.

### Search and replace

The search system uses `QTextEdit::ExtraSelection` to highlight all matches in yellow with the current match in orange. A `ScrollBarOverlay` widget is positioned on top of the vertical scrollbar to draw orange markers at match positions (hidden when scrollbar is not visible). Search results are capped at 10,000 matches to prevent UI lag on large files.

### Minimap

The minimap is a custom QWidget (not a QPlainTextEdit) that renders each line of code as a 2px high colored strip. Text content length and indentation are mapped to pixel positions, creating a dense bird's-eye view of the file structure. A semi-transparent blue overlay shows the currently visible area. The pixmap is rebuilt with a 150ms debounce timer. Clicking or dragging navigates the editor. Configurable in Settings.

### Auto-indent and bracket matching

Auto-indent preserves the leading whitespace of the current line when pressing Enter. Bracket matching highlights paired `()`, `{}`, `[]` characters in green when the cursor is adjacent. Both features are integrated into the `Editor` class and configurable in Settings.

### Auto-save

A `QTimer` periodically saves all modified tabs that have a file path (skipping untitled and remote files). The interval is configurable in Settings (5-600 seconds). The timer is stopped when auto-save is disabled.

### Drag & Drop

The main window accepts file drops via `dragEnterEvent`/`dropEvent`. Dropped files with `file://` URLs are opened as new tabs.

### Offline-first architecture

All third-party rendering libraries (KaTeX, Mermaid.js, highlight.js) are embedded in the Qt resource system (`resources.qrc`). At runtime, `MarkdownPreview::deployResources()` extracts them to a `QTemporaryDir` so that `QWebEngineView` can load them via `file://` URLs. `LocalContentCanAccessRemoteUrls` is set to `false` to block any network requests. This ensures the application works without any network access.

### Markdown preview rendering

The preview uses a two-stage pipeline:
1. `markdownToHtml()` - converts Markdown to HTML with inline processing (`processInline()` handles bold, italic, code, links, images, LaTeX placeholders)
2. `buildHtml()` - wraps the body in a full HTML document with CSS, KaTeX, Mermaid, and highlight.js scripts

The preview is updated via a debounced `QTimer` (300ms) to avoid excessive re-rendering during typing.

### Theme system

`EditorTheme` defines colors for: background, foreground, current line, line numbers, selection, and 8 syntax categories (keyword, type, string, comment, number, function, preprocessor, operator). Themes are applied to the `Editor` palette, line number area, minimap background, and forwarded to `CodeHighlighter` which rebuilds its format rules.

### Large file support

`LargeFileReader` reads files in 4 MB chunks with a progress callback. It auto-detects encoding from BOM (UTF-8, UTF-16 BE/LE, UTF-32 BE/LE) and falls back to UTF-8. The chunked approach prevents memory spikes and allows the UI to show a progress dialog.

### SSH remote editing

`SshSession` wraps libssh2 with a high-level API for connecting, listing directories, and reading/writing files via SFTP. Remote files are loaded into regular tabs with an `isRemote` flag. Saving (Ctrl+S) detects remote files and writes back via SFTP. Remote file reads are limited to 512 MB to prevent out-of-memory crashes.

## Build System

CMake with `AUTOMOC` and `AUTORCC` enabled. Dependencies:
- `Qt6::Widgets`, `Qt6::Core`, `Qt6::Gui`, `Qt6::WebEngineWidgets`, `Qt6::WebEngineQuick`
- `libssh2`

## Resource Bundle

`resources.qrc` includes:
- `resources/katex/katex.min.js`, `katex.min.css`, `fonts/*.woff2` (20 font files)
- `resources/mermaid/mermaid.min.js`
- `resources/hljs/highlight.min.js`, `github.min.css`
