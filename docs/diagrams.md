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
        TBR[QTabBar + Close Buttons]
        WC[Window Controls]
    end

    subgraph "Editor Tab (per file)"
        ED[Editor]
        SDL[SpacedDocumentLayout]
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
        CMK[cmark-gfm]
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
    ED --> SDL
    TW --> MM
    TW --> MP
    MP --> CMK
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
    QPlainTextDocumentLayout <|-- SpacedDocumentLayout
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

    Editor --> SpacedDocumentLayout : line spacing
    Editor --> CodeHighlighter : code files
    Editor --> MarkdownHighlighter : .md files
    Editor --> LineNumberArea : gutter
    Editor --> ScrollBarOverlay : search markers

    MarkdownPreview --> QWebEngineView : contains

    class SpacedDocumentLayout {
        -double m_factor
        +setSpacingFactor(f)
        +spacingFactor() double
        +blockBoundingRect(block) QRectF
    }

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
        -applyThemeToApp(theme)
    }

    class Editor {
        -SpacedDocumentLayout* m_spacedLayout
        -double m_lineSpacing
        -bool m_autoIndent
        -bool m_bracketMatching
        +setLanguage(lang)
        +setMarkdownMode(on)
        +setLineSpacing(factor)
        +applyTheme(theme)
        +applySettings(settings)
        +highlightSearchMatches(text, caseSensitive)
    }

    class MarkdownHighlighter {
        -bool m_dark
        +setDarkTheme(dark)
        -buildRules()
    }

    class MarkdownPreview {
        -QWebEngineView* m_webView
        -QTimer* m_debounce
        -bool m_dark
        +updatePreview(markdown)
        +setDarkMode(dark)
        +zoomIn()
        +zoomOut()
        +exportToPdf(path, marginL, marginR, pageNum, landscape, border)
        -markdownToHtml(md) QString
        -postProcessPdf(data, path, layout, pageNum, border)
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
        +double lineSpacing
        +QString themeName
    }

    class EditorTheme {
        +bool isDark
        +QColor background
        +QColor foreground
        +QColor keyword
        +QColor string
        +QColor comment
        +loadFromDirectory(path)$
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

## Markdown Preview Flow

```mermaid
sequenceDiagram
    actor User
    participant MW as MainWindow
    participant ED as Editor
    participant MP as MarkdownPreview
    participant CMK as cmark-gfm
    participant WV as QWebEngineView

    User->>MW: Click preview button on .md tab
    MW->>MP: new MarkdownPreview()
    MW->>MP: setDarkMode(isDark)
    MW->>MW: connect(editor.textChanged → preview)
    MW->>MW: Insert preview tab after source
    MP->>MP: updatePreview(text)
    MP->>MP: debounce 400ms
    MP->>CMK: parse + attach GFM extensions
    CMK-->>MP: AST
    MP->>MP: injectMathSpans(AST)
    MP->>CMK: render HTML
    CMK-->>MP: HTML string
    MP->>MP: Convert mermaid blocks
    MP->>MP: Build full HTML page with CSS/JS
    MP->>WV: load(file:///tmp/.../preview.html)
```

## PDF Export Flow

```mermaid
sequenceDiagram
    actor User
    participant MW as MainWindow
    participant DLG as PDF Options Dialog
    participant MP as MarkdownPreview
    participant WE as QWebEnginePage
    participant PDF as QPdfDocument/Writer

    User->>MW: View > Export Preview to PDF
    MW->>DLG: Show options (margins, numbering, orientation, border)
    DLG-->>MW: User confirms
    MW->>MP: exportToPdf(path, margins, numbering, landscape, border)
    MP->>WE: injectPrintCss()
    alt No post-processing needed
        MP->>WE: printToPdf(path, layout)
    else Needs page numbers or borders
        MP->>WE: printToPdf(callback, layout)
        WE-->>MP: PDF byte array
        MP->>PDF: Load via QPdfDocument
        MP->>PDF: Re-render via QPdfWriter + QPainter
        Note over PDF: Add margins, page numbers, borders
        MP->>PDF: Save to file
    end
    MP->>WE: removePrintCss()
```

## Theme Application Flow

```mermaid
flowchart TD
    A[Settings Dialog OK / Theme selected] --> B[applyThemeToApp]
    B --> C[Set dark/light mode on previews]
    B --> D[TitleBar::applyTheme - tabs, buttons]
    B --> E[Status bar combos stylesheet]
    B --> F["MainWindow::setStyleSheet()<br/>(QPlainTextEdit, QStatusBar, QScrollBar, etc.)"]
    B --> G{isDark?}
    G -->|Yes| H[QApplication::setPalette - dark palette]
    G -->|No| I[QApplication::setPalette - standard palette]
    H --> J["Editor::applyTheme (AFTER global palette)"]
    I --> J
    J --> K[setPalette on editor viewport]
    J --> L[CodeHighlighter::applyThemeColors + rehighlight]
    J --> M[MarkdownHighlighter::setDarkTheme + rehighlight]
    J --> N[LineNumberArea::update]
    B --> O["Cascade update() on all child widgets"]
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
    subgraph "Zed Theme Files (~/.config/ed/themes/)"
        TF["*.json files<br/>(Zed theme format)"]
    end

    subgraph EditorTheme
        BG[background / foreground]
        CL[currentLine / selection]
        LN[lineNumber bg/fg]
        UI[titleBar / tabBar / statusBar / panel / border]
        SY[keyword / type / string / comment / number / function / preprocessor / operator]
    end

    subgraph "Applied to"
        SS["MainWindow stylesheet<br/>(QPlainTextEdit, QStatusBar, QScrollBar)"]
        EP[Editor palette]
        LNA[LineNumberArea]
        MMP[Minimap]
        CHL[CodeHighlighter formats]
        MDH[MarkdownHighlighter dark/light]
        MPV[MarkdownPreview dark/light]
        TBR[TitleBar + tabs + window controls]
        AP[QApplication palette - dark themes]
    end

    TF --> EditorTheme
    EditorTheme --> SS & EP & LNA & MMP & CHL & MDH & MPV & TBR & AP
```

## Application Feature Map

```mermaid
mindmap
  root((TextEd))
    Editor
      Syntax Highlighting
        28 Languages
        Markdown dark/light
      Line Numbers
      Current Line Highlight
      Bracket Matching
      Auto-indent
      Font Zoom
      Line Spacing
      Whitespace Display
      Vertical Ruler
      Minimap
    File Management
      Tab Interface
      Closable Tabs
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
      cmark-gfm Parser
      GFM Extensions
      Live Preview in Tab
      Multiple Previews
      Dark/Light Mode
      LaTeX / KaTeX
      Mermaid Diagrams
      Code Highlighting
      Preview Zoom
      PDF Export
        Margins
        Page Numbers
        Orientation
        Page Borders
    Remote
      SSH / SFTP
      Remote File Browser
      File Management
      Saved Connections
      Frameless Dialog
    Appearance
      Custom Title Bar
      Hamburger Menu
      Zed External Themes
      Dark/Light Mode
      Theme-aware Scrollbars
      Separate GUI/Editor Fonts
      Configurable Settings
      Application Icon
```
