# CfdmNote — Diário de Desenvolvimento
## Sessão: 2026-03-22

---

## Contexto inicial

O projeto existia como **Keepsidian** — um gerenciador de senhas em Python/PyQt (v0.3.0-alpha), hospedado em `github.com/cfdmmarket-dot/Keepsidian`. O usuário trouxe um documento de estratégia (`KEEPSIDIAN_ESTRATEGIA.md`) descrevendo a necessidade de um pivô arquitetural completo para C++17 + Qt + CMake, baseado no KeePassXC.

---

## Decisões tomadas na sessão

### 1. Novo projeto — CfdmNote
- **Nome**: CfdmNote (definido pelo usuário)
- **Repositório**: `https://github.com/cfdmmarket-dot/CfdmNote`
- **Diretório local**: `/home/cfdm/Proj-CfdmNote_/`
- O projeto Python (Keepsidian) é mantido separado e intacto

### 2. Arquitetura definida

Após discussão, a arquitetura foi alinhada assim:

> **CfdmNote = KeePassXC completo + notas Markdown integradas (estilo Obsidian)**

**Regra inviolável:** A interface do KeePassXC não é modificada. Tudo extra é adicionado de forma não invasiva (QDockWidget, QMenu, event filters).

**Dois tipos de banco coexistindo:**
| Banco | Formato | Criptografia | Uso |
|---|---|---|---|
| Principal | `.ksdb` (SQLite) | Nenhuma (por enquanto) | Entradas do dia a dia + notas |
| Cofre | `.kdbx` (KeePass) | AES-256 + Argon2 | Dados sensíveis protegidos |

**Funcionalidades herdadas do KeePassXC (todas):**
- Entradas e grupos de senhas
- Auto-Type
- Gerador de senhas
- Importação/exportação `.kdbx`
- Todos os diálogos e relatórios

**Funcionalidades adicionadas (Obsidian):**
- Notas Markdown vinculadas a entradas
- `[[wiki-links]]` entre notas
- Grafo de conhecimento
- Busca full-text (FTS5)
- Banco SQLite nativo `.ksdb`
- Cofre criptografado opcional

### 3. Stack técnica
- **Linguagem**: C++17
- **Qt**: Qt5 (igual ao KeePassXC 2.8.0)
- **Build**: CMake 3.16+
- **Markdown**: cmark-gfm (submodule)
- **Banco**: SQLite via Qt5Sql

---

## Implementação realizada

### Estrutura do projeto
```
Proj-CfdmNote_/
├── CMakeLists.txt          ← build system (add_subdirectory keepassxc)
├── CLAUDE.md               ← contexto para o Claude Code
├── README.md
├── src/
│   ├── main.cpp            ← entry point: Application + MainWindow + dock + menu
│   ├── gui/
│   │   ├── NoteEditorWidget.h/.cpp  ← editor Markdown + preview cmark-gfm
│   │   └── VaultDialog.h/.cpp       ← janela criar cofre .kdbx criptografado
│   └── notes/
│       └── NoteManager.h/.cpp       ← SQLite CRUD + FTS5 para banco .ksdb
├── resources/
│   └── icons/
│       ├── cfdmnote.svg             ← logo (chave dourada + documento azul + [[]])
│       ├── cfdmnote_16.png
│       ├── cfdmnote_32.png
│       ├── cfdmnote_48.png
│       ├── cfdmnote_64.png
│       ├── cfdmnote_128.png
│       └── cfdmnote_256.png
├── third_party/
│   ├── keepassxc/          ← submodule KeePassXC 2.8.0
│   └── cmark-gfm/          ← submodule GitHub Flavored Markdown parser
└── docs/
    └── DEVLOG-sessao-2026-03-22.md  ← este arquivo
```

### Estratégia do main.cpp
O `main.cpp` usa a `Application` e `MainWindow` do KeePassXC diretamente:
1. `Bootstrap::bootstrap()` — inicializa o motor (crypto, config, recursos)
2. `Application app` + `MainWindow mainWindow` — interface original intacta
3. `TitleFilter` — event filter que substitui "KeePassXC" → "CfdmNote" no título em tempo real
4. `QDockWidget` com `NoteEditorWidget` — painel de notas adicionado à direita
5. Menu **CfdmNote** inserido no menubar existente
6. `patchWelcomeWidget()` — substitui texto e ícone da tela de boas-vindas

