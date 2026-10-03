# CfdmNote — Diário de Desenvolvimento

> **O que é este arquivo?**
> Registro cronológico de todas as sessões de desenvolvimento do CfdmNote.
> Cada entrada documenta o que foi discutido, as decisões tomadas, o código gerado e as razões por trás de cada escolha.
> Serve como memória do projeto para o desenvolvedor e para o assistente Claude Code.

---

## Sessão — AppImage v2.10.1 + make-appimage.sh fix (2026-04-09)

### O que foi feito
- `make-appimage.sh` corrigido: flags `--appimage-extract-and-run` no linuxdeploy e no appimagetool — elimina necessidade de FUSE instalado no host
- `CfdmNote-2.10.1-linux-x86_64.AppImage` gerado com sucesso (41MB, todas as libs Qt5 bundled)
- `cfdm-start.sh note` no launcher central abre o AppImage automaticamente
- AppImage executável: `chmod +x CfdmNote-2.10.1-linux-x86_64.AppImage && ./CfdmNote-2.10.1-linux-x86_64.AppImage`

---

## Sessão 01 — Fundação (v1.0.x)
**Data estimada:** 2026-01-xx

### O que foi feito
- Criação do projeto CfdmNote do zero
- KeePassXC 2.8.0 integrado como submodule em `third_party/keepassxc/`
- cmark-gfm integrado como submodule em `third_party/cmark-gfm/`
- CMakeLists.txt estruturado para compilar KeePassXC como biblioteca
- `src/main.cpp` — entry point com bootstrap do KeePassXC
- Branding inicial: nome "CfdmNote", ícone personalizado

### Decisões técnicas
- **Por que C++17 + Qt5?** Mesma stack do KeePassXC — permite usar o código-fonte diretamente sem bindings
- **Por que submodule e não fork?** Facilita atualização do KeePassXC quando versões novas saírem
- **Regra de ouro definida:** nunca modificar arquivos em `third_party/` — toda adição é feita por injeção não invasiva em `main.cpp`

---

## Sessão 02 — Login, Segurança e Importação (v1.1.0)
**Data estimada:** 2026-01-xx

### O que foi feito
- `AppLockDialog`: tela de login com senha + contador de tentativas (apaga tudo na 10ª)
- `SecurityQuestionsDialog`: recuperação por 3 perguntas secretas
- `VaultUnlockDialog`: abrir cofre .kdbx com senha + arquivo chave
- `CsvImportDialog`: importar CSV com barra de progresso real (lotes de 200 entradas)
- Overlay "Aguarde" durante importação pesada

### Decisões técnicas
- Hash da senha com `SHA-256 + salt fixo "CfdmNote::AppLock::"` — sem bcrypt para não adicionar dependência
- Lotes de 200 entradas no CSV evitam congelar a UI
- Perguntas de segurança são opcionais (usuário pode pular na primeira configuração)

---

## Sessão 03 — Lixeira, Ícones e Abas (v1.2.0)
**Data estimada:** 2026-01-xx

### O que foi feito
- Lixeira dedicada: painel próprio fora da árvore de grupos (acima de Searches & Tags)
- Ícone personalizado por entrada (PNG/JPG/GIF) via menu CfdmNote
- Aba "Notas" integrada no preview de entrada (Ctrl+F9)
- GIF animado exibido via `QMovie + QBuffer` quando a entrada tem ícone GIF
- Aba "Links": Local Disk + External/Removable com informação de unidade

### Decisões técnicas
- `QSplitter::replaceWidget()` (Qt 5.9+) para injetar o QTabWidget sem modificar o DatabaseWidget original
- Timer 400ms + fallback 800ms: garante que o widget está visível antes da injeção
- `QStorageInfo(path)` para detectar label e tipo de unidade dos links

---

## Sessão 04 — Backup Externo (v1.2.1 → v1.2.2)
**Data estimada:** 2026-01-xx

### O que foi feito
- Backup Externo (Ctrl+Shift+B): detecção automática de mídias removíveis
- Nota descritiva no arquivo de backup
- Histórico de 20 últimas operações de backup
- Filtro: só dispositivos físicos `/dev/sd*`, `nvme*` (exclui tmpfs, loop, etc.)

### Decisões técnicas
- `QStorageInfo::mountedVolumes()` + filtro por `device()` — mais confiável que `isRoot()`
- Histórico armazenado no banco principal como entrada especial

---

## Sessão 05 — Comentários, TXT e Links Avançados (v1.3.0)
**Data estimada:** 2026-01-xx

### O que foi feito
- Campo "Comentário" na aba Notas: observação rápida por entrada (atributo `CfdmNote::Comment`)
- Importar TXT: cria entradas a partir de arquivos `.txt` (título = nome do arquivo)
- Links: suporte a arquivos individuais além de pastas
- Botão "Adicionar" com menu Pasta / Arquivo(s)

---

## Sessão 06 — Pesquisa Web e Exportação Criptografada (v1.4.0)
**Data estimada:** 2026-01-xx

### O que foi feito
- Pesquisar na web com título da entrada: Google / DuckDuckGo / Perplexity / ChatGPT
- Exportar nota criptografada: AES-256-GCM + PBKDF2-SHA256 → arquivo `.cfdmnote`
- Compartilhar entrada via WhatsApp / Telegram / Discord / Facebook (sem senha)

### Decisões técnicas
- Criptografia via Qt Crypto (Botan) — já disponível no KeePassXC
- Arquivo `.cfdmnote` é um container binário: salt + IV + tag + ciphertext

