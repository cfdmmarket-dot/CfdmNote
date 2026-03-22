cmake_minimum_required(VERSION 3.16)

project(CfdmNote
    VERSION 1.0.0
    DESCRIPTION "KeePassXC + Obsidian notes — by CFDM"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ─── Opções do KeePassXC ANTES do add_subdirectory ───────────────────────────
# Desativa o que não precisamos agora
set(WITH_TESTS          OFF CACHE BOOL "" FORCE)
set(WITH_GUI_TESTS      OFF CACHE BOOL "" FORCE)
set(WITH_XC_BROWSER     OFF CACHE BOOL "" FORCE)
set(WITH_XC_FDOSECRETS  OFF CACHE BOOL "" FORCE)
set(WITH_XC_SSHAGENT    OFF CACHE BOOL "" FORCE)
set(WITH_XC_YUBIKEY     OFF CACHE BOOL "" FORCE)
set(WITH_XC_KEESHARE    OFF CACHE BOOL "" FORCE)
set(WITH_XC_UPDATECHECK OFF CACHE BOOL "" FORCE)
set(WITH_XC_NETWORKING  OFF CACHE BOOL "" FORCE)
set(WITH_XC_AUTOTYPE    ON  CACHE BOOL "" FORCE)
set(WITH_APP_BUNDLE     OFF CACHE BOOL "" FORCE)
set(WITH_DOCS           OFF CACHE BOOL "" FORCE)
set(WITH_XC_DOCS        OFF CACHE BOOL "" FORCE)

# Renomeia o executável do KeePassXC para não conflitar
set(PROGNAME keepassxc_base CACHE STRING "" FORCE)

# ─── KeePassXC completo como subprojeto (Qt5, keepassxc_core, keepassxc_gui) ─
# Produz: keepassxc_core (static), keepassxc_gui (static), keepassxc_base (exe)
add_subdirectory(third_party/keepassxc EXCLUDE_FROM_ALL)

# Não precisamos do executável original do KeePassXC
set_target_properties(keepassxc_base PROPERTIES EXCLUDE_FROM_ALL TRUE)

# ─── cmark-gfm (Markdown GFM parser) ─────────────────────────────────────────
set(CMARK_TESTS  OFF CACHE BOOL "" FORCE)
set(CMARK_SHARED OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/cmark-gfm EXCLUDE_FROM_ALL)

# ─── CfdmNote — nosso executável ─────────────────────────────────────────────
set(CFDMNOTE_SRCS
    src/main.cpp
    src/gui/VaultDialog.cpp
    src/gui/NoteEditorWidget.cpp
    src/notes/NoteManager.cpp
)

add_executable(cfdmnote ${CFDMNOTE_SRCS})

target_include_directories(cfdmnote PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    # headers do KeePassXC (MainWindow, Application, Entry, etc.)
    ${CMAKE_SOURCE_DIR}/third_party/keepassxc/src
    # arquivos gerados pelo KeePassXC (ui_MainWindow.h, etc.)
    ${CMAKE_BINARY_DIR}/third_party/keepassxc/src
    # cmark-gfm
    ${CMAKE_SOURCE_DIR}/third_party/cmark-gfm/src
    ${CMAKE_BINARY_DIR}/third_party/cmark-gfm/src
)

target_link_libraries(cfdmnote PRIVATE
    keepassxc_gui       # inclui keepassxc_core transitivamente
    cmark-gfm_static
    cmark-gfmextensions_static
)

install(TARGETS cfdmnote DESTINATION bin)
