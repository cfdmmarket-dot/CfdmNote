# CfdmNote — Contexto para o Claude Code

## O que é este projeto
CfdmNote = KeePassXC completo + notas Markdown integradas (estilo Obsidian).

**Regra de ouro:** A interface do KeePassXC NÃO é modificada.
Tudo extra é adicionado via QTabWidget, QMenu, QSplitter ou extensão não invasiva.

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
- **Versão em src/main.cpp DEVE ser atualizada a cada feature confirmada pelo usuário**
- **Formato versão: X.Y.Z** — criar commit git após cada bump confirmado
- **Atualizar este CLAUDE.md** a cada feature confirmada (arquitetura + arquivos)

## Workflow de versionamento
1. Feature implementada → testar
2. Usuário confirma que funciona → bump APP_VERSION em src/main.cpp
3. Atualizar CLAUDE.md (arquitetura + arquivos novos)
4. git commit com mensagem `feat(vX.Y.Z): descrição`

## Versão atual: v1.8.0
### Features confirmadas
- [x] v1.0.x — Base KeePassXC compilável + branding CfdmNote
- [x] v1.1.0 — Banco principal auto-open (sem senha), AppLockDialog, VaultUnlockDialog
- [x] v1.1.0 — SecurityQuestionsDialog (recuperação por perguntas)
- [x] v1.1.0 — CsvImportDialog com barra de progresso real (lotes de 200)
- [x] v1.1.0 — Overlay "Aguarde" durante importação
- [x] v1.2.0 — Lixeira fora da árvore de grupos (painel dedicado acima de Searches & Tags) ✓
- [x] v1.2.0 — Ícone personalizado por entrada (PNG/JPG/GIF) via menu CfdmNote ✓
- [x] v1.2.0 — Aba "Notas" integrada no preview de entrada (Ctrl+F9)
- [x] v1.2.0 — GIF animado na aba Notas quando entrada tem ícone GIF
- [x] v1.2.0 — Aba "Links" na entrada: Local Disk + External/Removable com info de unidade
- [x] v1.2.1 — Backup Externo (Ctrl+Shift+B): detecção de mídias, nota descritiva, histórico 20 entradas ✓
- [x] v1.2.2 — Backup: lista de unidades filtrada (só dispositivos físicos /dev/sd*, nvme*) ✓
- [x] v1.3.0 — Campo "Comentário" na aba Notas: observação rápida por entrada (CfdmNote::Comment) ✓
- [x] v1.3.0 — Importar TXT: cria entradas a partir de arquivos .txt (título = nome do arquivo) ✓
- [x] v1.3.0 — Links: suporte a arquivos além de pastas; botão "Adicionar" com menu Pasta/Arquivo(s) ✓
- [x] v1.4.0 — Pesquisar na web (Google/DuckDuckGo/Perplexity/ChatGPT) com título da entrada ✓
- [x] v1.4.0 — Exportar nota criptografada: AES-256-GCM + PBKDF2-SHA256 → arquivo .cfdmnote ✓
- [x] v1.4.0 — Compartilhar entrada via WhatsApp/Telegram/Discord/Facebook (sem senha) ✓
- [x] v1.5.0 — Lembretes por entrada: data/hora, recorrência, notificação com adiar/WhatsApp/Telegram ✓
- [x] v1.6.0 — Manual de Consulta integrado (F1): índice lateral, busca, renderização Markdown ✓
- [x] v1.7.0 — Informações dos Bancos (Ctrl+Shift+I): tabela ao vivo com tamanho/entradas/datas ✓
- [x] v1.7.0 — Botão 📊 DB Info na barra de status da janela principal ✓
- [x] v1.7.1 — Fix: painel Notas/Links nunca colapsa (mínimo 160px + setCollapsible false) ✓
- [x] v1.7.1 — Fix: botões sem texto em QMessageBox (MessageBoxFixer via installEventFilter) ✓
- [x] v1.7.2 — Fix: handle do splitter com 8px + gradiente azul + hover highlight ✓
- [x] v1.8.0 — Pesquisa Poderosa (Ctrl+Shift+F): busca local por arquivos, busca na web (Google/GitHub/YouTube/Wikipedia/DuckDuckGo), Google Dorking com preview, verificar link (VirusTotal/Google Safe Browsing/URLVoid) ✓
- [x] v1.5.0 — ReminderManager: verifica a cada 60s, reagenda recorrentes, adiar 1h/8h ✓
- [x] v1.5.0 — Google Calendar: botão abre browser com evento pré-preenchido (sem OAuth) ✓