---

## Sessão 07 — Lembretes com Notificação (v1.5.0)
**Data estimada:** 2026-01-xx

### O que foi feito
- Lembretes por entrada: data/hora, recorrência, notificação com adiar/WhatsApp/Telegram
- `ReminderManager`: verifica a cada 60s, reagenda recorrentes, adiar 1h/8h
- Google Calendar: botão abre browser com evento pré-preenchido (sem OAuth)
- Dados armazenados em `CfdmNote::Reminder` como JSON nos atributos da entrada

---

## Sessão 08 — Manual F1 e Info dos Bancos (v1.6.0 → v1.7.2)
**Data estimada:** 2026-01-xx

### O que foi feito
- Manual de Consulta integrado (F1): índice lateral, busca, renderização Markdown com cmark-gfm
- Informações dos Bancos (Ctrl+Shift+I): tabela ao vivo com tamanho/entradas/datas
- Botão 📊 DB Info na barra de status
- Fix: painel Notas/Links nunca colapsa (mínimo 160px + `setCollapsible(false)`)
- Fix: botões sem texto em QMessageBox (MessageBoxFixer via `installEventFilter`)
- Fix: handle do splitter com 8px + gradiente azul + hover highlight

---

## Sessão 09 — Pesquisa Poderosa e Botão CfdmNote (v1.8.0 → v1.8.2)
**Data estimada:** 2026-01-xx

### O que foi feito
- `PowerSearchDialog` (Ctrl+Shift+F): busca local por arquivos + busca na web
  - Google Dorking com preview
  - Verificar link (VirusTotal / Google Safe Browsing / URLVoid)
  - GitHub, YouTube, Wikipedia, DuckDuckGo
- Botão "⚡ CfdmNote" na toolbar: acesso rápido a todas as funções via QToolButton+menu
- Tooltip rico nas entradas (hover mostra título/usuário/URL/tags)
- Fix tooltip: instalado em `connectDbWidget` (não mais timer fixo)

---

## Sessão 10 — Áudio, Tradução e Busca por Contexto (v1.9.0 → v1.9.2)
**Data estimada:** 2026-02-xx

### O que foi feito
- `AudioCaptureDialog` (Ctrl+Shift+A): grava WAV 16kHz via `arecord`, transcreve com `whisper-cli` ou `python3 -m whisper`, insere na nota
- `TranslateDialog` (Ctrl+Shift+T): LibreTranslate (3 endpoints gratuitos, sem API key), fallback Google Translate/DeepL, inverter idiomas
- Pesquisar contexto da nota: extrai palavras-chave (TF sem stopwords PT+EN, top 8), abre Google/Wikipedia/ChatGPT/GitHub
- `ToastNotification`: popup flutuante escuro com barra regressiva, botão copiar comando, fade-in/out, auto-close 20s

### Decisões técnicas
- `arecord` em vez de PortAudio: zero dependência extra no Linux
- LibreTranslate grátis sem API key — fallback para Google/DeepL se offline
- TF-IDF simplificado (sem IDF corpus): suficiente para top 8 palavras-chave de uma nota

---

## Sessão 11 — PowerSearch Avançado + Em Alta (v2.0.0)
**Data estimada:** 2026-02-xx

### O que foi feito
- PowerSearchDialog: seletor de navegador privado
  - Detecta automaticamente: Brave / LibreWolf / Firefox / Waterfox / Mullvad / Vivaldi / Chromium / Tor
  - Salva preferência
- Aba "📈 Em Alta": 36 atalhos em 6 categorias (Vídeo, Redes Sociais, Tech/Dev, Notícias, Criadores, Compras)

---

## Sessão 12 — Em Alta Global, Tooltips e Relacionados (v2.1.0)
**Data:** 2026-03-23

### O que foi feito

#### 📈 Em Alta expandida (13 sub-abas)
- Sub-abas: Brasil, Portugal, USA, Ásia, Europa, América Latina, África, Oceania
- Sub-abas temáticas: Finanças/Cripto, Política Global, Cultura & Ciência, Produtos Digitais, Criadores
- Razão: usuário atua entre Brasil, Portugal e tem expansão planejada para USA e Ásia

#### 🔥 Highlights nos botões de trending
- Botões marcados com 🔥 recebem borda dourada (`border: 2px solid #d69e2e`)
- Tooltip escuro automático com domínio + descrição do site em cada botão
- Sites com 🔥: YouTube Trending, Google Trends, Hotmart, Mercado Livre, GitHub Trending, Product Hunt, Exploding Topics, CoinMarketCap, Bloomberg

#### Tooltips ricos no menu CfdmNote
- `cfdmMenu->setToolTipsVisible(true)`
- Cada item tem `setToolTip()` com HTML (ícone + descrição + atalho) e `setStatusTip()`
- Razão: esposa do usuário não domina informática — tudo precisa ser autoexplicativo

#### Aba 🔗 Relacionados
- Auto-detecta entradas com mesmo domínio (host sem "www.")
- Referências cruzadas manuais por UUID (armazena em `CfdmNote::RelatedEntries`)
- Popup com detalhes da entrada: título, grupo, usuário, URL, comentário
- Botões: Copiar Título, Copiar Usuário, Abrir URL, 🎯 Ir para entrada, Remover (manual)

#### Aba ⏰ Lembrete (no painel inferior)
- Painel compacto com status: ativo/disparado/desativado/sem lembrete
- Mostra tempo restante (dias/horas/minutos), recorrência, mensagem
- Botões: ✏️ Definir/Editar, 🗑 Remover
- Acesso direto sem abrir dialog separado

