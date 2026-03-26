# Diagrams

Visual diagrams for TextEd architecture, features, and usage flows.

## Application Architecture

```mermaid
graph TB
    subgraph MainWindow
        TB[TitleBar]
        TW[QTabWidget]
        SB[SearchBar]
        STB[StatusBar]
    end

    subgraph TitleBar
        HM[Hamburger Menu ☰]
        TBR[QTabBar]
        WC[Window Controls]
    end

    subgraph "Editor Tab (per file)"
        ED[Editor]
        MM[Minimap]
    end

    subgraph Editor Internals
        LNA[LineNumberArea]
        SBO[ScrollBarOverlay]
        MH[MarkdownHighlighter]
        CH[CodeHighlighter]
    end

    subgraph "Preview Tab (per .md file)"
        MP["MarkdownPreview (QWidget)"]
        WV[QWebEngineView]
        KT[KaTeX]
        MJ[Mermaid.js]
        HJS[highlight.js]
    end

    subgraph Remote
        SSH[SshSession / libssh2]
        SCD[SshConnectDialog]
        RFB[RemoteFileBrowser]
    end

    subgraph Settings
        SD[SettingsDialog]
        AS[AppSettings]
        ET[EditorTheme]
    end

    subgraph File I/O
        LFR[LargeFileReader]
    end

    TW --> ED
    TW --> MM
    TW --> MP
    MP --> WV
    WV --> KT
    WV --> MJ
    WV --> HJS
    ED --> LNA
    ED --> SBO
    ED --> MH
    ED --> CH
    MainWindow --> SSH
    SSH --> SCD
    SSH --> RFB
    MainWindow --> SD
    SD --> AS
    SD --> ET
    MainWindow --> LFR
```

## Class Diagram

```mermaid
classDiagram
    QMainWindow <|-- MainWindow
    QWidget <|-- TitleBar
    QPlainTextEdit <|-- Editor
    QWidget <|-- MarkdownPreview
    QSyntaxHighlighter <|-- CodeHighlighter
    QSyntaxHighlighter <|-- MarkdownHighlighter
    QWidget <|-- Minimap
    QWidget <|-- SearchBar
    QWidget <|-- ScrollBarOverlay
    QDialog <|-- SettingsDialog
    QDialog <|-- SshConnectDialog
    QDialog <|-- RemoteFileBrowser

    MainWindow --> TitleBar : custom title bar
    MainWindow --> Editor : manages tabs
    MainWindow --> Minimap : per editor
    MainWindow --> MarkdownPreview : per md file
    MainWindow --> SearchBar : find/replace
    MainWindow --> SshSession : remote editing
    MainWindow --> AppSettings : configuration
    MainWindow --> TabData : per-tab state

    Editor --> CodeHighlighter : code files
    Editor --> MarkdownHighlighter : .md files
    Editor --> LineNumberArea : gutter
    Editor --> ScrollBarOverlay : search markers

    MarkdownPreview --> QWebEngineView : contains

    class TitleBar {
        -QTabBar* m_tabBar
        -QToolButton* m_hamburgerButton
        -QToolButton* m_minButton
        -QToolButton* m_maxButton
        -QToolButton* m_closeButton
        -QLabel* m_titleLabel
        +applyTheme(bg, fg, activeBg, activeFg, hoverBg)
    }

    class MainWindow {
        -TitleBar* m_titleBar
        -QTabWidget* m_tabWidget
        -QVector~TabData~ m_tabs
        -QList~MarkdownPreview*~ m_mdPreviews
        -AppSettings m_settings
        -SshSession* m_sshSession
        +openFile(path)
        +newFile()
        +save()
        +openPreviewTab(sourceIndex)
        +closePreviewTab(previewIndex)
    }

    class Editor {
        -bool m_autoIndent
        -bool m_bracketMatching
        +setLanguage(lang)
        +setMarkdownMode(on)
        +applyTheme(theme)
        +applySettings(settings)
        +highlightSearchMatches(text, caseSensitive)
    }

    class MarkdownPreview {
        -QWebEngineView* m_webView
        -QTimer* m_debounce
        -bool m_dark
        +updatePreview(markdown)
        +setDarkMode(dark)
        +zoomIn()
        +zoomOut()
        +exportToPdf(path)
    }

    class TabData {
        +QString filePath
        +bool modified
        +bool isMarkdown
        +bool isRemote
        +bool isPreview
        +QString remotePath
        +Encoding encoding
        +Language language
    }

    class AppSettings {
        +QFont editorFont
        +QFont guiFont
        +bool syntaxHighlighting
        +bool lineNumbers
        +bool lineWrap
        +bool autoIndent
        +bool bracketMatching
        +bool autoSave
        +bool minimap
        +bool showRuler
        +QString themeName
    }

    class EditorTheme {
        +QColor background
        +QColor foreground
        +QColor keyword
        +QColor string
        +QColor comment
        +themeByName(name)$ EditorTheme
        +themeNames()$ QStringList
    }
```

