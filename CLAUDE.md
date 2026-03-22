# CfdmNote — Contexto para o Claude Code

## O que é este projeto
Fork do KeePassXC (C++/Qt 6) com notas Markdown integradas (estilo Obsidian).
Banco SQLite próprio (.ksdb) em vez de .kdbx.
Nome do projeto: **CfdmNote**

## Regras absolutas
- NUNCA Python, NUNCA PyQt
- SEMPRE C++17 + Qt 6 + CMake
- SEMPRE partir de third_party/keepassxc/ antes de criar código novo
- NUNCA criar classe que já existe no KeePassXC — estender ou usar diretamente
- Performance é crítica: banco com 40.000+ entradas

## Estrutura
- `third_party/keepassxc/` → código base do KeePassXC (não editar)
- `third_party/cmark-gfm/` → parser Markdown GFM (não editar)
- `src/gui/Kps*.h/.cpp`   → extensões da UI do KeePassXC
- `src/notes/`            → sistema de notas Markdown (novo)
- `src/sync/`             → sync via libgit2 (futuro)

## Arquivos-chave do KeePassXC para referência
| Necessidade | Arquivo |
|---|---|
| Janela principal | `third_party/keepassxc/src/gui/MainWindow.h` |
| Widget do banco | `third_party/keepassxc/src/gui/DatabaseWidget.h` |
| Tabela de entradas | `third_party/keepassxc/src/gui/entry/EntryView.h` |
| Árvore de grupos | `third_party/keepassxc/src/gui/group/GroupView.h` |
| Modelo Entry | `third_party/keepassxc/src/core/Entry.h` |
| Leitura .kdbx | `third_party/keepassxc/src/format/KdbxReader.h` |
| Auto-Type | `third_party/keepassxc/src/autotype/AutoType.h` |

## Para compilar
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
./build/cfdmnote
```

## Formato de banco nativo: .ksdb (SQLite)
- Tabelas: groups, entries, notes, note_links, attachments
- FTS5 para busca full-text
- Importa .kdbx via KdbxReader do KeePassXC

## Repositório
- GitHub: https://github.com/cfdmmarket-dot/CfdmNote
- Branch principal: main