#### Versão na tela de login
- `CfdmVersion.h` criado — única fonte de verdade (`#define CFDMNOTE_VERSION "2.1.0"`)
- Exibida em azul (`#3182ce`) abaixo do título no `AppLockDialog`

#### Tema Navy Blue + VaultGreen automático
- `ThemeManager.h` (header-only) com QSS cobrindo todos os widgets
- Navy Blue: paleta `#1a365d / #2c5282 / #3182ce / #bee3f8 / #ebf8ff`
- VaultGreen: paleta `#1a4731 / #276749 / #38a169 / #9ae6b4 / #f0fff4`
- Troca automática: abre cofre → verde; fecha cofre → volta ao azul
- Razão: "se o banco sem criptografia for azul e o encriptado for verde, sempre saberei em qual estou"

#### Tooltip nos grupos
- `GroupTooltipFilter`: QObject com eventFilter em QTreeView
- Mostra: nome do grupo, subgrupos, entradas diretas, entradas recursivas, data de modificação

#### Tooltip enriquecido nas entradas
- Mostra: título (azul grande), usuário, URL, tags, comentário (CfdmNote::Comment), preview de 180 chars da nota, data de modificação

### Bugs corrigidos nesta sessão
1. `Metadata` incomplete type em `RelatedEntriesWidget.cpp` → removido display de ícone customizado
2. `Entry::tags()` retorna `QString` (não `QStringList`) → removido `.join(", ")`
3. `DatabaseWidget::setCurrentGroup` não existe → removida linha, mantida só a busca via QLineEdit
4. `app` não capturado no lambda do `actOpenVault` → adicionado `&app` no capture list

---

## Sessão 13 — Planejamento de Roadmap Expandido
**Data:** 2026-03-23

### Discussão: features pendentes do roadmap original

| # | Feature | Status |
|---|---------|--------|
| 011 | Agrupamento automático por link/nota | Pendente |
| 012 | Botão de IA (CFDM Triplex) | Aguarda Triplex |
| 013 | Distribuição (AppImage, .deb, .exe) | Pendente |
| 014 | Exportar notas como prompts de IA | Aguarda Triplex |
| 015 | Sugestão de projetos | Aguarda Triplex |
| 016 | Sincronizador inteligente | Aguarda Triplex |
| 017 | Arquiteto do Conhecimento | Aguarda Triplex |
| 018 | Seção do Saber (glossário, dicionário) | Pendente |

### Sugestões novas — inspiradas no Obsidian

| Sugestão | Descrição | Prioridade |
|----------|-----------|------------|
| Grafo de Conhecimento | Visualização gráfica das conexões entre entradas. Qt/QGraphicsScene ou graphviz | ⭐⭐⭐ |
| Templates de entrada | Modelos reutilizáveis: "Projeto YouTube", "Receita", "Contato" | ⭐⭐⭐ |
| Backlinks automáticos | Se nota de A menciona título de B → B mostra "A te referencia" na aba Relacionados | ⭐⭐⭐ |
| Daily Note | Entrada do dia criada automaticamente ao abrir o app | ⭐⭐ |
| Canvas / Quadro visual | Arrastar entradas em mapa mental visual (tipo Obsidian Canvas) | ⭐⭐ |
| Aliases para entradas | Apelidos para facilitar linkagem nas notas via `[[alias]]` | ⭐ |
| Modo Foco / Zen | Editor de notas em fullscreen sem distrações | ⭐ |

### Sugestões novas — inspiradas no Evernote

| Sugestão | Descrição | Prioridade |
|----------|-----------|------------|
| **Importar ENEX (Evernote)** | Parser XML do formato .enex — notas, tags, notebooks, anexos, lembretes | ⭐⭐⭐ |
| OCR em imagens | `tesseract-ocr` extrai texto de imagem/screenshot e insere na nota | ⭐⭐⭐ |
| Snippets de código | Syntax highlight no cmark-gfm render (só CSS) | ⭐⭐⭐ |
| Web Clipper | Salvar página web como entrada via script/extensão de browser | ⭐⭐⭐ |
| Histórico de versões | Ver versões anteriores de uma nota com diff por data | ⭐⭐ |
| Anotar PDF/Imagem | Anotações sobre PDF ou imagem diretamente na aba Notas | ⭐⭐ |
| Tags hierárquicas | Tags com pai/filho: `viagem/europa/portugal` | ⭐⭐ |
| Lembretes em massa | Definir lembrete para múltiplas entradas selecionadas | ⭐ |

### Importar do Evernote — viabilidade técnica
- Evernote exporta no formato **ENEX** (XML com extensão `.enex`)
- Conteúdo: título, texto (HTML), data, tags, notebook, lembrete, anexos (base64)
- Implementação: `QXmlStreamReader` (já no Qt5 — zero dependência extra)
- Cada `<note>` → uma entrada no CfdmNote
- Texto HTML → Markdown simples (ou preservar como HTML na aba Notas)
- Tags → tags da entrada KeePassXC
- Notebook → grupo no KeePassXC
- Anexos → arquivos salvos + linkados na aba Links
- **Conclusão: totalmente viável com a infraestrutura atual**

---

*CfdmNote DEVLOG — Mantido por Claude Code (Anthropic) + Cfdm Market*
*Atualizar a cada sessão de desenvolvimento*

---

