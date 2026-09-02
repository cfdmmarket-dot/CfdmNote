# CfdmNote — Lista de Features
> KeePassXC completo + Notas Markdown integradas (estilo Obsidian)
> C++17 + Qt5 + CMake | Fonte: Planejamento SYNCTHING + roadmap v001/v002
> Versão atual: v2.10.1 | Atualizado: 2026-04-03

---

## LEGENDA
- ✅ Implementado e confirmado
- 🔨 Em desenvolvimento
- ⬜ Pendente
- 🔗 Depende de integração externa (Nexus/ICARUS)

---

## FEATURES CONFIRMADAS (v1.0.0 → v2.10.1)

| Versão | Feature | Status |
|--------|---------|--------|
| v1.0.x | Base KeePassXC compilável + branding CfdmNote | ✅ |
| v1.1.0 | Banco principal auto-open (sem senha) | ✅ |
| v1.1.0 | AppLockDialog — login com senha + limite tentativas | ✅ |
| v1.1.0 | SecurityQuestionsDialog — recuperação por perguntas | ✅ |
| v1.1.0 | CsvImportDialog com barra de progresso real | ✅ |
| v1.2.0 | Lixeira fora da árvore de grupos | ✅ |
| v1.2.0 | Ícone personalizado por entrada (PNG/JPG/GIF) | ✅ |
| v1.2.0 | Aba "Notas" integrada no preview de entrada (Ctrl+F9) | ✅ |
| v1.2.0 | GIF animado na aba Notas | ✅ |
| v1.2.0 | Aba "Links" na entrada: Local Disk + External/Removable | ✅ |
| v1.2.1 | Backup Externo (Ctrl+Shift+B) com histórico 20 entradas | ✅ |
| v1.3.0 | Campo "Comentário" na aba Notas | ✅ |
| v1.3.0 | Importar TXT: entradas a partir de arquivos .txt | ✅ |
| v1.3.0 | Links: suporte a arquivos além de pastas | ✅ |
| v1.4.0 | Pesquisar na web (Google/DuckDuckGo/Perplexity/ChatGPT) | ✅ |
| v1.4.0 | Exportar nota criptografada (AES-256-GCM + PBKDF2-SHA256) | ✅ |
| v1.4.0 | Compartilhar entrada via WhatsApp/Telegram/Discord/Facebook | ✅ |
| v1.5.0 | Lembretes por entrada: data/hora, recorrência, notificação | ✅ |
| v1.6.0 | Manual de Consulta integrado (F1) | ✅ |
| v1.7.0 | Informações dos Bancos (Ctrl+Shift+I) | ✅ |
| v1.8.0 | Pesquisa Poderosa (Ctrl+Shift+F): local + web + Dorking | ✅ |
| v1.8.1 | Botão "⚡ CfdmNote" na toolbar | ✅ |
| v1.8.1 | Tooltip rico nas entradas (hover) | ✅ |
| v1.9.0 | Capturar Áudio / Transcrição (Ctrl+Shift+A) | ✅ |
| v1.9.1 | Tradução de notas (LibreTranslate/Google) (Ctrl+Shift+T) | ✅ |
| v1.9.1 | Pesquisar contexto da nota (extrai palavras-chave) | ✅ |
| v2.0.0 | PowerSearchDialog + seletor de navegador privado | ✅ |
| v2.1.0 | Aba "📈 Em Alta": 13 sub-abas / 5 continentes | ✅ |
| v2.1.0 | Aba "🔗 Relacionados": referências cruzadas por UUID | ✅ |
| v2.1.0 | Aba "⏰ Lembrete": painel compacto de status | ✅ |
| v2.2.0 | MindMapDialog: mapa mental interativo + export SVG | ✅ |
| v2.2.0 | RoadmapDialog: Kanban 4 colunas com status | ✅ |
| v2.2.0 | WhiteboardDialog: canvas livre + sticky notes | ✅ |
| v2.2.0 | ThemeManager SteelBlue | ✅ |
| v2.2.0 | Criar Grupo com seletor de pai (QComboBox indentado) | ✅ |
| v2.2.4 | Mover Grupo: seletor de destino com validação de ciclo | ✅ |
| v2.10.1 | KdbxImportDialog: importar .kdbx externo com senha/chave | ✅ |
| v2.10.1 | Fix CsvImportDialog: hierarquia de grupos KeePassXC | ✅ |

---

## FEATURES PENDENTES — Planejamento SYNCTHING

### GRUPO A — Extensões de Interface e UX

