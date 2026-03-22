# CfdmNote — Contexto para o Claude Code

## O que é este projeto
CfdmNote = KeePassXC completo + notas Markdown integradas (estilo Obsidian).

**Regra de ouro:** A interface do KeePassXC NÃO é modificada.
Tudo extra é adicionado como QDockWidget, QMenu ou extensão não invasiva.

## Stack
- C++17 + Qt5 + CMake
- KeePassXC 2.8.0 como submodule (third_party/keepassxc)
- cmark-gfm como submodule (third_party/cmark-gfm)
- SQLite via Qt5Sql para banco .ksdb

## Regras absolutas
- NUNCA modificar arquivos em third_party/keepassxc/
- NUNCA modificar arquivos em third_party/cmark-gfm/
- NUNCA Python, NUNCA PyQt
- SEMPRE C++17 + Qt5 + CMake
- A aparência do KeePassXC deve ser preservada integralmente

## Arquitetura
```
main.cpp
  ├── Bootstrap::bootstrap()       ← KeePassXC (inicializa tudo)
  ├── Application app              ← KeePassXC Application
  ├── MainWindow mainWindow        ← KeePassXC (interface original, intacta)
  ├── QDockWidget noteDock         ← NOSSO: painel de notas (direita)
  │     └── NoteEditorWidget       ← NOSSO: editor Markdown + preview
  └── QMenu cfdmMenu               ← NOSSO: menu "CfdmNote" no menubar
        ├── Painel de Notas
        ├── Criar Cofre Criptografado → VaultDialog
        └── Sobre CfdmNote
```

## Dois tipos de banco
| Banco | Formato | Criptografia | Uso |
|---|---|---|---|
| Principal | .ksdb (SQLite) | Nenhuma (por enquanto) | Entradas do dia a dia + notas |
| Cofre | .kdbx (KeePass) | AES-256 + Argon2 | Dados sensíveis protegidos |

## Arquivos do projeto (src/)
| Arquivo | Descrição |
|---|---|
| `main.cpp` | Entry point — monta app sobre o KeePassXC |
| `gui/NoteEditorWidget.h/.cpp` | Editor Markdown + preview (cmark-gfm) |
| `gui/VaultDialog.h/.cpp` | Janela para criar cofre .kdbx criptografado |
| `notes/NoteManager.h/.cpp` | SQLite CRUD + FTS5 para banco .ksdb |

## Arquivos do KeePassXC usados como referência
| Necessidade | Arquivo |
|---|---|
| Janela principal | `third_party/keepassxc/src/gui/MainWindow.h` |
| Aplicação Qt | `third_party/keepassxc/src/gui/Application.h` |
| Widget do banco | `third_party/keepassxc/src/gui/DatabaseWidget.h` |
| Bootstrap/init | `third_party/keepassxc/src/core/Bootstrap.h` |
| Tabela de entradas | `third_party/keepassxc/src/gui/entry/EntryView.h` |
| Árvore de grupos | `third_party/keepassxc/src/gui/group/GroupView.h` |
| Modelo Entry | `third_party/keepassxc/src/core/Entry.h` |
| Escrita .kdbx | `third_party/keepassxc/src/format/KdbxWriter.h` |
| Leitura .kdbx | `third_party/keepassxc/src/format/KdbxReader.h` |
| Auto-Type | `third_party/keepassxc/src/autotype/AutoType.h` |
| KDF Argon2 | `third_party/keepassxc/src/crypto/kdf/Argon2Kdf.h` |
| Chaves | `third_party/keepassxc/src/keys/CompositeKey.h` |

## Para compilar
```bash
# Dependências (Ubuntu/Debian)
sudo apt-get install cmake ninja-build \
    qtbase5-dev qttools5-dev qttools5-dev-tools \
    libqt5sql5-sqlite libqt5x11extras5-dev \
    libsodium-dev libargon2-dev libbotan-2-dev \
    zlib1g-dev libqrencode-dev libminizip-dev \
    libgcrypt20-dev libxi-dev libxtst-dev libx11-dev

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
./build/cfdmnote
```

## Repositório
- GitHub: https://github.com/cfdmmarket-dot/CfdmNote
- Branch: main
- Diretório: /home/cfdm/Proj-CfdmNote_/