## Sessão 14 — Mind Map v2 + Descoberta de Banco + Assistente de Voz
**Data:** 2026-03-24
**Versões:** v2.3.9 → v2.4.0 → v2.4.1

### Features confirmadas ✅

#### Mind Map — melhorias visuais e de interação (v2.3.9)
- **Linhas seguem nós ao arrastar** — `position:relative + left/top` resolveu o problema de nós "flutuando" sobre as linhas; `linkDiv()` do ME v5 usa `getBoundingClientRect()` que respeita o novo posicionamento
- **`branchColor` explícito por nó** — corrigiu linhas amareladas/pêssego que surgiam pelo indexamento incorreto da paleta do tema
- **Pointer events bloqueados** — `pointerdown/pointermove/pointerup` capturados no document antes do ME v5, impedindo que o `edgeMoveController` arrastasse o mapa ao passar o mouse pelas bordas sem clicar
- **Numeração hierárquica no hover** — número `1.2.3` gravado como `num` no JSON; JS aplica `title` nos `me-tpc` após init → aparece no tooltip nativo ao passar o mouse, sem poluir o visual
- **Caminho no banco (📍)** — painel lateral mostra `Root › Pessoal › Trabalho › CfdmNote` ao clicar num nó
- **Botão 📋 Detalhes** — toggle para mostrar/ocultar painel lateral (tecla `D`)
- **`linkStyle.strokeWidth=3` em nós folha** — linhas mais grossas em grupos sem filhos tornam a conexão mais visível

#### Descoberta automática de bancos (v2.4.0)
- `MainDatabaseManager::configuredPath()` / `setConfiguredPath()` — caminho persistido em `QSettings("CfdmNote","CfdmNote")`
- `discoverOnMachine()` — varre `~/`, `~/Documentos`, `~/Desktop`, Dropbox, OneDrive, `/media/$USER/*`, `/run/media/*` por `.kdbx`/`.ksdb`
- **Argumento CLI `--database /path/banco.kdbx`** — define o banco a abrir na inicialização; com validação de existência
- **Picker automático** — se banco configurado não existe e há `.kdbx` na máquina, mostra dialog com lista para o usuário escolher; botão "Criar novo" volta ao padrão

#### Assistente de Voz (v2.4.1)
- `VoiceCommandDialog` — grava até 1min (arecord), transcreve via Whisper (whisper-cli ou python3)
- **Parser de intenção** com regex PT-BR:
  - `"criar entrada <nome>"` → cria `Entry` no grupo atual
  - `"novo grupo <nome>"` → cria `Group` filho do grupo atual
  - `"adicionar nota <texto>"` → append na nota da entrada selecionada
- **Confirmação antes de executar** — mostra "→ Criar entrada **GitHub** no grupo **Dev**" e aguarda clique em ✔ Executar ou ✎ Corrigir
- Acesso: Menu CfdmNote → Assistente de Voz (`Ctrl+Shift+V`) e Botão 🤖 IA na toolbar
- Botão 🤖 IA virou menu: item Assistente de Voz + item CFDM Triplex IA (desabilitado, em breve)

### Lembrete (feature anterior confirmada nesta sessão)
- Sistema de lembretes funcionando perfeitamente ✅

### Decisão de arquitetura
- **Mind Map é puramente visual** — arrastar nós não altera o banco; reorganização real é feita na interface KeePassXC + recarregar o mapa
- **Banco configurável por sessão** — `configuredPath()` persiste entre reinícios via `QSettings`

### Convenção estabelecida
- **DEVLOG.md deve ser atualizado a cada sessão** (instrução permanente do usuário)

---

*CfdmNote DEVLOG — Mantido por Claude Code (Anthropic) + Cfdm Market*
*Atualizar a cada sessão de desenvolvimento*

---

## Sessão 15 — Verificador de Dependências + Regras de Ouro
**Data:** 2026-03-24
**Versão:** v2.4.2

### Features confirmadas ✅

#### Verificador de Dependências (v2.4.2)
- `DependencyCheckDialog` — tabela com status ✅/⚠/❌ para cada dependência
- Dependências verificadas: `arecord`, `aplay`, `python3`, `openai-whisper`, `whisper-cli`, `ffmpeg`, `xclip`, `tesseract-ocr`
- Cada item mostra: **feature habilitada** + **comando de instalação exato**
- Botão "Copiar comandos faltando" — coloca todos os `apt install`/`pip install` no clipboard
- Roda automaticamente na primeira execução ou sempre que algo estiver faltando (800ms após login)
- Acessível a qualquer momento: Menu CfdmNote → "🔍 Verificar Dependências..."
- `QSettings("CfdmNote","CfdmNote").value("startup/depcheck_done")` evita popup repetido quando tudo ok

#### openai-whisper instalado na máquina
- Versão: 20250625
- Comando usado: `pip install openai-whisper --break-system-packages`
- Modelo base (~140MB) será baixado na primeira transcrição automaticamente

### Regras de ouro estabelecidas (persistidas na memória do Claude)
1. **DEVLOG.md atualizado a cada sessão** — sem exceção
2. **Proceed = Always Yes** — Claude executa diretamente sem pedir confirmação; só consulta em trade-offs de arquitetura reais

---

## Sessão 17 — Feature 006: Abrir com + Lista de Funcionalidades (v2.4.3)
**Data:** 2026-03-24
**Versão:** v2.4.3

### Features implementadas ✅

