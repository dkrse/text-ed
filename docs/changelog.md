# Changelog

All notable changes to TextEd are documented in this file.

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
