# Changelog

All notable changes to TextEd are documented in this file.

## [0.4.0] - 2026-03-11

### Added

- **Saved SSH connections** - save, load, and delete SSH connection profiles (host, port, username, auth type, key path); passwords are never stored
- **Remote file management** - create, rename, and delete files and directories in the remote file browser (toolbar buttons + right-click context menu)
- **Vertical ruler** - configurable vertical line at a specified column (default 80), toggle and position in Settings
- **Settings persistence** - all editor and appearance settings are now saved via QSettings and restored on next launch

### Fixed

- **Dark theme not fully applied** - menu bar, tab bar, status bar, and dialogs now switch to a dark palette when a dark editor theme is selected
- **Text invisible after restart with dark theme** - syntax highlighter colors were not applied on startup; theme colors are now applied whenever a highlighter is created or changed

## [0.3.0] - 2026-03-10

### Added

- **Go to Line** (Ctrl+G) - jump to a specific line number via input dialog
- **Auto-indent** - new lines preserve leading whitespace of the previous line, configurable in Settings
- **Bracket matching** - highlights paired `()`, `{}`, `[]` in green when cursor is adjacent, configurable in Settings
- **Drag & Drop** - open files by dragging them into the editor window
- **Auto-save** - automatically saves modified files at a configurable interval (5-600 seconds), configurable in Settings
- **Minimap** - VS Code-style code overview on the right side, renders each line as a 2px colored strip with visible area overlay, configurable in Settings
- New settings: Auto Indent, Bracket Matching, Auto Save (with interval), Show Minimap

## [0.2.0] - 2026-03-10

### Added

- **Find & Replace** (Ctrl+F / Ctrl+H) with:
  - Yellow highlighting of all search matches, orange for current match
  - Match count display ("3 of 15")
  - Navigation with F3 (next) and Shift+F3 (previous)
  - Case-sensitive toggle
  - Replace and Replace All functionality
  - Orange markers on the vertical scrollbar showing match positions
- **Assembly language syntax highlighting** for `.asm`, `.s`, `.S`, `.nasm`, `.yasm` files with x86 instructions, registers, directives, labels, NASM preprocessor
- **Dedicated Makefile syntax highlighting** with variables `$(VAR)`/`${VAR}`, automatic variables (`$@`, `$<`, `$^`...), targets, special targets (.PHONY etc.)
- **Session restore** - open files are remembered and automatically re-opened on next launch
- **Recent files** menu (File > Recent Files) with last 5 opened files, persisted across sessions
- **Unsaved changes dialog** when closing the application window (Save / Discard / Cancel for each modified tab)

### Fixed

- Fixed file marked as modified immediately after opening (caused by `setPlainText()` triggering `textChanged` signal before `modified` flag was reset)
- Fixed orange search markers appearing as horizontal lines extending into text area when vertical scrollbar is not visible (short files)

### Improved

- Increased SSH directory listing buffer from 512 to 4096 bytes (prevents issues with long filenames)
- Added 512 MB file size limit for remote file reads (prevents out-of-memory crashes)
- Added 10,000 result cap for search highlighting (prevents UI lag on large files)
- Language count increased from 27 to 28 (added Assembly)

## [0.1.0] - 2026-03-09

### Added

- **Core editor** based on `QPlainTextEdit` with line number gutter and current line highlighting
- **Tab-based multi-document interface** with per-tab state (file path, encoding, language, modified flag)
- **Large file support** with chunked reading (4 MB chunks), progress dialog, and BOM-based encoding auto-detection
- **Character encoding selection** - UTF-8, UTF-16 BE/LE, UTF-32 BE/LE, Latin-1, System encoding
- **Font zoom** with Ctrl+Plus / Ctrl+Minus, separate GUI and editor font configuration
- **Markdown syntax highlighting** in the editor (headings, bold, italic, code, links, LaTeX, fenced code blocks, mermaid blocks)
- **Markdown live preview** via QWebEngineView with:
  - LaTeX formula rendering (KaTeX, bundled offline)
  - Mermaid diagram rendering (bundled offline)
  - Code block syntax highlighting (highlight.js, bundled offline)
  - Inline formatting (bold, italic, code, links, images) in all block contexts
- **PDF export** of the Markdown preview (View > Export Preview to PDF)
- **Syntax highlighting for 27 languages**: C/C++, C#, Python, JavaScript, TypeScript, Java, Kotlin, Rust, Go, Shell, SQL, HTML, XML, CSS, Ruby, PHP, Lua, Perl, Haskell, Swift, JSON, YAML, TOML, INI, CMake, Makefile
- **Automatic language detection** based on file extension
- **Settings dialog** with Editor and Appearance tabs:
  - Editor font and size
  - Syntax highlighting toggle
  - Line numbers, word wrap, current line highlight, whitespace display
  - Tab width
  - GUI font and size
  - Color theme selection
- **12 built-in color themes**: Default Light, Solarized Light, Monokai, Dracula, One Dark, Nord, Gruvbox Dark, Gruvbox Light, Tomorrow Night, GitHub Dark, Catppuccin Mocha, Catppuccin Latte
- **SSH remote editing** via libssh2/SFTP:
  - Connect dialog with password and private key authentication
  - Remote file browser with directory navigation and file permissions display
  - Open and save remote files, seamless Ctrl+S saving back to server
  - SSH connection status indicator in status bar
- **Fully offline architecture** - all rendering resources bundled in Qt resource system
