# CfdmNote

Gerenciador de senhas com notas Markdown integradas.

Fork do [KeePassXC](https://keepassxc.org/) (C++17 / Qt 6) com sistema de notas estilo Obsidian.

## Funcionalidades
- Banco de dados nativo `.ksdb` (SQLite) — performance para 40.000+ entradas
- Importação de `.kdbx` (KeePassXC/KeePass)
- Editor Markdown com preview em tempo real
- `[[wiki-links]]` entre notas
- Busca full-text (FTS5) em entradas e notas
- Auto-Type (Linux X11, Windows)
- Sync via libgit2 *(Fase 5)*
- Grafo de conhecimento *(Fase 4)*

## Compilar

```bash
# Instalar dependências (Ubuntu/Debian)
sudo apt-get install cmake ninja-build \
    qt6-base-dev qt6-tools-dev qt6-l10n-tools \
    libqt6sql6-sqlite libsodium-dev libargon2-dev zlib1g-dev git

# Clonar com submodules
git clone --recurse-submodules https://github.com/cfdmmarket-dot/CfdmNote.git
cd CfdmNote

# Compilar
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)

# Executar
./build/cfdmnote
```

## Estrutura
```
src/
├── main.cpp
├── gui/
│   ├── KpsMainWindow.h/.cpp     ← janela principal (estende KeePassXC)
│   └── NoteEditorWidget.h/.cpp  ← editor Markdown + preview
└── notes/
    └── NoteManager.h/.cpp       ← CRUD SQLite + FTS5
third_party/
├── keepassxc/                   ← submodule (base herdada)
└── cmark-gfm/                   ← submodule (parser Markdown GFM)
```
