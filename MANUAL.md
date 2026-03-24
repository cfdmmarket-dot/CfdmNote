# CfdmNote — Manual de Consulta
**Versão 2.4.3** · Atualizado em 2026-03-24 · Plataforma Linux · C++17 + Qt5

> 💡 **Como saber se o manual está atualizado:** a versão acima deve coincidir com a versão exibida na tela de login. Se for diferente, o manual está desatualizado para essa release.

---

## Sumário

1. [Sobre o CfdmNote](#1-sobre-o-cfdmnote)
2. [Início Rápido](#2-início-rápido)
3. [Tela Principal — Estrutura Visual](#3-tela-principal--estrutura-visual)
4. [Banco de Dados](#4-banco-de-dados)
5. [Entradas — O que é e o que guarda](#5-entradas--o-que-é-e-o-que-guarda)
6. [Aba Detalhes](#6-aba-detalhes)
7. [Aba Notas](#7-aba-notas)
8. [Aba Links](#8-aba-links)
9. [Lixeira](#9-lixeira)
10. [Menu CfdmNote — Guia Completo](#10-menu-cfdmnote--guia-completo)
11. [Backup Externo](#11-backup-externo)
12. [Cofre Criptografado](#12-cofre-criptografado)
13. [Importar Dados](#13-importar-dados)
14. [Pesquisar na Web](#14-pesquisar-na-web)
15. [Lembretes](#15-lembretes)
16. [Compartilhar Entrada](#16-compartilhar-entrada)
17. [Exportar Nota](#17-exportar-nota)
18. [Atalhos de Teclado](#18-atalhos-de-teclado)
19. [Segurança — O que é protegido e o que não é](#19-segurança--o-que-é-protegido-e-o-que-não-é)
20. [Estrutura Técnica do Sistema](#20-estrutura-técnica-do-sistema)
21. [Roadmap — Próximas Funcionalidades](#21-roadmap--próximas-funcionalidades)
22. [Compilar e Executar](#22-compilar-e-executar)
23. [Roadmap Expandido — Obsidian + Evernote](#23-roadmap-expandido--obsidian--evernote)
24. [Diário de Desenvolvimento](#24-diário-de-desenvolvimento)
25. [Mapa Mental](#25-mapa-mental)
26. [Assistente de Voz](#26-assistente-de-voz)
27. [Verificador de Dependências](#27-verificador-de-dependências)
28. [Descoberta e CLI de Banco de Dados](#28-descoberta-e-cli-de-banco-de-dados)
29. [Lista Completa de Funcionalidades](#29-lista-completa-de-funcionalidades)
30. [Aba Links — Abrir com Programa](#30-aba-links--abrir-com-programa)

---

## 1. Sobre o CfdmNote

**CfdmNote** é um gerenciador pessoal de senhas, notas e conhecimento.

É construído sobre o **KeePassXC 2.8.0** (motor de banco de dados criptografado, Auto-Type, grupos e entradas) com uma camada de extensões desenvolvidas pela CFDM que adicionam:

- Notas Markdown vinculadas a cada entrada
- Campo de comentário rápido por entrada
- Links para pastas e arquivos do disco
- Ícones personalizados por entrada (incluindo GIFs animados)
- Lembretes recorrentes com notificação
- Backup externo para HDs e pendrives
- Importação de CSV e TXT
- Pesquisa web integrada
- Compartilhamento via apps de mensagem
- Exportação de notas com criptografia

**Princípio fundamental:** a interface original do KeePassXC é preservada integralmente. Todas as extensões do CfdmNote são adicionadas de forma não invasiva (menus, abas, painéis extras).

### Tecnologia

| Componente | Tecnologia |
|---|---|
| Interface gráfica | Qt5 (C++17) |
| Motor de banco | KeePassXC 2.8.0 (submodule) |
| Formato de banco | .kdbx (padrão KeePass) |
| Parser Markdown | cmark-gfm (GitHub Flavored Markdown) |
| Banco de notas | SQLite (Qt5Sql) — arquivo .ksdb |
| Criptografia | Botan 2.19.3 (AES-256-GCM, Argon2, PBKDF2) |
| Build | CMake 3.16+ |

---

## 2. Início Rápido

### Ao abrir o CfdmNote

1. **Tela de Login** — digite a senha do app (AppLockDialog).
   - Se esquecer: clique em "Recuperar" → responda às 3 perguntas secretas.
   - Após 10 tentativas erradas o banco é apagado automaticamente.

2. O banco principal abre automaticamente (sem senha adicional).

3. A janela principal do KeePassXC aparece com o menu **CfdmNote** adicionado na barra de menus.

### Criando a primeira entrada

1. Clique com botão direito em um grupo → **Adicionar entrada**
2. Preencha: Título, Usuário, Senha, URL
3. Clique em uma entrada para ver os detalhes no painel inferior
4. Acesse as abas **Notas** e **Links** para adicionar conteúdo extra

---

## 3. Tela Principal — Estrutura Visual

```
┌─────────────────────────────────────────────────────────────────┐
│  Arquivo  Entradas  Ver  Ferramentas  CfdmNote  Ajuda           │  ← Barra de menus
├──────────────────┬──────────────────────────────────────────────┤
│                  │  Título     │  Usuário  │  Senha  │  URL     │
│  GRUPOS          │─────────────┴───────────┴─────────┴──────────│  ← Lista de entradas
│  ├─ Grupo 1      │  ■ Entrada A                                  │
│  ├─ Grupo 2      │  ■ Entrada B                                  │
│  └─ ...          │  ■ Entrada C                                  │
│                  ├──────────────────────────────────────────────┤
│ ┌──────────────┐ │  [ Detalhes ] [ Notas ] [ Links ]            │
│ │  🗑 Lixeira  │ │                                               │
│ └──────────────┘ │  Conteúdo da aba selecionada                 │  ← Painel de preview
│                  │                                               │
│  Searches & Tags │                                               │
└──────────────────┴──────────────────────────────────────────────┘
```

### Painéis principais

| Painel | Descrição |
|---|---|
| **Grupos** (esquerda) | Árvore de grupos para organizar entradas. A Lixeira fica como botão dedicado abaixo dos grupos. |
| **Lista de entradas** (centro-superior) | Entradas do grupo selecionado. Clique para selecionar. |
| **Abas de preview** (centro-inferior) | Três abas: Detalhes, Notas, Links. Aparecem ao selecionar uma entrada. |

---

## 4. Banco de Dados

O CfdmNote trabalha com **dois tipos** de banco simultaneamente:

| Tipo | Arquivo | Criptografia | Para quê |
|---|---|---|---|
| **Banco Principal** | `.kdbx` (senha vazia) | Nenhuma | Entradas do cotidiano, notas, links |
| **Cofre** | `.kdbx` (senha real) | AES-256 + Argon2 | Dados altamente sensíveis |

O banco principal abre automaticamente ao iniciar. O cofre só abre quando você invocar pelo menu CfdmNote.

### Arquivo de notas (.ksdb)

As notas Markdown longas são armazenadas em um banco SQLite separado (`.ksdb`), vinculadas ao UUID de cada entrada. Isso mantém o arquivo `.kdbx` leve e as notas pesquisáveis por texto completo (FTS5).

---

## 5. Entradas — O que é e o que guarda

Uma **entrada** é o registro central do CfdmNote. Cada entrada pode conter:

| Campo | Descrição |
|---|---|
| Título | Nome da entrada (ex: "Netflix", "Servidor SSH") |
| Usuário | Login/e-mail |
| Senha | Protegida; visível ao clicar no olho |
| URL | Endereço web associado |
| Notas (padrão) | Campo de texto simples do KeePassXC |
| **Comentário** | Observação rápida (campo CfdmNote, aba Notas) |
| **Nota Markdown** | Texto longo com formatação (aba Notas) |
| **Links** | Caminhos para pastas/arquivos (aba Links) |
| **Ícone personalizado** | PNG, JPG ou GIF animado |
| **Lembrete** | Data/hora + recorrência + mensagem |
| Tags | Etiquetas do KeePassXC |

### Atributos personalizados CfdmNote

Internamente o CfdmNote armazena dados extras como atributos da entrada KeePass:

| Atributo | Conteúdo |
|---|---|
| `CfdmNote::Comment` | Texto do campo Comentário |
| `CfdmNote::LocalLinks` | Caminhos locais separados por `\n` |
| `CfdmNote::RemovableLinks` | Caminhos removíveis separados por `\n` |
| `CfdmNote::Reminder` | JSON: datetime, recurrence, message, enabled |

---

## 6. Aba Detalhes

A aba padrão mostra o preview original do KeePassXC:
- Título, usuário, URL, notas padrão
- Histórico de alterações
- Ícone da entrada

Esta aba **não é modificada** pelo CfdmNote — é exatamente como no KeePassXC original.

---

## 7. Aba Notas

Ativada com **Ctrl+F9** ou clicando na aba "Notas".

### Componentes

```
┌─────────────────────────────────────────────────────┐
│  [GIF animado — aparece só se entrada tem ícone GIF] │
├─────────────────────────────────────────────────────┤
│  Comentário: [ campo de texto rápido, 3 linhas ]    │
├─────────────────────────────────────────────────────┤
│                                                     │
│  Editor Markdown (texto longo)                      │
│                                                     │
│  Suporta: **negrito**, _itálico_, # Títulos,        │
│  - listas, [links](url), `código`, tabelas GFM      │
│                                                     │
└─────────────────────────────────────────────────────┘
```

### Campo Comentário
- Observação rápida de até algumas linhas
- Salva automaticamente 600ms após parar de digitar
- Exibido também no ShareDialog ao compartilhar

### Editor Markdown
- Suporta GitHub Flavored Markdown (GFM)
- Preview renderizado via cmark-gfm
- Notas salvas no banco SQLite (.ksdb) vinculadas ao UUID da entrada
- Persiste mesmo se a entrada for renomeada

### GIF Animado
- Aparece no topo da aba Notas se a entrada tiver um ícone GIF animado definido
- Animação reproduz em loop automaticamente

---

## 8. Aba Links

Permite vincular **pastas e arquivos** do disco a uma entrada.

### Seções

| Seção | Para quê |
|---|---|
| **Local Disk** | Pastas/arquivos no disco interno |
| **External / Removable Disk** | Pastas/arquivos em HD externo, pendrive, etc. |

### Adicionar link

Clique em **Adicionar...** → aparece um menu com duas opções:

- **Pasta...** — abre seletor de diretório
- **Arquivo(s)...** — abre seletor de um ou mais arquivos

### Informações exibidas

Para cada link adicionado o CfdmNote exibe:
- **Pasta:** nome da unidade, device (`/dev/sdb1`) e subdiretório
- **Arquivo:** nome do arquivo e diretório pai
- Ícone verde (encontrado) ou vermelho (não encontrado / mídia desmontada)
- Tooltip com caminho completo

### Abrir

- Duplo clique no item **ou** botão **Abrir** → abre no gerenciador de arquivos padrão do sistema
- Se for um arquivo: abre com o aplicativo padrão do sistema (Open With)

### Remover

Selecione o item → botão **Remover**.

Os links são salvos como atributos da entrada. Se a entrada for excluída, os links somem com ela.

---

## 9. Lixeira

A lixeira fica como um **botão dedicado** entre a árvore de grupos e o painel "Searches & Tags" — fora da árvore de grupos para não poluir a hierarquia.

- Clicar no botão **🗑 Lixeira** navega diretamente para o grupo Lixeira
- Entradas na lixeira podem ser restauradas (botão direito → Mover para grupo)
- Para esvaziar a lixeira: menu Ferramentas → Esvaziar Lixeira

---

## 10. Menu CfdmNote — Guia Completo

O menu **CfdmNote** aparece na barra de menus à direita do menu "Ferramentas".

### Notas e Conteúdo

| Item | Atalho | O que faz |
|---|---|---|
| **Notas da entrada** | Ctrl+F9 | Alterna entre aba Detalhes e aba Notas na entrada selecionada |
| **Pesquisar na web...** | — | Submenu: abre browser com o título da entrada como query |

### Cofre

| Item | Atalho | O que faz |
|---|---|---|
| **Abrir Cofre...** | Ctrl+Shift+O | Abre o último cofre usado (ou seletor de arquivo) |
| **Abrir outro cofre...** | — | Sempre mostra seletor de arquivo |
| **Criar Cofre Criptografado...** | — | Cria novo arquivo .kdbx com senha e criptografia AES-256 |

### Backup e Importação

| Item | Atalho | O que faz |
|---|---|---|
| **Backup Externo...** | Ctrl+Shift+B | Copia o banco para HD externo ou pendrive |
| **Importar CSV...** | — | Importa entradas de arquivo CSV com progresso real |
| **Importar TXT...** | — | Importa arquivos .txt como entradas (título = nome do arquivo) |

### Ferramentas de Entrada

| Item | Atalho | O que faz |
|---|---|---|
| **Definir lembrete...** | Ctrl+Shift+R | Cria/edita lembrete com recorrência para a entrada selecionada |
| **Exportar nota...** | — | Exporta a nota como .txt ou .cfdmnote (criptografado) |
| **Compartilhar entrada...** | — | Compartilha via WhatsApp, Telegram, Discord ou Facebook |
| **Definir ícone da entrada...** | — | Define ícone personalizado PNG/JPG/GIF para a entrada |

### Sobre

| Item | O que faz |
|---|---|
| **Sobre CfdmNote** | Exibe versão e lista de funcionalidades |

---

## 11. Backup Externo

**Menu CfdmNote → Backup Externo... (Ctrl+Shift+B)**

### Como usar

1. Selecione a unidade de destino na lista (mostra apenas dispositivos físicos: `/dev/sd*`, `/dev/nvme*`, etc.)
2. O nome do arquivo é gerado automaticamente: `CfdmNote_backup_AAAA-MM-DD_HH-MM.kdbx`
3. Adicione uma **nota descritiva** opcional (ex: "USB Kingston preto 32GB")
4. Clique em **Fazer Backup**

### O que acontece

- O arquivo do banco ativo é copiado para a unidade selecionada
- O CfdmNote verifica a integridade: compara o tamanho do original com a cópia
- O backup é registrado no histórico (máximo 20 entradas)

### Histórico de backups

- Lista todos os backups anteriores com data, destino e nota
- Ícone ✅ = arquivo existe | ⚠ = arquivo não encontrado (mídia desmontada)
- Duplo clique → abre a pasta do backup no gerenciador de arquivos
- Botão **Remover** → exclui o registro do histórico (não apaga o arquivo)

---

## 12. Cofre Criptografado

O cofre é um banco `.kdbx` com senha real e criptografia forte. Ideal para dados sensíveis como documentos de identidade, chaves privadas, contratos.

### Criar um cofre

**Menu CfdmNote → Criar Cofre Criptografado...**
1. Escolha onde salvar o arquivo `.kdbx`
2. Digite e confirme uma senha forte
3. Opcionalmente adicione um arquivo-chave (segunda camada de segurança)

### Abrir um cofre

**Menu CfdmNote → Abrir Cofre... (Ctrl+Shift+O)**
- Se já abriu um cofre antes: abre o último usado automaticamente
- Para escolher outro: **Abrir outro cofre...**

O cofre abre em uma nova aba ao lado do banco principal.

---

## 13. Importar Dados

### Importar CSV

**Menu CfdmNote → Importar CSV...**

Detecta automaticamente as colunas:
`Title`, `Username`, `Password`, `URL`, `Notes`, `Group`

- Barra de progresso real (processa em lotes de 200)
- Cria grupos automaticamente se a coluna "Group" existir
- Overlay "Aguarde" evita fechar o app durante a importação

### Importar TXT

**Menu CfdmNote → Importar TXT...**

Selecione um ou mais arquivos `.txt`.
Para cada arquivo:
- **Título da entrada** = nome do arquivo (sem extensão)
- **Notas da entrada** = conteúdo completo do arquivo

Útil para migrar anotações do TXTPad ou outros apps de texto.

---

## 14. Pesquisar na Web

**Menu CfdmNote → Pesquisar na web...**

Selecione uma entrada → escolha o mecanismo:

| Opção | URL usada |
|---|---|
| Google | `google.com/search?q=<título>` |
| DuckDuckGo | `duckduckgo.com/?q=<título>` |
| Perplexity AI | `perplexity.ai/search?q=<título>` |
| ChatGPT | `chatgpt.com/?q=<título>` |

O **título** da entrada é usado como query. Nenhum dado sensível (senha, usuário) é enviado.

---

## 15. Lembretes

**Menu CfdmNote → Definir lembrete... (Ctrl+Shift+R)**

### Configurar

| Campo | Descrição |
|---|---|
| Data | Calendário popup para escolher a data |
| Hora | Horário no formato HH:MM |
| Recorrência | Uma vez / Diário / Semanal / Mensal / Anual |
| Mensagem | Texto opcional que aparece na notificação |

### Google Calendar

O botão **Google Calendar** abre o browser com um evento já preenchido (título, data, hora, URL da entrada). Não requer login nem OAuth.

### Como a notificação funciona

O CfdmNote verifica os lembretes a cada **60 segundos**. Quando um dispara:
- A janela do app é trazida para frente
- Aparece um diálogo com o título e mensagem da entrada

| Botão | Ação |
|---|---|
| **OK** | Marca como lido; se recorrente, agenda próxima ocorrência |
| **Adiar 1h** | Reagenda para daqui a 1 hora |
| **Adiar 8h** | Reagenda para daqui a 8 horas |
| **WhatsApp** | Abre WhatsApp com a mensagem do lembrete |
| **Telegram** | Abre Telegram com a mensagem do lembrete |

### Recorrência inteligente

Se o app ficou fechado por vários dias e havia um lembrete semanal, ele pula automaticamente as ocorrências passadas e agenda a **próxima data futura** — sem acumular notificações antigas.

### Remover lembrete

Abra **Definir lembrete...** na entrada → botão **Remover**.

---

## 16. Compartilhar Entrada

**Menu CfdmNote → Compartilhar entrada...**

### O que é compartilhado

| Incluído | Excluído |
|---|---|
| Título | Senha |
| Comentário | Usuário/login |
| URL | Dados do cofre |
| Extrato das Notas (até 300 chars) | |

### Plataformas

| Plataforma | Como funciona |
|---|---|
| **WhatsApp** | Abre browser/app com texto pré-preenchido |
| **Telegram** | Abre browser/app com texto pré-preenchido |
| **Discord** | Copia o texto para a área de transferência (colar com Ctrl+V) |
| **Facebook** | Abre browser com texto/URL pré-preenchido |
| **Copiar** | Copia o texto para a área de transferência |

O texto é **editável** no dialog antes de enviar. Você pode remover ou adicionar informações manualmente.

---

## 17. Exportar Nota

**Menu CfdmNote → Exportar nota...**

Exporta a nota Markdown da entrada selecionada para um arquivo.

### Formatos

| Formato | Extensão | Criptografia |
|---|---|---|
| Texto simples | `.txt` | Nenhuma |
| Nota CfdmNote | `.cfdmnote` | AES-256-GCM + PBKDF2-SHA256 |

### Exportar como .cfdmnote (criptografado)

1. Marque a opção "Exportar com criptografia"
2. Digite e confirme uma senha
3. Clique em Exportar

**Algoritmo:** PBKDF2-SHA256 com 200.000 iterações deriva a chave de 32 bytes. AES-256-GCM garante confidencialidade e integridade (detecta se o arquivo foi alterado).

**Formato do arquivo binário:**

```
[8 bytes]  Magic: "CFDMNOTE"
[1 byte]   Versão: 1
[32 bytes] Salt (aleatório)
[12 bytes] Nonce (aleatório)
[4 bytes]  Tamanho do ciphertext (little-endian)
[N bytes]  Ciphertext + tag GCM (16 bytes no final)
```

---

## 18. Atalhos de Teclado

| Atalho | Função |
|---|---|
| **Ctrl+F9** | Alternar aba Notas ↔ Detalhes |
| **Ctrl+Shift+B** | Abrir Backup Externo |
| **Ctrl+Shift+O** | Abrir Cofre |
| **Ctrl+Shift+R** | Definir Lembrete na entrada selecionada |
| **Ctrl+N** | Nova entrada (KeePassXC) |
| **Ctrl+E** | Editar entrada selecionada (KeePassXC) |
| **Delete** | Mover entrada para Lixeira (KeePassXC) |
| **Ctrl+C** | Copiar senha (KeePassXC) |
| **Ctrl+B** | Copiar usuário (KeePassXC) |
| **Ctrl+U** | Abrir URL no browser (KeePassXC) |
| **Ctrl+F** | Buscar entradas (KeePassXC) |

---

## 19. Segurança — O que é protegido e o que não é

### Banco Principal

O banco principal abre **sem senha** (acesso protegido apenas pelo login do app). Os dados **não são criptografados** no disco. Use para informações do cotidiano que não precisam de proteção forte.

### Banco do App (Login)

A senha do AppLockDialog é armazenada com hash seguro no arquivo de configuração (`cfdmnote.conf`). Após 10 tentativas erradas, o banco é apagado.

### Cofre

O cofre usa **AES-256 + Argon2** (algoritmos do KeePassXC). É o lugar certo para:
- Documentos digitalizados (RG, CPF, passaporte)
- Chaves SSH/GPG
- Seeds de carteiras de criptomoedas
- Qualquer dado que não pode vazar

### Exportação de nota (.cfdmnote)

Usa **AES-256-GCM** com chave derivada por PBKDF2-SHA256 (200k iterações). O arquivo pode ser transportado em qualquer mídia com segurança.

### Compartilhamento

O ShareDialog **nunca inclui senha**. O texto compartilhado vai para servidores de terceiros (WhatsApp, Telegram, Facebook) quando você confirma o envio. Revise sempre antes de compartilhar.

### Lembretes

Armazenados como atributos na entrada do banco `.kdbx`. Se o banco principal não é criptografado, os lembretes também não são.

---

## 20. Estrutura Técnica do Sistema

Esta seção é voltada para desenvolvedores ou usuários avançados que queiram entender como o app é organizado internamente.

### Arquitetura de camadas

```
┌────────────────────────────────────────────────────┐
│  CAMADA DE APRESENTAÇÃO (src/gui/)                 │
│                                                    │
│  AppLockDialog       — Login com senha             │
│  SecurityQuestionsDialog — Recuperação             │
│  NoteEditorWidget    — Editor Markdown             │
│  LinksPanelWidget    — Aba Links                   │
│  BackupDialog        — Backup externo              │
│  VaultDialog         — Criar cofre                 │
│  VaultUnlockDialog   — Abrir cofre                 │
│  CsvImportDialog     — Importar CSV                │
│  ReminderDialog      — Definir lembrete            │
│  ShareDialog         — Compartilhar                │
│  ExportNoteDialog    — Exportar nota cifrada       │
├────────────────────────────────────────────────────┤
│  CAMADA DE LÓGICA (src/reminders/, src/db/)        │
│                                                    │
│  ReminderManager     — Verifica lembretes (60s)    │
│  MainDatabaseManager — Abre banco + injeta Lixeira │
├────────────────────────────────────────────────────┤
│  CAMADA DE DADOS (src/notes/)                      │
│                                                    │
│  NoteManager         — CRUD notas SQLite + FTS5    │
├────────────────────────────────────────────────────┤
│  MOTOR (third_party/keepassxc/)  [não modificado]  │
│                                                    │
│  keepassxc_core  — Database, Entry, Group, Crypto  │
│  keepassxc_gui   — MainWindow, DatabaseWidget      │
├────────────────────────────────────────────────────┤
│  CRIPTOGRAFIA (Botan 2.19.3)  [sistema]            │
│  MARKDOWN (cmark-gfm)  [submodule, não modificado] │
└────────────────────────────────────────────────────┘
```

### Injeção de widgets (não invasiva)

O CfdmNote **não modifica** o código do KeePassXC. Em vez disso, injeta componentes após a janela estar visível:

| Técnica | Onde é usada |
|---|---|
| `QSplitter::replaceWidget()` | Substitui o painel de preview por QTabWidget com 3 abas |
| `QTreeView::setRowHidden()` | Oculta a Lixeira da árvore de grupos |
| `menuBar()->insertMenu()` | Adiciona o menu CfdmNote antes do menu Ajuda |
| `QTimer::singleShot(400ms)` | Aguarda widgets estarem visíveis antes de injetar |

### Formato dos dados extras por entrada

```
Entrada KeePass (Entry)
  ├── Atributos padrão: title, username, password, url, notes
  └── Atributos personalizados (EntryAttributes):
        CfdmNote::Comment      → texto simples
        CfdmNote::LocalLinks   → "/caminho/1\n/caminho/2"
        CfdmNote::RemovableLinks → "/media/user/HD\n..."
        CfdmNote::Reminder     → {"datetime":"...","recurrence":"weekly",...}
```

### Fluxo de salvamento

Todo dado modificado pelo CfdmNote é salvo via:
```
db->save(Database::Atomic, {}, &errorString)
```
Isso garante que o flag `m_modified` do KeePassXC é limpo corretamente, evitando o popup de "salvar/descartar" com botões sem nome.

---

## 21. Roadmap — Próximas Funcionalidades

| # | Feature | Status |
|---|---|---|
| 010 | Agrupamento automático de entradas por link/nota compartilhada | Planejado |
| 011 | Farejador/Pesquisa: busca local + GitHub + YouTube + Wikipedia + trends | Planejado |
| 012 | Botão de IA — integração com CFDM Triplex (agentes) | Planejado |
| 013 | Distribuição: AppImage Linux, instalador .deb/.rpm, instalador Windows | Planejado |
| 014 | Exportar notas como prompts sequenciais para CFDM Triplex | Planejado |
| 015 | Ofertador de projetos com as 5 fases + sugestão de prompts | Planejado |
| 016 | Sincronizador: sugere compartilhar notas por relevância/frequência | Planejado |
| 017 | Arquiteto do Conhecimento: sugere tarefas e projetos com base no contexto | Planejado |
| 018 | Seção do Saber: glossário, dicionário, tradutor, sinônimos | Planejado |

> As features 012, 014, 015, 016 e 017 dependem da integração com o **CFDM Triplex** (motor de agentes de IA).
> As demais (011, 013, 018) são independentes e podem ser implementadas a qualquer momento.

---

## 22. Compilar e Executar

### Pré-requisitos

```bash
# Ubuntu/Debian
sudo apt install cmake build-essential qt5-default libqt5sql5 \
     libbotan-2-dev pkg-config
```

### Compilar

```bash
cd /home/cfdm/Proj-CfdmNote_/
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

### Executar

```bash
./build/cfdmnote
```

### Repositório

- GitHub: https://github.com/cfdmmarket-dot/CfdmNote
- Branch: `main`
- Diretório local: `/home/cfdm/Proj-CfdmNote_/`

---

## 23. Roadmap Expandido — Obsidian + Evernote

### Inspirações do Obsidian

| Feature | Descrição | Prioridade |
|---------|-----------|------------|
| **Grafo de Conhecimento** | Visualização gráfica das conexões entre entradas. Cada entrada é um nó; links relacionados são arestas. Permite ver clusters de conhecimento de forma visual | ⭐⭐⭐ |
| **Templates de entrada** | Modelos reutilizáveis: "Projeto YouTube", "Receita", "Lead de Vendas", "Contato". Ao criar entrada nova, escolhe o template e os campos são preenchidos automaticamente | ⭐⭐⭐ |
| **Backlinks automáticos** | Se a nota da entrada A menciona o título da entrada B, a entrada B exibe automaticamente "A te referencia" na aba 🔗 Relacionados — exatamente como o Obsidian faz | ⭐⭐⭐ |
| **Daily Note** | Entrada do dia criada automaticamente ao abrir o app (data como título). Ideal para journaling, log diário, lista de tarefas do dia | ⭐⭐ |
| **Canvas / Quadro Visual** | Arrastar entradas em um quadro livre (tipo Miro ou Obsidian Canvas) para criar mapas mentais e fluxos de trabalho visuais | ⭐⭐ |
| **Modo Foco / Zen** | Abre só o editor de notas em tela cheia, sem distrações — ideal para escrever textos longos | ⭐ |

### Inspirações do Evernote

| Feature | Descrição | Prioridade |
|---------|-----------|------------|
| **Importar ENEX** | Importar arquivos `.enex` exportados do Evernote: notas, tags, notebooks, anexos e lembretes. Implementação com `QXmlStreamReader` — zero dependências extras | ⭐⭐⭐ |
| **OCR em imagens** | Extrair texto de imagem ou screenshot e inserir na nota. Usa `tesseract-ocr` (disponível no Linux via apt) | ⭐⭐⭐ |
| **Snippets de código** | Blocos de código com syntax highlight na visualização Markdown. O `cmark-gfm` já suporta — só precisa de CSS adicional | ⭐⭐⭐ |
| **Web Clipper** | Salvar uma página web diretamente como entrada (via script de linha de comando ou extensão de browser) | ⭐⭐⭐ |
| **Histórico de versões** | Ver versões anteriores de uma nota com diff por data — "Note History" do Evernote | ⭐⭐ |
| **Anotar PDF/Imagem** | Abrir PDF ou imagem na aba Notas com anotações por cima (highlight, texto) | ⭐⭐ |
| **Tags hierárquicas** | Tags com estrutura pai/filho: `viagem/europa/portugal` — como no Evernote e Obsidian | ⭐⭐ |

### Importar do Evernote — como funciona

O Evernote exporta no formato **ENEX** (arquivo `.enex`, que é XML):

```
Evernote (.enex)
  └── <note>              → entrada no CfdmNote
        ├── <title>       → título da entrada
        ├── <content>     → texto HTML → convertido para Markdown
        ├── <tag>         → tags da entrada
        ├── <notebook>    → grupo no CfdmNote
        ├── <reminder>    → CfdmNote::Reminder
        └── <resource>    → anexos (base64) → aba Links
```

**Para exportar do Evernote:** Arquivo → Exportar notas → Formato ENEX
**Depois:** CfdmNote → Menu CfdmNote → Importar ENEX *(feature a implementar)*

---

## 24. Diário de Desenvolvimento

O CfdmNote mantém um **diário completo de desenvolvimento** que documenta cada sessão de trabalho: features discutidas, decisões técnicas, bugs encontrados e soluções aplicadas.

### Onde encontrar

- **Arquivo:** `DEVLOG.md` na raiz do projeto (`/home/cfdm/Proj-CfdmNote_/DEVLOG.md`)
- **GitHub:** https://github.com/cfdmmarket-dot/CfdmNote/blob/main/DEVLOG.md

### Estrutura de cada entrada

Cada sessão no DEVLOG contém:
- **O que foi feito** — lista das features implementadas
- **Decisões técnicas** — *por que* cada solução foi escolhida
- **Bugs corrigidos** — problema, causa e solução
- **Discussões de roadmap** — sugestões avaliadas e priorizadas

### Por que isto é importante

O diário permite que:
1. O desenvolvedor entenda o raciocínio por trás de cada escolha sem precisar ler o código inteiro
2. Novos colaboradores se integrem rapidamente ao projeto
3. Features rejeitadas ou adiadas sejam revisitadas com contexto completo
4. O assistente de IA (Claude Code) mantenha continuidade entre sessões de trabalho

> **Nota:** Este manual e o DEVLOG são atualizados a cada versão confirmada do CfdmNote, seguindo o workflow de versionamento: feature implementada → testada → bump de versão → commit `feat(vX.Y.Z)`.

---

---

## 25. Mapa Mental

**Acesso:** Menu CfdmNote → Mapa Mental · ou botão na toolbar

O Mapa Mental gera automaticamente uma visualização gráfica de todos os grupos e entradas do banco aberto.

### Interação
| Ação | Resultado |
|------|-----------|
| Clique num nó | Expande ou recolhe os filhos |
| Arrastar nó | Move livremente no canvas (apenas visual) |
| Rolar scroll | Zoom in/out |
| Arrastar fundo | Panorâmica do mapa |
| `Ctrl+Z` / botão ↩ | Desfaz último movimento de nó |

### Hover — número hierárquico
Ao passar o mouse sobre qualquer nó aparece um tooltip com a numeração hierárquica:
- Grupos de topo: `1`, `2`, `3`…
- Sub-grupos: `1.1`, `1.2`…
- Sub-sub-grupos: `1.1.1`…

Isso ajuda a identificar a posição do elemento sem seguir as linhas.

### Painel de detalhes
Ao clicar num nó o painel lateral mostra:
- Título e tipo (grupo ou entrada)
- Caminho completo no banco: `📍 Root › Pessoal › Trabalho › CfdmNote`
- Informações extras (usuário, URL, tags — para entradas)

### Botões da toolbar
| Botão | Função |
|-------|--------|
| ↺ Recarregar | Regenera o mapa com dados atuais do banco |
| 🔍+ / 🔍− | Zoom in / out |
| ⊡ Caber | Centraliza o mapa |
| 💾 PNG | Exporta screenshot do mapa |
| ↩ Desfazer | Desfaz último movimento de nó |
| 📋 Detalhes | Mostra/oculta painel lateral (tecla `D`) |
| ↔ Lateral / → Direita / ← Esquerda | Direção do layout |
| Entradas | Mostra/oculta entradas individuais no mapa |

### Importante
O mapa é **puramente visual** — arrastar nós não altera a estrutura do banco.
Para reorganizar grupos/entradas use a interface principal do KeePassXC e depois clique em Recarregar.

---

## 26. Assistente de Voz

**Acesso:** Menu CfdmNote → Assistente de Voz · `Ctrl+Shift+V` · Botão 🤖 IA na toolbar

**Requisito:** `openai-whisper` instalado (`pip install openai-whisper`)

O Assistente de Voz permite criar entradas, grupos e notas falando um comando.

### Como usar
1. Selecione o grupo ou entrada de destino na interface principal
2. Abra o Assistente de Voz
3. Clique em **● Gravar** e diga o comando
4. Clique em **■ Parar**
5. Clique em **Transcrever**
6. Confirme a ação detectada e clique em **✔ Executar**

### Comandos suportados
| O que dizer | Ação executada |
|-------------|----------------|
| `"Criar entrada GitHub"` | Cria entrada com título "GitHub" no grupo atual |
| `"Nova entrada Banco Nubank"` | Cria entrada "Banco Nubank" no grupo atual |
| `"Criar grupo Finanças"` | Cria grupo "Finanças" dentro do grupo atual |
| `"Novo grupo Projetos"` | Cria grupo "Projetos" dentro do grupo atual |
| `"Adicionar nota Reunião amanhã às 14h"` | Adiciona texto à nota da entrada selecionada |
| `"Nota Lembrar de renovar certificado"` | Idem — atalho sem "adicionar" |

### Confirmação antes de executar
O sistema sempre mostra o que vai fazer antes de executar:
> → Criar entrada **GitHub** no grupo **Dev**

Use **✎ Corrigir** para editar o texto transcrito e tentar novamente.

### Modelo de transcrição
O modelo padrão é `base` (~140 MB, baixado automaticamente na primeira vez).
Para mais precisão em PT-BR, considere usar `small` ou `medium` no `AudioCaptureDialog`.

---

## 27. Verificador de Dependências

**Acesso:** Menu CfdmNote → 🔍 Verificar Dependências · aparece automaticamente na primeira execução

O verificador inspeciona todos os programas auxiliares necessários para as features do CfdmNote.

### Status por dependência
| Ícone | Significado |
|-------|-------------|
| ✅ | Instalado e funcionando |
| ⚠ | Opcional — ausente desabilita feature específica |
| ❌ | Obrigatório — feature essencial não funciona |

### Dependências verificadas
| Dependência | Feature habilitada | Instalar com |
|-------------|-------------------|--------------|
| `arecord` (alsa-utils) | Gravação de áudio pelo microfone | `sudo apt install alsa-utils` |
| `aplay` (alsa-utils) | Reproduzir áudio gravado | `sudo apt install alsa-utils` |
| `python3` | Execução de scripts e Whisper | `sudo apt install python3` |
| `openai-whisper` | Transcrição de voz → texto | `pip install openai-whisper` |
| `whisper-cli` | Transcrição alternativa (mais rápida) | Ver whisper.cpp no GitHub |
| `ffmpeg` | Conversão de formatos de áudio | `sudo apt install ffmpeg` |
| `xclip` | Área de transferência (clipboard) | `sudo apt install xclip` |
| `tesseract-ocr` | OCR — texto em imagens (feature futura) | `sudo apt install tesseract-ocr` |

### Instalar tudo de uma vez
Clique em **📋 Copiar comandos faltando** — todos os comandos necessários são copiados para o clipboard. Cole no terminal e execute.

---

## 28. Descoberta e CLI de Banco de Dados

### Argumento de linha de comando
É possível indicar qual banco abrir diretamente ao iniciar o CfdmNote:

```bash
./cfdmnote --database /caminho/para/meu_banco.kdbx
```

O caminho é salvo automaticamente para as próximas execuções.

### Descoberta automática
Se nenhum banco estiver configurado (primeira instalação ou banco movido), o CfdmNote:
1. Varre locais comuns em busca de arquivos `.kdbx` e `.ksdb`:
   - `~/`, `~/Documentos`, `~/Desktop`, `~/Dropbox`, `~/OneDrive`
   - Pendrives e HDs externos em `/media/$USER/` e `/run/media/$USER/`
2. Exibe um picker com os bancos encontrados
3. Ao selecionar, salva o caminho para uso futuro

### Opções no picker
| Botão | Ação |
|-------|------|
| Abrir selecionado | Usa o banco escolhido da lista |
| Criar novo banco | Cria um banco limpo no local padrão |
| Perguntar depois | Ignora por agora (aparece de novo se ainda não configurado) |

### Para uso com Docker ou imagem
Monte o banco como volume e passe o caminho via argumento:
```bash
docker run -v /home/usuario/banco.kdbx:/data/banco.kdbx cfdmnote \
  --database /data/banco.kdbx
```

---

## 29. Lista Completa de Funcionalidades

Todas as funcionalidades implementadas, organizadas por versão. Atualize esta seção sempre que uma feature nova for adicionada.

| Versão | Funcionalidade | Atalho |
|--------|---------------|--------|
| v1.0.x | Base KeePassXC compilável + branding CfdmNote | — |
| v1.1.0 | Banco principal auto-open (sem senha) | — |
| v1.1.0 | AppLockDialog — tela de login com senha | — |
| v1.1.0 | Recuperação por perguntas secretas | — |
| v1.1.0 | Importar CSV com barra de progresso | — |
| v1.2.0 | Lixeira dedicada fora da árvore de grupos | — |
| v1.2.0 | Ícone personalizado por entrada (PNG/GIF) | — |
| v1.2.0 | Aba "Notas" integrada no preview de entrada | Ctrl+F9 |
| v1.2.0 | GIF animado na aba Notas | — |
| v1.2.0 | Aba "Links" — Local Disk + Removable | — |
| v1.2.1 | Backup Externo para mídia removível | Ctrl+Shift+B |
| v1.3.0 | Campo "Comentário" rápido por entrada | — |
| v1.3.0 | Importar TXT (título = nome do arquivo) | — |
| v1.3.0 | Links: suporte a arquivos além de pastas | — |
| v1.4.0 | Pesquisar na web (Google/DuckDuckGo/etc.) | — |
| v1.4.0 | Exportar nota criptografada AES-256-GCM | — |
| v1.4.0 | Compartilhar via WhatsApp/Telegram/Discord | — |
| v1.5.0 | Lembretes por entrada com recorrência | Ctrl+Shift+R |
| v1.5.0 | Google Calendar integrado nos lembretes | — |
| v1.6.0 | Manual de Consulta integrado | F1 |
| v1.7.0 | Informações dos Bancos (tamanho, entradas, datas) | Ctrl+Shift+I |
| v1.7.1 | Fix: painel Notas/Links nunca colapsa | — |
| v1.7.2 | Handle do splitter melhorado | — |
| v1.8.0 | Pesquisa Poderosa: local, web, Dorking, VirusTotal | Ctrl+Shift+F |
| v1.8.1 | Botão "🤖 IA" na toolbar com menu | — |
| v1.8.1 | Tooltip rico nas entradas (hover) | — |
| v1.9.0 | Capturar Áudio e transcrever com Whisper | Ctrl+Shift+A |
| v1.9.1 | Tradução de notas via LibreTranslate | Ctrl+Shift+T |
| v1.9.1 | Pesquisar contexto da nota (palavras-chave automáticas) | — |
| v1.9.2 | ToastNotification — popup flutuante com barra regressiva | — |
| v2.0.0 | Seletor de navegador privado (Brave/LibreWolf/etc.) | — |
| v2.0.0 | Aba "📈 Em Alta" — 36 atalhos de tendências | — |
| v2.1.0 | Trending expandido: 13 regiões do mundo | — |
| v2.1.0 | Aba "🔗 Relacionados" — entradas com mesmo domínio | — |
| v2.1.0 | Aba "⏰ Lembrete" compacta no preview | — |
| v2.1.0 | Versão exibida na tela de login | — |
| v2.2.0 | Mapa Mental interativo (Mind Elixir) | — |
| v2.2.0 | Kanban/Roadmap 4 colunas | — |
| v2.2.0 | Quadro Branco (Whiteboard) com sticky notes | — |
| v2.2.0 | Tema SteelBlue + NavyBlue toolbar | — |
| v2.2.0 | Criar Grupo com seletor de pai | Ctrl+Shift+G |
| v2.2.4 | Mover Grupo com validação de ciclo | — |
| v2.3.x | Descoberta automática de bancos na máquina | — |
| v2.3.x | Argumento CLI `--database /caminho` | — |
| v2.4.0 | Assistente de Voz (criar entrada/grupo/nota por voz) | Ctrl+Shift+V |
| v2.4.2 | Verificador de Dependências no startup | — |
| **v2.4.3** | **Aba Links — Abrir com programa (context menu)** | **clique dir.** |

> 💡 Esta tabela é atualizada a cada nova funcionalidade. A versão no cabeçalho deste manual confirma que está sincronizado com a release atual.

---

## 30. Aba Links — Abrir com Programa

A aba **Links** agora suporta "Abrir com" para arquivos e pastas cadastrados.

### Como usar
1. Vá para a aba **Links** de qualquer entrada
2. Clique com o **botão direito** sobre um item da lista
3. O menu de contexto oferece:

| Opção | Ação |
|-------|------|
| 📂 Abrir | Abre com o programa padrão do sistema |
| 📁 Abrir pasta que contém | Abre o gerenciador de arquivos na pasta pai (só para arquivos) |
| ⎘ Copiar caminho | Copia o caminho completo para a área de transferência |
| 🖥 Abrir com... → `vlc` | Abre diretamente com o app detectado |
| 🖥 Abrir com... → Outro aplicativo... | Permite digitar o comando desejado |

### Apps detectados automaticamente
O CfdmNote detecta apps instalados na máquina conforme a extensão do arquivo:

| Extensão | Apps detectados |
|----------|----------------|
| `.pdf` | evince, okular, zathura |
| `.doc/.docx` | libreoffice, wps |
| `.mp4/.mkv` | vlc, mpv, totem |
| `.mp3/.flac` | vlc, mpv, rhythmbox |
| `.jpg/.png` | eog, gimp, shotwell |
| `.txt/.md` | gedit, kate, mousepad |
| `.py` | gedit, spyder, thonny |
| `.zip/.tar` | file-roller, ark |

### Lembrar o app por extensão
Quando você escolhe "Outro aplicativo..." e confirma, o app escolhido é **lembrado** para aquela extensão. Na próxima vez, ele aparece na lista de sugestões.

### Dica: Duplo clique
O duplo clique continua funcionando como antes — abre com o programa padrão do sistema (`xdg-open`).

---

*CfdmNote v2.4.3 — Desenvolvido por CFDM*
*Manual atualizado em 2026-03-24*