### NoteEditorWidget
- `QSplitter` horizontal: editor `QPlainTextEdit` | preview `QTextBrowser`
- cmark-gfm com extensões GFM: tabelas, strikethrough, autolink, tasklist
- `[[wiki-links]]` convertidos em links clicáveis
- Auto-save com debounce 1500ms
- Preview com debounce 300ms

### NoteManager (SQLite)
Schema do banco `.ksdb`:
- Tabelas: `groups`, `entries`, `notes`, `note_links`, `attachments`
- Índices para 40.000+ entradas
- FTS5 virtual tables (`entries_fts`, `notes_fts`) com triggers automáticos
- PRAGMA WAL + cache 32MB

### VaultDialog
- Cria cofre `.kdbx` usando `Database`, `Kdbx4Writer`, `CompositeKey`, `PasswordKey`, `Argon2Kdf`
- AES-256 + Argon2d (8 rounds, 64MB memória, 2 threads)
- Indicador visual de força de senha
- Compatível com qualquer gerenciador `.kdbx`

---

## Dependências instaladas

```bash
sudo apt-get install cmake ninja-build \
    qtbase5-dev qtbase5-private-dev qttools5-dev qttools5-dev-tools \
    libqt5sql5-sqlite libqt5x11extras5-dev libqt5svg5-dev \
    libsodium-dev libargon2-dev libbotan-2-dev \
    zlib1g-dev libqrencode-dev libminizip-dev libreadline-dev \
    libkeyutils-dev libxi-dev libxtst-dev libx11-dev
```

---

## Problemas resolvidos durante a sessão

| Problema | Solução |
|---|---|
| `PROGNAME keepassxc_base` ignorado pelo KeePassXC | Usa nome real `keepassxc` no `set_target_properties` |
| `cmark-gfm-core-extensions.h` não encontrado | Adiciona `extensions/` ao `target_include_directories` |
| `Argon2Kdf::Type::D` não existe | Enum correto: `Argon2Kdf::Type::Argon2d` |
| `KdbxWriter` é abstrato | Usa `Kdbx4Writer` (implementação concreta) |
| `Bootstrap::bootstrap(argc, argv)` inválido | Assinatura real: `Bootstrap::bootstrap()` |
| `libcmark-gfm_static` não encontrado | Nome real dos targets: `libcmark-gfm_static` e `libcmark-gfm-extensions_static` |
| Ordem de linkagem das libs cmark | Extensions antes do core: `libcmark-gfm-extensions_static libcmark-gfm_static` |
| `QSqlDatabase` não encontrado | Adiciona `Qt5::Sql` ao `target_link_libraries` |
| `botan/secmem.h` não encontrado | Adiciona `pkg_check_modules(BOTAN botan-2)` e inclui `BOTAN_INCLUDE_DIRS` |
| Resources "can't find data dir" | Cria symlink: `Proj-CfdmNote_/share → third_party/keepassxc/share` |
| Texto "Welcome to KeePassXC 2.8.0" | `patchWelcomeWidget()` encontra `welcomeLabel` por objectName e altera em runtime |
| Ícone do KeePassXC na janela | `app.setWindowIcon()` + `mainWindow.setWindowIcon()` com logo CfdmNote |

---

## Commits da sessão

| Hash | Mensagem |
|---|---|
| `8e804fd` | `feat: initial C++17/Qt6 implementation — Fase 1` |
| `a5b2428` | `refactor: nova arquitetura — KeePassXC via add_subdirectory + dock widget` |
| `ea796bd` | `fix: corrige erros de compilação — build funcional` |
| `29ac699` | `feat: logo CfdmNote — chave dourada + documento Markdown` |
| `1a828a4` | `refactor: remove todas as referências ao KeePassXC dos fontes do usuário` |
| `2fe664a` | `feat: v1.0.1 — TitleFilter substitui KeePassXC por CfdmNote em tempo real` |
| `d327769` | `feat: v1.0.1 — substitui ícone e texto de boas-vindas do KeePassXC` |

---

## Versão atual
**v1.0.1** — build funcional, app abre, importação de banco em teste.

## Próximos passos planejados
- [ ] Conectar painel de notas às entradas selecionadas
- [ ] Criar/abrir banco `.ksdb` nativo
- [ ] Personalização visual (cores, tema)
- [ ] Push para GitHub
- [ ] Fase 2: WikiLinkParser, painel de notas na árvore esquerda
- [ ] Fase 3: FTS5 + busca unificada entradas + notas
- [ ] Fase 4: Grafo de conhecimento
- [ ] Fase 5: Sync via libgit2