## Arquitetura atual
```
main.cpp
  ├── Bootstrap::bootstrap()          ← KeePassXC (inicializa tudo)
  ├── Application app                 ← KeePassXC Application
  ├── MainWindow mainWindow           ← KeePassXC (interface original, intacta)
  │     └── m_groupSplitter (VERTICAL, injetado)
  │           ├── [0] groupView       ← grupos (Lixeira ocultada aqui)
  │           ├── [1] lixeiraWidget   ← NOSSO: botão Lixeira dedicado
  │           └── [2] tagWidget       ← Searches & Tags (KeePassXC)
  └── QMenu cfdmMenu                  ← NOSSO: menu "CfdmNote" no menubar

DatabaseWidget (por banco aberto)
  └── m_previewSplitter (VERTICAL)
        ├── [0] entryView             ← lista de entradas (KeePassXC)
        └── [1] QTabWidget "notesTabWidget"  ← NOSSO
              ├── Tab 0 "Detalhes"    ← previewView original (KeePassXC)
              ├── Tab 1 "Notas"       ← gifLabel + commentEdit (QPlainTextEdit) + NoteEditorWidget
              └── Tab 2 "Links"       ← LinksPanelWidget (pasta e arquivo; botão menu)
```

## Menu CfdmNote (itens)
| Item | Atalho | Função |
|---|---|---|
| Notas da entrada | Ctrl+F9 | Alterna aba Notas/Detalhes |
| Abrir Cofre... | Ctrl+Shift+O | Último cofre ou seletor |
| Abrir outro cofre... | — | Sempre seletor |
| Criar Cofre Criptografado... | — | VaultDialog |
| Importar CSV... | — | CsvImportDialog (barra progresso) |
| Importar TXT... | — | Importa .txt como entradas (título=nome do arquivo) |
| Pesquisar na web... | — | Submenu: Google / DuckDuckGo / Perplexity / ChatGPT |
| Pesquisa Poderosa... | Ctrl+Shift+F | PowerSearchDialog: local + web + Dorking + verificar link |
| Exportar nota... | — | ExportNoteDialog: .txt ou .cfdmnote (AES-256-GCM) |
| Compartilhar entrada... | — | ShareDialog: WhatsApp/Telegram/Discord/Facebook |
| Definir lembrete... | Ctrl+Shift+R | ReminderDialog: data/hora/recorrência/Google Calendar |
| Definir ícone da entrada... | — | Ícone personalizado (PNG/GIF/etc) |
| Sobre CfdmNote | — | Caixa sobre |

## Arquivos do projeto (src/)
| Arquivo | Descrição |
|---|---|
| `main.cpp` | Entry point — monta app + setupNotesTab + Lixeira + menu |
| `gui/AppLockDialog.h/.cpp` | Login com senha + contador tentativas (apaga a 10) |
| `gui/SecurityQuestionsDialog.h/.cpp` | Recuperação por 3 perguntas secretas |
| `gui/NoteEditorWidget.h/.cpp` | Editor Markdown + preview cmark-gfm |
| `gui/VaultDialog.h/.cpp` | Criar cofre .kdbx criptografado |
| `gui/VaultUnlockDialog.h/.cpp` | Abrir cofre .kdbx com senha + arquivo chave |
| `gui/CsvImportDialog.h/.cpp` | Import CSV com progresso real (lotes de 200) |
| `gui/LinksPanelWidget.h/.cpp` | Aba Links: Local Disk + Removable com QStorageInfo |
| `db/MainDatabaseManager.h/.cpp` | Cria/abre banco principal; injeta Lixeira no splitter |
| `notes/NoteManager.h/.cpp` | SQLite CRUD + FTS5 para banco .ksdb |

## Dois tipos de banco
| Banco | Formato | Criptografia | Uso |
|---|---|---|---|
| Principal | .kdbx (senha vazia) | Nenhuma | Entradas do dia a dia + notas |
| Cofre | .kdbx (senha real) | AES-256 + Argon2 | Dados sensíveis protegidos |

## Detalhes técnicos importantes
- `PasswordKey("")` em vez de CompositeKey vazia → faz `isInitialized()` retornar true
- Lixeira criada manualmente com UUID + icon 43 + `metadata()->setRecycleBin()`
- `setupNotesTab()` usa `QSplitter::replaceWidget()` (Qt 5.9+) para injetar QTabWidget
- Timer 400ms + fallback 800ms garantem que o widget está visível antes da injeção
- `QStorageInfo(path)` para obter device/label da unidade do link
- Links armazenados em atributos da entrada: `CfdmNote::LocalLinks`, `CfdmNote::RemovableLinks`
- Ícones GIF animados exibidos com QMovie + QBuffer na aba Notas
- `db->save(Database::Atomic, {}, &err)` limpa flag `m_modified` (evita popup unnamed buttons)

## Para compilar
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
./build/cfdmnote
```

## Repositório
- GitHub: https://github.com/cfdmmarket-dot/CfdmNote
- Branch: main
- Diretório: /home/cfdm/Proj-CfdmNote_/