#### Aba Links — "Abrir com" (feature 006)
- **Menu de contexto** (clique direito) em ambas as listas (Local Disk + Removable)
- Opções: 📂 Abrir · 📁 Abrir pasta que contém · ⎘ Copiar caminho · 🖥 Abrir com...
- **Detecção automática de apps** por extensão via `QStandardPaths::findExecutable` + `xdg-mime query default`
- Apps mapeados para 30+ extensões: pdf, docx, mp4, mp3, jpg, txt, md, py, zip, etc.
- **Lembrar app por extensão** via `QSettings("CfdmNote","CfdmNote").value("openwith/<ext>")`
- "Abrir pasta que contém" disponível apenas para arquivos (não pastas)
- "Copiar caminho" copia para clipboard sem abrir nada
- Duplo clique continua com comportamento original (xdg-open)

#### Lista de Funcionalidades no Manual (seção 29)
- Tabela completa de todas as features com versão e atalho
- Botão "⚡ Funcionalidades" no rodapé do ManualDialog — navega direto para a seção 29
- Seção 30 explica o novo "Abrir com" com tabela de extensões

### Decisão técnica
- Context menu injeto via `Qt::CustomContextMenu` + `customContextMenuRequested` — não interfere no KeePassXC
- `QMimeDatabase` (Qt5 built-in) para detecção de MIME type — sem dependência externa
- Apps detectados só aparecem no menu se estiverem instalados na máquina

---

## Sessão 21 — Importar Documentos e Imagens como Notas (v2.5.2)
**Data:** 2026-03-24
**Versão:** v2.5.2

### Feature: ImportDocDialog — multi-formato + OCR
- `ImportDocDialog.h/.cpp` criados do zero
- Formatos suportados:
  - `.txt`, `.md`, `.log`, `.csv` → QFile direto
  - `.pdf` → `pdftotext -layout` (poppler-utils)
  - `.docx` → `unzip -p word/document.xml` + stripXmlTags
  - `.odt` → `unzip -p content.xml` + stripXmlTags
  - `.rtf` → `unrtf --text`
  - `.html`, `.htm` → stripHtmlTags (regex Qt)
  - `.png`, `.jpg`, `.jpeg`, `.bmp`, `.tif`, `.tiff`, `.webp`, `.gif` → **OCR via tesseract**
- `extractText()` movido para `public static` → reutilizável no botão da aba Notas
- Seletor de grupo destino com QComboBox indentado por profundidade
- Progresso real por arquivo (QProgressBar)
- Menu CfdmNote: "Importar TXT" substituído por "Importar Documentos / Imagens..."

### Feature: Botão na aba Notas
- Mini-toolbar adicionada na aba Notas: `📄 Importar Documento / Imagem`
- Importa um arquivo e **acrescenta** o texto ao final da nota da entrada selecionada
- Prefixo `📄 *Importado de: nome-do-arquivo*` inserido antes do conteúdo
- Botão desabilitado quando nenhuma entrada está selecionada

### Decisões técnicas
- `extractImage()` usa `tesseract <input> stdout -l por+eng` com fallback para só `eng`
- `.webp` e `.gif` convertidos para PNG temporário via ImageMagick (`convert`) antes do OCR
- Lambda do botão usa `dbWidget->findChild<NoteEditorWidget*>()` (não captura `noteEditor` diretamente — declarado depois no mesmo escopo)
- `stripXmlTags()` e `stripHtmlTags()` usam QRegularExpression puro, sem dependências externas

---

## Sessão 20 — Importar Grupo entre Bancos (v2.5.1)
**Data:** 2026-03-24
**Versão:** v2.5.1

### Feature: Merge/Import de grupos entre .kdbx
- `MergeGroupDialog`: seletor de banco origem + seletor de banco destino (qualquer par de bancos abertos)
- Árvore de grupos em ambos os lados com contagem de entradas e subgrupos
- `Group::clone(Entry::CloneDefault, Group::CloneDefault)` → clone recursivo completo com novos UUIDs
- `cloned->setParent(dest)` insere no banco de destino
- Banco de destino salvo automaticamente após import
- Conta entradas/subgrupos antes da confirmação para dar visibilidade ao usuário
- Atalho: `Ctrl+Shift+M` | Menu CfdmNote → "📥 Importar Grupo entre Bancos..."
- MANUAL.md seção 32

### Clarificação ao usuário
O banco principal do CfdmNote usa `.kdbx` (mesmo formato KeePassXC, senha vazia). O `.ksdb` é arquivo SQLite separado só para notas/glossário. O merge funciona entre qualquer `.kdbx`.

---

## Sessão 19 — Seção do Saber: Glossário Pessoal (v2.5.0)
**Data:** 2026-03-24
**Versão:** v2.5.0

### Feature 018 — Glossário integrado
- `NoteManager` estendido: tabela `glossary` (id, term, definition, category, synonyms, example, created, modified)
- Métodos: `addGlossaryTerm`, `updateGlossaryTerm`, `deleteGlossaryTerm`, `getGlossaryTerms`, `searchGlossaryTerms`, `getGlossaryCategories`
- Migração automática via `CREATE TABLE IF NOT EXISTS` — bancos existentes recebem a tabela sem recriar
- `GlossaryDialog`: 3 painéis (categorias | tabela | detalhe), busca em tempo real, editor modal
- Categoria é um ComboBox editável — usuário cria categorias novas diretamente
- Duplo clique no termo abre editor; `Delete` apaga com confirmação
- "📝 Inserir na Nota" adiciona bloco formatado (📖 **Termo** + definição + sinônimos + exemplo) na nota da entrada atual
- Atalho: `Ctrl+Shift+K` | Menu CfdmNote → "🧠 Seção do Saber — Glossário..."
- MANUAL.md: seção 31 + entrada na tabela de funcionalidades