## File Open Flow

```mermaid
sequenceDiagram
    actor User
    participant MW as MainWindow
    participant LFR as LargeFileReader
    participant ED as Editor
    participant CH as CodeHighlighter

    User->>MW: Open file (Ctrl+O / drag & drop)
    MW->>MW: Check if file already open
    alt Already open
        MW->>MW: Switch to existing tab
    else New file
        MW->>MW: createTab()
        MW->>LFR: detectEncoding(path)
        LFR-->>MW: encoding
        MW->>LFR: readAll(path, encoding)
        Note over LFR: 4 MB chunks with progress
        LFR-->>MW: content
        MW->>ED: setPlainText(content)
        MW->>CH: setLanguage(detected)
        MW->>ED: applyTheme(theme)
        MW->>ED: setProperty("savedContent", text)
        Note over MW: Add preview button if .md
    end
```

## Save Flow

```mermaid
sequenceDiagram
    actor User
    participant MW as MainWindow
    participant ED as Editor
    participant SSH as SshSession

    User->>MW: Save (Ctrl+S)
    MW->>MW: Get current TabData
    alt Remote file
        MW->>SSH: writeFile(remotePath, data)
        SSH-->>MW: ok/error
    else Local file
        alt Has file path
            MW->>MW: saveFileFromTab(index, path)
        else No file path
            MW->>User: Save As dialog
            User-->>MW: chosen path
            MW->>MW: saveFileFromTab(index, path)
        end
    end
    MW->>ED: setProperty("savedContent", text)
    MW->>MW: updateTabTitle() (remove *)
```

## Markdown Preview Flow

```mermaid
sequenceDiagram
    actor User
    participant MW as MainWindow
    participant ED as Editor
    participant MP as MarkdownPreview
    participant WV as QWebEngineView

    User->>MW: Click preview button on .md tab
    MW->>MP: new MarkdownPreview()
    MW->>MP: setDarkMode(isDark)
    MW->>MW: connect(editor.textChanged → preview)
    MW->>MW: Insert preview tab after source
    MP->>MP: updatePreview(text)
    MP->>MP: debounce 400ms
    MP->>MP: render()
    Note over MP: markdownToHtml() with math protection
    Note over MP: buildHtml() with theme-aware CSS
    MP->>MP: Write HTML to temp file
    MP->>WV: load(file:///tmp/.../preview.html)

    User->>ED: Edit text
    ED-->>MP: textChanged signal
    MP->>MP: updatePreview(text)
    MP->>MP: debounce 400ms...
```

## Modification Detection

```mermaid
flowchart TD
    A[File loaded / saved] --> B["savedContent = editor.toPlainText()"]
    C[User edits text] --> D[contentsChanged signal]
    D --> E{ignoreTextChanged?}
    E -->|Yes| F[Skip - programmatic change]
    E -->|No| G["Compare: toPlainText() vs savedContent"]
    G --> H{Content matches?}
    H -->|Yes| I["modified = false (remove *)"]
    H -->|No| J["modified = true (show *)"]
    K[User presses Undo] --> D
    L[Theme / highlighter change] --> M["ignoreTextChanged = true"]
    M --> N[Apply changes]
    N --> O["ignoreTextChanged = false"]
```

