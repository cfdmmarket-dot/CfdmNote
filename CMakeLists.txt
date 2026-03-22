cmake_minimum_required(VERSION 3.20)

project(CfdmNote
    VERSION 1.0.0
    DESCRIPTION "KeePassXC fork with integrated Markdown notes"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ─── Qt 6 ───────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
    Xml
    Network
    Concurrent
)

# ─── KeePassXC — bibliotecas herdadas ───────────────────────────────────────
set(KPX_SRC ${CMAKE_SOURCE_DIR}/third_party/keepassxc/src)

# Desativa testes e extras do KeePassXC
set(WITH_TESTS OFF CACHE BOOL "" FORCE)
set(WITH_GUI_TESTS OFF CACHE BOOL "" FORCE)
set(WITH_BROWSER OFF CACHE BOOL "" FORCE)
set(WITH_FDOSECRETS OFF CACHE BOOL "" FORCE)
set(WITH_APP_BUNDLE OFF CACHE BOOL "" FORCE)
set(KEEPASSXC_BUILD_TYPE "Release" CACHE STRING "" FORCE)

# Core do KeePassXC (Entry, Group, Database, Config, etc.)
file(GLOB_RECURSE KPX_CORE_SRCS
    "${KPX_SRC}/core/*.cpp"
    "${KPX_SRC}/core/*.h"
)
add_library(keepassxc-core STATIC ${KPX_CORE_SRCS})
target_include_directories(keepassxc-core PUBLIC ${KPX_SRC})
target_link_libraries(keepassxc-core PUBLIC Qt6::Core Qt6::Xml Qt6::Network)

# Format (.kdbx reader/writer)
file(GLOB_RECURSE KPX_FORMAT_SRCS
    "${KPX_SRC}/format/*.cpp"
    "${KPX_SRC}/format/*.h"
)
add_library(keepassxc-format STATIC ${KPX_FORMAT_SRCS})
target_include_directories(keepassxc-format PUBLIC ${KPX_SRC})
target_link_libraries(keepassxc-format PUBLIC keepassxc-core)

# Crypto
file(GLOB_RECURSE KPX_CRYPTO_SRCS
    "${KPX_SRC}/crypto/*.cpp"
    "${KPX_SRC}/crypto/*.h"
)
add_library(keepassxc-crypto STATIC ${KPX_CRYPTO_SRCS})
target_include_directories(keepassxc-crypto PUBLIC ${KPX_SRC})
target_link_libraries(keepassxc-crypto PUBLIC Qt6::Core)

# GUI do KeePassXC
file(GLOB_RECURSE KPX_GUI_SRCS
    "${KPX_SRC}/gui/*.cpp"
    "${KPX_SRC}/gui/*.h"
    "${KPX_SRC}/gui/*.ui"
)
# Exclui módulos opcionais não usados
list(FILTER KPX_GUI_SRCS EXCLUDE REGEX ".*/browser/.*")
list(FILTER KPX_GUI_SRCS EXCLUDE REGEX ".*/fdosecrets/.*")
list(FILTER KPX_GUI_SRCS EXCLUDE REGEX ".*/sshagent/.*")

add_library(keepassxc-gui STATIC ${KPX_GUI_SRCS})
target_include_directories(keepassxc-gui PUBLIC ${KPX_SRC})
target_link_libraries(keepassxc-gui PUBLIC
    keepassxc-core
    keepassxc-format
    keepassxc-crypto
    Qt6::Widgets
    Qt6::Gui
)

# Auto-Type (Linux XCB)
if(UNIX AND NOT APPLE)
    file(GLOB KPX_AUTOTYPE_SRCS
        "${KPX_SRC}/autotype/*.cpp"
        "${KPX_SRC}/autotype/*.h"
        "${KPX_SRC}/autotype/xcb/*.cpp"
        "${KPX_SRC}/autotype/xcb/*.h"
    )
    add_library(keepassxc-autotype STATIC ${KPX_AUTOTYPE_SRCS})
    target_include_directories(keepassxc-autotype PUBLIC ${KPX_SRC})
    target_link_libraries(keepassxc-autotype PUBLIC keepassxc-gui Qt6::Widgets)
endif()

# ─── cmark-gfm (Markdown parser) ────────────────────────────────────────────
add_subdirectory(third_party/cmark-gfm EXCLUDE_FROM_ALL)

# ─── CfdmNote — código próprio ───────────────────────────────────────────────
set(CFDMNOTE_SRCS
    src/main.cpp
    src/gui/KpsMainWindow.cpp
    src/gui/NoteEditorWidget.cpp
    src/notes/NoteManager.cpp
)

add_executable(cfdmnote ${CFDMNOTE_SRCS})

target_include_directories(cfdmnote PRIVATE
    src/
    ${KPX_SRC}
    third_party/cmark-gfm/src
)

target_link_libraries(cfdmnote PRIVATE
    keepassxc-gui
    keepassxc-core
    keepassxc-format
    keepassxc-crypto
    cmark-gfm
    cmark-gfm-extensions
    Qt6::Widgets
    Qt6::Sql
    Qt6::Core
    Qt6::Gui
)

if(UNIX AND NOT APPLE)
    target_link_libraries(cfdmnote PRIVATE keepassxc-autotype)
endif()

# ─── Instalar ────────────────────────────────────────────────────────────────
install(TARGETS cfdmnote DESTINATION bin)
