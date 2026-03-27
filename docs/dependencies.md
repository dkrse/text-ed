# Dependencies

Build and runtime dependencies for TextEd.

## Build Dependencies

### Required packages (Debian/Ubuntu)

```bash
sudo apt install qt6-base-dev qt6-webengine-dev qt6-pdf-dev libssh2-1-dev cmake build-essential
```

### Required packages (Fedora)

```bash
sudo dnf install qt6-qtbase-devel qt6-qtwebengine-devel qt6-qtpdf-devel libssh2-devel cmake gcc-c++
```

| Package | Purpose |
|---|---|
| `cmake` | Build system (3.16+) |
| `g++` | C++17 compiler |
| Qt 6 Base | Qt 6 Widgets, Core, Gui |
| Qt 6 WebEngine | Qt 6 WebEngineWidgets, WebEngineQuick (Markdown preview) |
| Qt 6 PdfWidgets | QPdfDocument, QPdfWriter (PDF export post-processing) |
| `libssh2` | SSH/SFTP remote file editing |

### Fetched automatically at build time

| Dependency | Version | Purpose |
|---|---|---|
| cmark-gfm | 0.29.0.gfm.13 | GitHub Flavored Markdown parser (fetched via CMake FetchContent, built statically) |

## Runtime Dependencies

### Qt 6 libraries

| Library | Purpose |
|---|---|
| `libQt6Core6` | Core framework |
| `libQt6Gui6` | GUI, fonts, rendering |
| `libQt6Widgets6` | Widget toolkit |
| `libQt6WebEngineCore6` | Chromium engine (Markdown preview) |
| `libQt6WebEngineWidgets6` | WebEngine widget integration |
| `libQt6WebEngineQuick6` | WebEngine QML support |
| `libQt6PdfWidgets6` | PDF document handling |
| `libQt6Network6` | Network (transitive) |
| `libQt6OpenGL6` | OpenGL rendering |
| `libQt6DBus6` | D-Bus (desktop integration) |

### System libraries

| Library | Purpose |
|---|---|
| `libssh2` | SSH/SFTP connections |
| `libcrypto` / `libssl` | TLS (used by libssh2 and Qt) |
| `libfontconfig` | Font discovery |
| `libfreetype` | Font rendering |
| `libharfbuzz` | Text shaping |
| `libEGL` / `libGLX` | GPU rendering |
| `libglib-2.0` | GLib (used by Qt platform) |
| `libdbus-1` | D-Bus IPC |
| `libdrm` / `libgbm` | DRM/GBM (GPU buffer management) |
| `libasound` | ALSA (WebEngine audio) |
| `libexpat` | XML parsing |
| `libxkbcommon` | Keyboard handling |

### Bundled resources (no external dependency)

| Resource | Purpose |
|---|---|
| KaTeX | LaTeX math rendering in preview |
| Mermaid.js | Diagram rendering in preview |
| highlight.js | Code syntax highlighting in preview (dark + light CSS) |
| cmark-gfm | Statically linked into binary |

All rendering resources are embedded in the binary via Qt resource system (`resources.qrc`). No internet connection is required.

## CMake Configuration

```cmake
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui WebEngineWidgets WebEngineQuick PdfWidgets)

# cmark-gfm fetched and built statically
include(FetchContent)
FetchContent_Declare(cmark-gfm
    GIT_REPOSITORY https://github.com/github/cmark-gfm.git
    GIT_TAG 0.29.0.gfm.13
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(cmark-gfm)

find_library(LIBSSH2_LIBRARY ssh2)
target_link_libraries(text-ed PRIVATE
    Qt6::Widgets Qt6::Core Qt6::Gui
    Qt6::WebEngineWidgets Qt6::WebEngineQuick Qt6::PdfWidgets
    ${LIBSSH2_LIBRARY}
    libcmark-gfm-extensions_static
    libcmark-gfm_static
)
```

## Minimum Requirements

| Requirement | Minimum | Recommended |
|---|---|---|
| C++ standard | C++17 | C++17 |
| CMake | 3.16 | 3.31+ |
| Qt | 6.2 | 6.8+ |
| libssh2 | 1.8 | 1.11+ |
| OS | Linux (X11/Wayland) | Fedora 43 / Debian 13 |