### Nota técnica
- Bugfix: `sed -i` com `\n` gera literal em vez de newline em alguns contextos → CMakeLists corrompido → linker não encontrava `main()`. Corrigido manualmente.

---

## Sessão 18 — Fix transcrição de áudio (v2.4.5)
**Data:** 2026-03-24
**Versão:** v2.4.5

### Problema
Usuário gravava áudio mas transcrição não funcionava. Causa raiz: o binário `whisper` instalado pelo pip fica em `~/.local/bin/whisper` — caminho que o Qt não inclui no PATH ao lançar o app via GUI.

### Fix aplicado
- `whisperEngine()` agora busca em 3 níveis:
  1. `whisper-cli` (whisper.cpp)
  2. `whisper` no PATH + caminhos absolutos conhecidos (`~/.local/bin/whisper`, `/usr/local/bin/whisper`, `/usr/bin/whisper`)
  3. `python3 -m whisper` como fallback
- Timeout aumentado de 3s para 8s (Python + torch levam ~2s para carregar)
- `onTranscribe()`: process anterior é deletado antes de criar novo (sem leak)
- `readyReadStandardError` conectado → status label mostra progresso em tempo real (download do modelo, etc.)
- `waitForStarted(3000)` verifica se o processo realmente iniciou
- `onTranscribeFinished()`: lê txt pelo caminho correto (`QDir::tempPath() + basename + ".txt"`); fallback em stdout e depois stderr; limpa arquivo temp após leitura

### Diagnóstico que revelou o problema
`find / -name "whisper-cli"` → encontrou `/home/cfdm/.local/bin/whisper` (openai-whisper CLI)

---

## Sessão 16 — Regra de Ouro Global: Proceed = Always Yes
**Data:** 2026-03-24
**Versão:** v2.4.2

### O que aconteceu
- CLAUDE.md atualizado com bloco de Regras de Ouro no topo (persistidas entre sessões)
- Usuário reforçou: a regra **Proceed = Always Yes** vale para **tudo** que fizermos juntos, não só neste projeto
- Regra atualizada na memória do Claude para aplicação global
- Usuário não precisa ficar na frente do ecrã — Claude deve trabalhar de forma autônoma

### Regra de ouro reforçada
- **Nunca perguntar "posso prosseguir?"** — executar diretamente se o próximo passo é claro
- **Aplica a todos os projetos** — não só ao CfdmNote
- **Proceed = Always Yes** é regra global, permanente, sem exceção para confirmações óbvias

---

## Sessão 17 — AudioCaptureDialog v2 + Diamond Branding + ImportDocDialog (v2.6.0)
**Data:** 2026-03-24
**Versão:** v2.6.0

### O que foi feito

#### ImportDocDialog (importar docs e imagens como nota)
- Novo `src/gui/ImportDocDialog.h/.cpp`: importa `.txt`, `.pdf`, `.docx`, `.odt`, `.rtf`, `.html`, `.htm` e imagens (PNG/JPG/BMP/TIFF/WEBP/GIF)
- `extractText()` público e estático — despachador por extensão
- Ferramentas externas: `pdftotext`, `unzip` + parse XML (docx/odt), `unrtf`, para cada formato
- Imagens: `tesseract input stdout -l por+eng` com fallback para `eng`; WEBP/GIF → PNG temp via `convert` (ImageMagick) antes do OCR
- `stripXmlTags()` e `stripHtmlTags()` com `QRegularExpression` — zero dependências extra
- Texto importado via `ned->appendText()` (evita crash com `entry->setNotes()` + `db->save()`)
- Caminho do arquivo original adicionado automaticamente à aba Links via `linksPanel->addExternalLink()`

#### Fix crash no import
- Root cause: `entry->setNotes()` + `db->save()` conflitavam com sinais internos do KeePassXC
- Solução: `NoteEditorWidget::appendText()` — passa pelo NoteManager SQLite, sem `db->save()` direto

#### Fix aba Links
- Seções sobrepostas: substituído `setFixedHeight(110)` por `QSplitter(Qt::Vertical)` com `setCollapsible(false)`
- Scrollbar e seta para baixo agora funcionam corretamente
- Ícones por tipo de arquivo: `QMimeDatabase::mimeTypeForFile()` + `QIcon::fromTheme(mime.iconName())`

#### Diamond Branding (ícones CFDM Holding)
- Todos os ícones substituídos pelo estilo diamante CFDM Holding
- `cfdmnote.svg` — fundo escuro circular, diamante brilhante (Table/Girdle/Culet em 256px), anel orbital, sparkles
- `diamond_toolbar.svg`, `diamond_tray.svg`, `diamond_animated.svg` criados
- `diamond_logo_animated.gif` — 24 frames via Python+Inkscape+ImageMagick (112KB)
- PNGs em todas as resoluções: 16/32/48/64/128/256px
- `resources/manual.qrc` atualizado com todos os ícones

#### AudioCaptureDialog v2 (reescrita completa)
- **WaveformWidget**: lê últimos 3200 bytes do WAV em crescimento, calcula RMS de samples S16_LE, barras coloridas (ciano/amarelo/vermelho/escuro), indicador REC piscante, timer 120ms
- **SpellHighlighter**: `QSyntaxHighlighter` com `aspell -a` em pipe persistente; sublinha palavras desconhecidas; menu contexto com sugestões
- **Layout novo**: Waveform + info arquivo + transcrição + spell toolbar + seção IA (5 botões stub → CFDM Triplex Feature 012)
- **updateFileInfo()**: tamanho em MB + duração estimada do WAV

