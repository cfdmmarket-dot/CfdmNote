# CfdmNote — Contexto para o Claude Code

## ⚡ Regras de Ouro do Projeto (SEMPRE aplicar)

1. **Proceed = sempre SIM** — nunca perguntar "posso prosseguir?", "quer que eu faça?", "confirma?". Se o próximo passo for claro, executar diretamente.
2. **DEVLOG.md atualizado a cada sessão** — sem exceção. Commit separado no final.
3. **MANUAL.md e ManualDialog sempre com a versão atual** — atualizar junto com cada release.

---

## O que é este projeto
CfdmNote = KeePassXC completo + notas Markdown integradas (estilo Obsidian).

**Regra de ouro de arquitetura:** A interface do KeePassXC NÃO é modificada.
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

## Versão atual: v2.2.8
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
- [x] v1.8.1 — Botão "⚡ CfdmNote" na toolbar (022): acesso rápido a todas as funções via QToolButton+menu ✓
- [x] v1.8.1 — Tooltip rico nas entradas (023): hover mostra título/usuário/URL/tags da entrada ✓
- [x] v1.8.2 — Fix: tooltip instalado em connectDbWidget (não mais timer fixo), garante funcionamento ao abrir banco ✓
- [x] v1.9.0 — Capturar Áudio / Transcrição (024): AudioCaptureDialog, grava WAV 16kHz via arecord, transcreve com whisper-cli ou python3 -m whisper, insere na nota (Ctrl+Shift+A) ✓
- [x] v1.9.1 — Tradução de notas (026): TranslateDialog, LibreTranslate (3 endpoints gratuitos, sem API key), fallback Google Translate/DeepL, inverter idiomas, inserir na nota (Ctrl+Shift+T) ✓
- [x] v1.9.1 — Pesquisar contexto da nota (027): extrai palavras-chave da nota (TF sem stopwords PT+EN, top 8), abre Google/Wikipedia/ChatGPT/GitHub com a query automática ✓
- [x] v1.9.2 — ToastNotification: popup flutuante escuro com barra regressiva, botão copiar comando, fade-in/out, auto-close 20s; usado no AudioCaptureDialog quando whisper não está instalado ✓
- [x] v2.0.0 — PowerSearchDialog: seletor de navegador privado (detecta Brave/LibreWolf/Firefox/Waterfox/Mullvad/Vivaldi/Chromium/Tor/etc, salva preferência) + aba "📈 Em Alta" (36 atalhos em 6 categorias: Vídeo, Redes Sociais, Tech/Dev, Notícias, Criadores, Compras) ✓
- [x] v2.1.0 — Aba "📈 Em Alta" expandida: 13 sub-abas cobrindo 5 continentes (BR, PT, USA, Europa, Ásia, América Latina, África, Oceania) + Finanças/Cripto, Política Global, Cultura & Ciência, Produtos Digitais, Criadores ✓
- [x] v2.1.0 — Tooltips 🔥 nos botões do trending (highlights dourados + popup escuro com descrição do site) ✓
- [x] v2.1.0 — Tooltips ricos no menu CfdmNote (setToolTipsVisible + HTML com ícone/descrição/atalho em cada item) ✓
- [x] v2.1.0 — Aba "🔗 Relacionados": auto-detecta entradas com mesmo domínio + referências cruzadas manuais por UUID (CfdmNote::RelatedEntries) + popup com detalhes + navegação ✓
- [x] v2.1.0 — Aba "⏰ Lembrete": painel compacto com status do lembrete (ativo/disparado/desativado), tempo restante, recorrência, botão editar/remover ✓
- [x] v2.1.0 — Versão exibida na tela de login (AppLockDialog): "v2.1.0" em azul abaixo do título ✓
- [x] v2.1.0 — CfdmVersion.h: fonte única de verdade para a versão (main.cpp + AppLockDialog sincronizados) ✓
- [x] v2.2.0 — MindMapDialog: mapa mental interativo com expand/collapse por clique, export SVG, painel de detalhes ✓
- [x] v2.2.0 — RoadmapDialog: Kanban 4 colunas (Backlog/InProgress/Done/Waiting), status por entrada, arrastar via menu ✓
- [x] v2.2.0 — WhiteboardDialog: canvas livre com cards de entradas + sticky notes coloridos, layout persistido ✓
- [x] v2.2.0 — ThemeManager SteelBlue: novo tema + NavyBlue toolbar clarificado (ícones visíveis) ✓
- [x] v2.2.0 — Criar Grupo com seletor de pai (Opção B): diálogo com QComboBox indentado por profundidade ✓
- [x] v2.2.0 — DEVLOG.md + seções 23/24 no manual + botão "Diário de Dev" no ManualDialog ✓
- [x] v2.2.0 — Fix: banco principal não abre mais atrás do AppLockDialog (limpeza de config na inicialização) ✓
- [x] v2.2.1 — Fix: AppLockDialog aparece corretamente após travamento/reinício (processEvents + show/raise/activateWindow) ✓
- [x] v2.2.1 — Fix: desabilita GPU do Qt5WebEngine (--disable-gpu) para evitar foco espúrio no startup ✓
- [x] v2.2.1 — Fix: MindMapDialog carrega JS/CSS inline via setHtml (evita restrições qrc:// no WebEngine) ✓
- [x] v2.2.2 — Fix: MindMap injeta JSON como objeto JS direto (não string escapada) — robusto com qualquer caractere ✓
- [x] v2.2.2 — Fix: mind.destroy() chamado antes de recriar o mapa; stack trace no erro em tela ✓
- [x] v2.2.3 — Fix: AppLockDialog remove show()+raise()+activateWindow() antes de exec() — dialog não fica "fantasma" ✓
- [x] v2.2.3 — Fix: MindMapDialog carrega HTML via arquivo temporário (file://) em vez de setHtml(qrc:/) — Chromium tem origin real ✓
- [x] v2.2.4 — Criar Grupo: atalho Ctrl+Shift+G ✓
- [x] v2.2.4 — Mover Grupo: novo item no menu CfdmNote — seleciona grupo e destino com validação de ciclo ✓
- [x] v2.2.4 — Botão toolbar: ⚡ CfdmNote → 🤖 IA (reservado para CFDM Triplex, exibe info da feature 012) ✓
- [x] v2.2.5 — Fix: overlay "Aguarde" não aparece mais ao abrir banco (removida conexão actionImport sem close, adicionado failsafe hide 3s) ✓
- [x] v2.2.6 — Fix: NavyBlue toolbar escura com texto/ícones brancos (fundo #1e3a5f, cor #ffffff) ✓
- [x] v2.2.8 — Fix: NavyBlue toolbar azul médio (#4a7fb5) — ícones KeePassXC visíveis + texto branco ✓
- [x] v2.2.6 — Fix: MindMap estilos CSS dos nós corrigidos para camelCase (borderRadius, fontSize, fontWeight) ✓
- [x] v2.2.6 — Fix: MindMap delay 300ms antes de injectMap() garante layout do WebView ✓
- [x] v2.2.0 — Git inicializado com histórico completo desde v1.0.0 ✓
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
