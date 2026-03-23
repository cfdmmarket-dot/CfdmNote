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

# ─── KeePassXC completo como subprojeto (Qt5, keepassxc_core, keepassxc_gui) ─
# Produz: keepassxc_core (static), keepassxc_gui (static), keepassxc (exe — não usamos)
add_subdirectory(third_party/keepassxc EXCLUDE_FROM_ALL)

# O executável original do KeePassXC se chama "keepassxc" — excluímos da build padrão
set_target_properties(keepassxc PROPERTIES EXCLUDE_FROM_ALL TRUE)

# ─── cmark-gfm (Markdown GFM parser) ─────────────────────────────────────────
set(CMARK_TESTS  OFF CACHE BOOL "" FORCE)
set(CMARK_SHARED OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/cmark-gfm EXCLUDE_FROM_ALL)

# ─── CfdmNote — nosso executável ─────────────────────────────────────────────
set(CFDMNOTE_SRCS
    src/main.cpp
    src/gui/AppLockDialog.cpp
    src/gui/SecurityQuestionsDialog.cpp
    src/gui/VaultDialog.cpp
    src/gui/VaultUnlockDialog.cpp
    src/gui/CsvImportDialog.cpp
    src/gui/LinksPanelWidget.cpp
    src/gui/BackupDialog.cpp
    src/gui/ShareDialog.cpp
    src/gui/ExportNoteDialog.cpp
    src/gui/ReminderDialog.cpp
    src/gui/ManualDialog.cpp
    src/gui/DatabaseInfoDialog.cpp
    src/gui/PowerSearchDialog.cpp
    src/gui/AudioCaptureDialog.cpp
    src/gui/TranslateDialog.cpp
    src/gui/ToastNotification.cpp
    src/gui/NoteEditorWidget.cpp
    src/reminders/ReminderManager.cpp
    src/notes/NoteManager.cpp
    src/db/MainDatabaseManager.cpp
    resources/manual.qrc
)

add_executable(cfdmnote ${CFDMNOTE_SRCS})

# Qt5Sql (necessário para NoteManager / QSqlDatabase)
find_package(Qt5 COMPONENTS Sql Network REQUIRED)

# Botan (necessário para VaultDialog / PasswordKey)
find_package(PkgConfig REQUIRED)
pkg_check_modules(BOTAN REQUIRED botan-2)

target_include_directories(cfdmnote PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    # headers do KeePassXC (MainWindow, Application, Entry, etc.)
    ${CMAKE_SOURCE_DIR}/third_party/keepassxc/src
    # arquivos gerados pelo KeePassXC (ui_MainWindow.h, etc.)
    ${CMAKE_BINARY_DIR}/third_party/keepassxc/src
    # cmark-gfm
    ${CMAKE_SOURCE_DIR}/third_party/cmark-gfm/src
    ${CMAKE_BINARY_DIR}/third_party/cmark-gfm/src
    ${CMAKE_SOURCE_DIR}/third_party/cmark-gfm/extensions
    # botan headers (ex: /usr/include/botan-2)
    ${BOTAN_INCLUDE_DIRS}
)

target_link_libraries(cfdmnote PRIVATE
    keepassxc_gui       # inclui keepassxc_core transitivamente
    Qt5::Sql
    Qt5::Network
    libcmark-gfm-extensions_static   # extensions primeiro (depende do core)
    libcmark-gfm_static               # core depois
    ${BOTAN_LIBRARIES}
)

install(TARGETS cfdmnote DESTINATION bin)