### Fix de compilação desta sessão
- `qSqrt` exigia `#include <QtMath>` — adicionado no topo de AudioCaptureDialog.cpp

---

## Sessão 18 — AppearanceDialog + IconThemeDialog (v2.7.0)
**Data:** 2026-03-24
**Versão:** v2.7.0

### O que foi feito

#### CfdmNote-Icones_ — pasta de ícones para edição
- Todos os ícones copiados para `/home/cfdm/Proj-CFDM-NOTE_/CfdmNote-Icones_/`
- SVGs, PNGs e GIFs para fácil edição e criação de variantes

#### IconThemeDialog — gerenciar ícones
- `src/gui/IconThemeDialog.h/.cpp`: lista todos os ícones com preview 40x40
- Grupos: App (6 resoluções), Toolbar/IA, Tray/Animações
- **Trocar** → QFileDialog → copia para `~/.config/cfdmnote/icons/`
- **Restaurar** por ícone (desfaz override individual)
- **Restaurar Todos** → limpa pasta de overrides
- `IconThemeDialog::icon()` — user folder primeiro, QRC fallback

#### AppearanceDialog — personalizar cores e fonte
- `src/gui/AppearanceDialog.h/.cpp`: 4 color pickers + QFontComboBox + QSpinBox
- Cores: Fundo, Toolbar, Texto, Destaque
- Preview inline em tempo real
- **Prévia** aplica no app sem salvar; **Aplicar e Salvar** persiste em QSettings
- **Restaurar Padrão** volta ao NavyBlue
- `buildQss()` gera QSS completo (~30 seletores) a partir de 4 cores base
- Startup: `AppearanceDialog::applySaved()` carrega tema customizado se salvo

#### Acesso via clique direito
- Clique direito na janela principal → menu contextual: Personalizar / Gerenciar Ícones / atalhos de tema
- Menu CfdmNote → 🎨 Tema de Cores → Personalizar Aparência / Gerenciar Ícones

---

## Sessão 17 — Importar .kdbx Externo + Fix CSV Hierárquico (v2.10.1)
**Data:** 2026-04-02

### Problema relatado
Dificuldade em importar dados de `.kdbx` antigo para o CfdmNote.
Dois problemas identificados: rota CSV com grupos hierárquicos quebrada, e ausência de importação direta de `.kdbx` com senha.

### Features implementadas ✅

#### KdbxImportDialog — nova feature (Ctrl+Shift+K)
- `src/gui/KdbxImportDialog.h/.cpp` — novo arquivo
- Abre qualquer `.kdbx` externo diretamente (sem abrir nas abas do app)
- Suporte a senha opcional + arquivo chave opcional
- Importa todos os grupos e entradas **recursivamente** com hierarquia preservada
- Pula a Lixeira do banco de origem (`metadata()->recycleBin()`)
- Acesso: Menu CfdmNote → "📂 Importar .kdbx Externo..." (Ctrl+Shift+K)

**Técnico:**
- `Database::create(path)` + `db->open(key, &error)` — mesmo padrão do VaultUnlockDialog
- `importGroupRecursive()` — clona entradas via `Entry::clone(Entry::CloneNewUuid)`, cria grupos com `new Group()` e `setParent()`
- `Group::setIcon(int)` para preservar ícone do grupo de origem

#### Fix CsvImportDialog — grupos hierárquicos
- Função `resolveGroup(QString rawPath)` substituiu o lookup plano anterior
- Parse do caminho por `/` com `Qt::SkipEmptyParts`
- Remove primeiro segmento se igual ao nome da raiz ("Root")
- Cache por caminho completo: `"Pessoal/YouTube"` como chave
- Resultado: `"Root/Pessoal/YouTube"` → cria estrutura `Root → Pessoal → YouTube`

### Rename: Triplex → Cfdm Nexus
- "CFDM Triplex" e "CFDM AI OS TRIPLEX" → novo nome oficial: **Cfdm Nexus**
- Atualizado em: CLAUDE.md, MEMORY.md, roadmap, relatórios dos 3 projetos

### Fluxo recomendado: antigo .kdbx → CfdmNote
**Opção A (preferencial):** Menu CfdmNote → 📂 Importar .kdbx Externo... → senha → Importar
**Opção B (CSV):** KeePassXC → Database → Export CSV → CfdmNote → Importar CSV

---

## Sessão 18 — Branding CFDM Holding + Fix Banner KeePassXC + Desktop Versionado (v2.10.1)
**Data:** 2026-03-30

### Problemas resolvidos ✅

#### Fix: Banner "KeePassXC 2.8.0 unstable build" persistente
- **Causa raiz:** `KEEPASSXC_BUILD_TYPE=Snapshot` no CMakeCache gerava `KEEPASSXC_BUILD_TYPE_SNAPSHOT` — ativava o banner via `globalMessageWidget`
- **Solução dupla:**
  1. `CMakeLists.txt`: `set(KEEPASSXC_BUILD_TYPE "Release" CACHE STRING "" FORCE)` — elimina o define na compilação
  2. `main.cpp`: `config()->set(Config::Messages_HidePreReleaseWarning, QString(KEEPASSXC_VERSION))` antes do AppLockDialog — suprime runtime
