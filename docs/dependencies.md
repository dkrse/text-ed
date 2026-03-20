# Dependencies

Build and runtime dependencies for TextEd on Debian GNU/Linux 13 (trixie).

## Build Dependencies

### Required packages

```bash
sudo apt install qt6-base-dev qt6-webengine-dev libssh2-1-dev cmake build-essential
```

| Package | Version (trixie) | Purpose |
|---|---|---|
| `cmake` | 3.31.6 | Build system |
| `g++` | 14.2.0 | C++17 compiler |
| `qt6-base-dev` | 6.8.2 | Qt 6 Widgets, Core, Gui |
| `qt6-webengine-dev` | 6.8.2 | Qt 6 WebEngineWidgets, WebEngineQuick (Markdown preview) |
| `libssh2-1-dev` | 1.11.1 | SSH/SFTP remote file editing |

### Transitive build dependencies (auto-installed)

| Package | Purpose |
|---|---|
| `qt6-base-dev-tools` | moc, rcc, uic |
| `qt6-declarative-dev` | Required by qt6-webengine-dev |
| `qt6-positioning-dev` | Required by qt6-webengine-dev |
| `qt6-webchannel-dev` | Required by qt6-webengine-dev |
| `libgl-dev`, `libglx-dev`, `libopengl-dev` | OpenGL (Qt GUI rendering) |
| `libvulkan-dev` | Vulkan (Qt GUI rendering) |
| `libssl-dev` | OpenSSL (required by libssh2) |
| `zlib1g-dev` | Compression (required by libssh2) |

## Runtime Dependencies

### Qt 6 libraries

| Library | Package | Purpose |
|---|---|---|
| `libQt6Core6` | `libqt6core6t64` | Core framework |
| `libQt6Gui6` | `libqt6gui6` | GUI, fonts, rendering |
| `libQt6Widgets6` | `libqt6widgets6` | Widget toolkit |
| `libQt6WebEngineCore6` | `libqt6webenginecore6` | Chromium engine (Markdown preview) |
| `libQt6WebEngineWidgets6` | `libqt6webenginewidgets6` | WebEngine widget integration |
| `libQt6WebEngineQuick6` | `libqt6webenginequick6` | WebEngine QML support |
| `libQt6Network6` | `libqt6network6` | Network (transitive) |
| `libQt6OpenGL6` | `libqt6opengl6` | OpenGL rendering |
| `libQt6DBus6` | `libqt6dbus6` | D-Bus (desktop integration) |

### System libraries

| Library | Package | Purpose |
|---|---|---|
| `libssh2` | `libssh2-1t64` | SSH/SFTP connections |
| `libcrypto` / `libssl` | `libssl3t64` | TLS (used by libssh2 and Qt) |
| `libfontconfig` | `libfontconfig1` | Font discovery |
| `libfreetype` | `libfreetype6` | Font rendering |
| `libharfbuzz` | `libharfbuzz0b` | Text shaping |
| `libEGL` / `libGLX` | `libegl1`, `libglx0` | GPU rendering |
| `libglib-2.0` | `libglib2.0-0t64` | GLib (used by Qt platform) |
| `libdbus-1` | `libdbus-1-3` | D-Bus IPC |
| `libdrm` / `libgbm` | `libdrm2`, `libgbm1` | DRM/GBM (GPU buffer management) |
| `libasound` | `libasound2t64` | ALSA (WebEngine audio) |
| `libexpat` | `libexpat1` | XML parsing |
| `libxkbcommon` | `libxkbcommon0` | Keyboard handling |

### Bundled resources (no external dependency)

| Resource | Version | Purpose |
|---|---|---|
| KaTeX | bundled | LaTeX math rendering in preview |
| Mermaid.js | bundled | Diagram rendering in preview |
| highlight.js | bundled | Code syntax highlighting in preview |

All rendering resources are embedded in the binary via Qt resource system (`resources.qrc`). No internet connection is required.

## CMake Configuration

```cmake
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui WebEngineWidgets WebEngineQuick)
find_library(LIBSSH2_LIBRARY ssh2)
target_link_libraries(text-ed PRIVATE
    Qt6::Widgets Qt6::Core Qt6::Gui
    Qt6::WebEngineWidgets Qt6::WebEngineQuick
    ${LIBSSH2_LIBRARY}
)
```

## Minimum Requirements

| Requirement | Minimum | Recommended |
|---|---|---|
| C++ standard | C++17 | C++17 |
| CMake | 3.16 | 3.31+ |
| Qt | 6.2 | 6.8+ |
| libssh2 | 1.8 | 1.11+ |
| OS | Linux (X11/Wayland) | Debian 13 (trixie) |
