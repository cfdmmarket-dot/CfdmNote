# CfdmNote — Diário de Desenvolvimento

> **O que é este arquivo?**
> Registro cronológico de todas as sessões de desenvolvimento do CfdmNote.
> Cada entrada documenta o que foi discutido, as decisões tomadas, o código gerado e as razões por trás de cada escolha.
> Serve como memória do projeto para o desenvolvedor e para o assistente Claude Code.

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

*CfdmNote DEVLOG — Mantido por Claude Code (Anthropic) + Cfdm Market*
*Atualizar a cada sessão de desenvolvimento*