- **Resultado:** banner nunca mais aparece

#### Versionamento no atalho Desktop
- `package/cfdmnote.desktop.in`: `Name=CfdmNote v@CFDMNOTE_VERSION@` + `Comment=CfdmNote v@CFDMNOTE_VERSION@ — KeePassXC + Markdown — CFDM Holding`
- `CMakeLists.txt`: `deploy-desktop` copia `.desktop` gerado para `~/Desktop/CfdmNote.desktop` além de `~/.local/share/applications/`
- **Resultado:** botão na área de trabalho sempre mostra a versão atual (auto-atualiza a cada build)

### Branding CFDM Holding ✅

#### Logo CFDM Holding em todos os produtos
- `resources/icons/cfdm-holding-logo.png` — copiado de `CfdmNote-Fotos-Imagens_/Logos/CfdmHolding-logo-Image .png`
- `resources/manual.qrc` — logo embedded no binário via QRC: `<file alias="cfdm-holding-logo.png">icons/cfdm-holding-logo.png</file>`

#### AppLockDialog (tela de login)
- Layout dual-logo: diamante CfdmNote (64×64) + CFDM Holding (90×64) lado a lado
- Tenta carregar holding logo do filesystem → fallback QRC
- Regra: os dois logos sempre juntos, nunca substituindo

#### Diálogo "Sobre CfdmNote"
- Reescrito de `QMessageBox::about()` para `QDialog` customizado
- Exibe ambos os logos + texto "Um produto CFDM Holding"

#### ManualDialog (Manual F1)
- Logo CFDM Holding (56×28) no rodapé antes do label de versão
- Texto da versão: `"CfdmNote v%1 — Manual de Consulta · CFDM Holding"`

#### Cfdm Nexus — web interface
- `web/templates/index.html`: sidebar — logo CFDM Holding + "Cfdm Nexus" + "CFDM Holding — AI OS"
- `web/templates/search.html`: header — mesma identidade visual (logo + Cfdm Nexus + subtítulo)
- Título das páginas: "Cfdm Nexus — CFDM Holding"

### Regra estabelecida (permanente)
> Em TODOS os produtos CFDM Holding: logo oficial aparece sempre que possível, ao lado do logo do produto — nunca substituindo. Texto padrão: "Um produto CFDM Holding".

---

*CfdmNote DEVLOG — Mantido por Claude Code (Anthropic) + Cfdm Market*
*Atualizar a cada sessão de desenvolvimento*

---

## Sessão 26 — Bugfix Desktop Files + Investigação Crash CSV (v2.10.1)
**Data:** 2026-04-02

### Cfdm Nexus — Desktop Files corrigidos
- **Problema:** `Broken Desktop Files` ao tentar abrir o Cfdm Nexus pelo atalho
- **Causa:** parênteses `(` e `)` no caminho `Proj-Cfdm-NEXUS-AI-OS-(Triplex )_/` são caracteres reservados no formato `.desktop` — precisavam de escape
- **Fix:** campo `Exec` corrigido para `\(Triplex\ \)` nos dois arquivos:
  - `/home/cfdm/Desktop/CfdmNexus.desktop`
  - `/home/cfdm/.local/share/applications/cfdm-nexus.desktop`
- `desktop-file-validate` passou sem erros após a correção

### CfdmNote — Investigação crash ao mover grupos importados via CSV
- **Sintoma:** app fechava subitamente ao mover grupo importado por CSV (ex: "CFDM-UNIVERSE-DIGITAL-GALAX")
- **Investigação:** analisado o caminho completo `GroupModel::dropMimeData` → `Group::setParent` → `emit aboutToMove` → `GroupModel::groupAboutToMove` → `beginMoveRows` → `Q_ASSERT(moveResult)`
- **Modo Debug:** `Q_ASSERT` ativo → `abort()` se `beginMoveRows` retorna false
- **Causa provável:** estado transitório pós-importação CSV; após reabrir o banco o crash cessou
- **Nota:** NoteManager não tem `openDatabase()` chamado no startup → mensagens "QSqlQuery: database not open" em stderr (falha silenciosa, não causa crash)
- **Status:** usuário confirmou que está OK após reabrir o banco


## 2026-10-03 — 🕒 Nota "Última atualização" gravada pelo próprio app
- A cada gravação da base aberta, o app atualiza **🕒 Atualizações → Última atualização** na MESMA gravação (data/hora, ficheiro, nº de notas e grupos, computador). Campo Utilizador = data/hora (visível na lista). Histórico da nota = últimas gravações.
- Mudança em `third_party/keepassxc/src/core/Database.cpp` (`Database::saveAs`), guardada como `patches/keepassxc-ultima-atualizacao.patch` porque o submódulo aponta para o KeePassXC oficial. Num clone novo: `git -C third_party/keepassxc apply ../../patches/keepassxc-ultima-atualizacao.patch` antes de compilar.
- Sem ciclo de gravações: `markAsClean()` depois do save pára o temporizador de alteração.
- Testado pelo Cristiano: gravou com Ctrl+S, a nota apareceu.
- Mesma sessão: base única = `/home/cfdm/CfdmNote-Database_/CfdmNote-PRINCIPAL.kdbx` (pasta tinha sido renomeada para `Cfdm-Note-Database_`, devolvida ao nome original); app passa a abrir sempre esse ficheiro; 219 cópias antigas arquivadas em `~/CfdmNote-ARQUIVO-antigos/` (lista lá dentro, nada apagado).