## Search System

```mermaid
flowchart LR
    subgraph SearchBar
        SI[Search Input]
        RI[Replace Input]
        CS[Case Sensitive]
        MC[Match Count]
    end

    subgraph Editor
        ES[ExtraSelections]
        SO[ScrollBarOverlay]
    end

    SI -->|searchTextChanged| ES
    SI -->|searchTextChanged| SO
    CS -->|toggle| ES
    ES -->|count| MC
    RI -->|replaceOne| Editor
    RI -->|replaceAll| Editor
```

## SSH Remote Editing Flow

```mermaid
sequenceDiagram
    actor User
    participant SCD as SshConnectDialog
    participant SSH as SshSession
    participant RFB as RemoteFileBrowser
    participant MW as MainWindow

    User->>SCD: Connect to SSH
    SCD->>SSH: connect(hostInfo)
    SSH->>SSH: libssh2 handshake + auth
    SSH-->>SCD: connected
    SCD-->>MW: Update status bar

    User->>RFB: Open Remote File
    RFB->>SSH: listDirectory(path)
    SSH-->>RFB: file listing
    User->>RFB: Select file
    RFB-->>MW: selected path

    MW->>SSH: readFile(remotePath)
    SSH-->>MW: file data
    MW->>MW: Open in tab (isRemote=true)

    User->>MW: Save (Ctrl+S)
    MW->>SSH: writeFile(remotePath, data)
    SSH-->>MW: ok
```

## Theme System

```mermaid
flowchart TB
    subgraph "12 Built-in Themes"
        L1[Default Light]
        L2[Solarized Light]
        L3[Gruvbox Light]
        L4[Catppuccin Latte]
        D1[Monokai]
        D2[Dracula]
        D3[One Dark]
        D4[Nord]
        D5[Gruvbox Dark]
        D6[Tomorrow Night]
        D7[GitHub Dark]
        D8[Catppuccin Mocha]
    end

    subgraph EditorTheme
        BG[background / foreground]
        CL[currentLine / selection]
        LN[lineNumber bg/fg]
        SY[keyword / type / string / comment / number / function / preprocessor / operator]
    end

    subgraph "Applied to"
        EP[Editor palette]
        LNA[LineNumberArea]
        MMP[Minimap]
        CHL[CodeHighlighter formats]
        MPV[MarkdownPreview dark/light]
        TBR[TitleBar + tabs + window controls]
        SBR[Scrollbars]
        CMB[Status bar combos]
        AP[QApplication palette - dark themes]
    end

    L1 & D1 --> EditorTheme
    EditorTheme --> EP & LNA & MMP & CHL & MPV & TBR & SBR & CMB & AP
```

## Application Feature Map

```mermaid
mindmap
  root((TextEd))
    Editor
      Syntax Highlighting
        28 Languages
        Markdown
      Line Numbers
      Current Line Highlight
      Bracket Matching
      Auto-indent
      Font Zoom
      Whitespace Display
      Vertical Ruler
      Minimap
    File Management
      Tab Interface
      Session Restore
      Recent Files
      Large File Support
      Drag & Drop
      Auto-save
      Encoding Detection
    Search
      Find Ctrl+F
      Replace Ctrl+H
      Go to Line Ctrl+G
      Match Highlighting
      Scrollbar Markers
      Case Sensitive
    Markdown
      Live Preview in Tab
      Multiple Previews
      Dark/Light Mode
      LaTeX / KaTeX
      Multi-line Math
      Mermaid Diagrams
      Code Highlighting
      Preview Zoom
      PDF Export
    Remote
      SSH / SFTP
      Remote File Browser
      File Management
      Saved Connections
    Appearance
      Custom Title Bar
      Hamburger Menu
      12 Color Themes
      Dark/Light Mode
      Theme-aware Scrollbars
      Separate GUI/Editor Fonts
      Configurable Settings
```
