cmake_minimum_required(VERSION 3.16)
project(TextEd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui WebEngineWidgets WebEngineQuick PdfWidgets)

# cmark-gfm — download and build statically
include(FetchContent)
FetchContent_Declare(
    cmark-gfm
    GIT_REPOSITORY https://github.com/github/cmark-gfm.git
    GIT_TAG 0.29.0.gfm.13
    GIT_SHALLOW TRUE
)
set(CMARK_TESTS OFF CACHE BOOL "" FORCE)
set(CMARK_SHARED OFF CACHE BOOL "" FORCE)
set(CMARK_STATIC ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(cmark-gfm)

add_executable(text-ed
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/Editor.cpp
    src/Editor.h
    src/MarkdownHighlighter.cpp
    src/MarkdownHighlighter.h
    src/MarkdownPreview.cpp
    src/MarkdownPreview.h
    src/CodeHighlighter.cpp
    src/CodeHighlighter.h
    src/SettingsDialog.cpp
    src/SettingsDialog.h
    src/EditorTheme.cpp
    src/EditorTheme.h
    src/SshSession.cpp
    src/SshSession.h
    src/SshConnectDialog.cpp
    src/SshConnectDialog.h
    src/RemoteFileBrowser.cpp
    src/RemoteFileBrowser.h
    src/LargeFileReader.cpp
    src/LargeFileReader.h
    src/SearchBar.cpp
    src/SearchBar.h
    src/Minimap.cpp
    src/Minimap.h
    src/TitleBar.cpp
    src/TitleBar.h
    resources.qrc
)

find_library(LIBSSH2_LIBRARY ssh2)
target_include_directories(text-ed PRIVATE
    ${cmark-gfm_SOURCE_DIR}/src
    ${cmark-gfm_BINARY_DIR}/src
    ${cmark-gfm_SOURCE_DIR}/extensions
    ${cmark-gfm_BINARY_DIR}/extensions
)
target_link_libraries(text-ed PRIVATE
    Qt6::Widgets Qt6::Core Qt6::Gui Qt6::WebEngineWidgets Qt6::WebEngineQuick Qt6::PdfWidgets
    ${LIBSSH2_LIBRARY}
    libcmark-gfm-extensions_static
    libcmark-gfm_static
)
