# TextEd

A fast, feature-rich text editor built with Qt 6 and C++17. Designed for efficient editing of text files, with strong Markdown support, syntax highlighting for 28 programming languages, SSH remote editing, and a fully offline architecture.

## Features

- **Fast file handling** - chunked reading with progress indicator for large files (4 MB chunks)
- **Custom title bar** - VS Code/Zed-style frameless window with integrated hamburger menu (☰), tabs with close buttons, and window controls (minimize, maximize, close) in a single compact bar
- **Tab-based interface** - multi-document editing with per-tab state and closable tabs
- **Session restore** - remembers open files and restores them on next launch
- **Recent files** - quick access to last 5 opened files (File > Recent Files)
- **Find & Replace** - Ctrl+F / Ctrl+H with yellow match highlighting, scrollbar markers, match count, case-sensitive toggle
- **Go to Line** - Ctrl+G to jump to a specific line number
- **Undo / Redo** - full undo/redo support (Ctrl+Z / Ctrl+Shift+Z); undo back to saved state clears the modified flag
- **Auto-indent** - preserves indentation when pressing Enter
- **Bracket matching** - highlights paired `()`, `{}`, `[]` when cursor is adjacent
- **Line spacing** - configurable line spacing (1.0, 1.2, 1.5, 1.8, 2.0) via custom document layout
- **Minimap** - VS Code-style code overview on the right side
- **Auto-save** - configurable automatic saving at regular intervals
- **Drag & Drop** - open files by dragging them into the window
- **Markdown preview** - live preview in a separate tab with cmark-gfm (GitHub Flavored Markdown) parser; supports tables, strikethrough, autolinks; debounced updates (400ms); multiple previews can be open simultaneously; toggle via preview button on markdown tabs or Ctrl+Shift+P; dark/light mode follows editor theme
- **LaTeX math rendering** - inline `$...$` and display `$$...$$` formulas via bundled KaTeX, with math injection into cmark-gfm AST
- **Mermaid diagrams** - rendered in the preview via bundled Mermaid.js with theme-aware rendering (dark/default)
- **Code block highlighting** - syntax-highlighted code blocks in preview via bundled highlight.js with dark/light CSS variants
- **PDF export** - export the Markdown preview to PDF with configurable margins, page numbering (none/page/page+total), orientation (portrait/landscape), and page borders
- **Syntax highlighting** - 28 languages including C/C++, Python, JavaScript, Rust, Go, Java, Assembly, and more
- **Character encoding** - UTF-8, UTF-16 BE/LE, UTF-32 BE/LE, Latin-1, System; auto-detection via BOM
- **Zed editor themes** - loads color themes from `~/.config/ed/themes/` in Zed theme JSON format; themes are reloaded when opening Settings
- **SSH remote editing** - connect to remote servers via SSH/SFTP with frameless connection dialog, browse and edit files, create/rename/delete remote files and directories, save connection profiles (passwords are never stored)
- **Fully offline** - all rendering resources (KaTeX, Mermaid, highlight.js) are bundled, no internet required
- **Font zoom** - Ctrl+Plus / Ctrl+Minus (context-aware: zooms preview when preview tab is active, editor otherwise)
- **Separate fonts** - independent font selection for GUI and editor text
- **Vertical ruler** - configurable vertical line at a specified column (default 80), useful for code style guides
- **Configurable** - line numbers, word wrap, current line highlight, whitespace display, tab width, auto-indent, bracket matching, minimap, auto-save, vertical ruler, line spacing; all settings persist across sessions

## Requirements

- Qt 6 (Widgets, Core, Gui, WebEngineWidgets, WebEngineQuick, PdfWidgets)
- libssh2
- cmark-gfm (fetched automatically via CMake FetchContent)
- CMake 3.16+
- C++17 compiler

See [docs/dependencies.md](docs/dependencies.md) for detailed dependency information.

### Installing dependencies (Debian/Ubuntu)

```bash
sudo apt install qt6-base-dev qt6-webengine-dev qt6-pdf-dev libssh2-1-dev cmake build-essential
```

### Installing dependencies (Fedora)

```bash
sudo dnf install qt6-qtbase-devel qt6-qtwebengine-devel qt6-qtpdf-devel libssh2-devel cmake gcc-c++
```

## Building

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

The resulting binary is `build/text-ed`.

## Desktop Integration

A `.desktop` file can be placed in `~/.local/share/applications/` for launcher integration. An SVG icon is included at `resources/icon.svg`.

## Usage

```bash
./text-ed                    # open with empty tab (or restores previous session)
./text-ed file.txt           # open a file
./text-ed file1.md file2.py  # open multiple files
```

### Keyboard shortcuts

| Shortcut | Action |
|---|---|
| Ctrl+N | New file |
| Ctrl+O | Open file |
| Ctrl+S | Save |
| Ctrl+Shift+S | Save As |
| Ctrl+W | Close tab |
| Ctrl+Z | Undo |
| Ctrl+Shift+Z | Redo |
| Ctrl+F | Find |
| Ctrl+H | Find and Replace |
| Ctrl+G | Go to Line |
| F3 | Find next |
| Shift+F3 | Find previous |
| Ctrl+Shift+P | Toggle Markdown preview |
| Ctrl+Plus | Zoom in (editor or preview) |
| Ctrl+Minus | Zoom out (editor or preview) |
| Ctrl+Scroll | Zoom in/out (in preview) |
| Ctrl+, | Preferences |
| Ctrl+Q | Quit |

## Documentation

- [Architecture](docs/architecture.md) - component design, source files, key design decisions
- [Diagrams](docs/diagrams.md) - Mermaid diagrams of architecture, flows, and features
- [Changelog](docs/changelog.md) - version history
- [Dependencies](docs/dependencies.md) - build and runtime dependencies

## License

MIT License. See [LICENSE](LICENSE) for details.

## Author

**krse**