| ID | Feature | Origem | Prioridade |
|----|---------|--------|-----------|
| 001 | Busca avançada na rede (WWW) via IA (sem risco ao banco) | V002 | Alta |
| 002 | Comentário adicional por entrada (campo extra) | V002 | Alta |
| 003 | Sub-links na aba Links: Local-Disk / External-Removable com path direto | V002 | Alta |
| 004 | Lembretes recorrentes com link ao Google Calendar / WhatsApp / Telegram | V002 | Alta |
| 005 | Importar TXT de banco local Android (TXTPad by pohrebniakov) | V002 | Alta |
| 006 | "Abrir com" — chamar programa por extensão de arquivo nos links | V002 | Alta |
| 007 | Backup Externo com nota descritiva de onde foi salvo | V002 | Média |

### GRUPO B — Integração com ICARUS (Assistente)

| ID | Feature | Origem | Prioridade |
|----|---------|--------|-----------|
| 008 | Servidor WebSocket C++ (QtWebSockets) para receber comandos ICARUS | Interface-JARVIS | Alta |
| 009 | Protocolo JSON ICARUS → CFDMNote: get_note, get_credentials, create_note | Interface-JARVIS | Alta |
| 010 | HUD de autorização (diálogo translúcido verde) quando ICARUS acessa cofre | Interface-JARVIS | Alta |
| 011 | Botão "🤖 IA" → conectar ao Cfdm Nexus (feature 012 do roadmap) | CLAUDE.md | Alta |
| 012 | RAG: endpoint local para consulta de notas via linguagem natural | AS-IAS-Skills | Média |
| 013 | Criar nota automaticamente via agente Destilador do Nexus | AS-IAS-Skills | Média |

### GRUPO C — Visualização Holográfica (Estilo JARVIS)

| ID | Feature | Origem | Prioridade |
|----|---------|--------|-----------|
| 014 | Tema "Holographic Green" — Glassmorphism em tons de verde | Interface-JARVIS | Média |
| 015 | Grafo 3D interativo de notas (tipo Excalibrain/Obsidian) | Interface-JARVIS | Média |
| 016 | Canvas infinito de notas (tipo Obsidian Canvas) com drag | Interface-JARVIS | Média |
| 017 | Visualização de grupos como mapa de nós conectados | Interface-JARVIS | Baixa |

### GRUPO D — Segurança Avançada

| ID | Feature | Origem | Prioridade |
|----|---------|--------|-----------|
| 018 | Comunicação AES-256 local entre ICARUS e CFDMNote | Interface-JARVIS | Alta |
| 019 | Metadados visíveis / conteúdo só descriptografado via ICARUS | Interface-JARVIS | Média |
| 020 | Modo Vault-to-Note: anexar entrada KeePass a nó do grafo de conhecimento | Interface-JARVIS | Baixa |

### GRUPO E — Expansão e Sincronização

| ID | Feature | Origem | Prioridade |
|----|---------|--------|-----------|
| 021 | Sync via libgit2 (Fase 6 do roadmap) | CLAUDE.md | Média |
| 022 | Agrupamento de entradas por link de domínio (feature 010) | Roadmap | Média |
| 023 | Farejador de duplicatas / entradas similares (feature 011) | Roadmap | Média |
| 024 | AppImage + .deb + .exe (distribuição multiplataforma, feature 013) | Roadmap | Alta |
| 025 | Importar PDF (extrair texto como nota) | V001 | Média |
| 026 | Importar DOCX / XLS como entradas estruturadas | V001 | Média |

---

## ROADMAP FASES PENDENTES

| Fase | Descrição | Status |
|------|-----------|--------|
| 6 | Sync via libgit2 — sincronização de banco entre dispositivos | ⬜ |
| 7 | Servidor WebSocket C++ para integração com ICARUS | ⬜ |
| 8 | Grafo 3D de notas + Canvas infinito | ⬜ |
| 9 | AppImage/deb/exe — distribuição multiplataforma | ⬜ |
| 10 | Integração total ICARUS + CFDMNote + Nexus (ecossistema) | ⬜ |

---

## REGRAS ABSOLUTAS DO PROJETO

- NUNCA Python, NUNCA PyQt
- SEMPRE C++17 + Qt5 + CMake
- NUNCA modificar arquivos em `third_party/`
- Interface do KeePassXC preservada integralmente
- SEMPRE atualizar `src/CfdmVersion.h` a cada versão
- SEMPRE atualizar `DEVLOG.md` ao final de cada sessão
