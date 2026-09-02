# CfdmNote — Guia de Compilação e Manutenção
> Use este guia quando perder acesso ao Claude Code ou precisar recompilar o projeto sozinho.

---

## 1. Onde está o projeto

| O quê | Caminho |
|---|---|
| Código fonte | `/home/cfdm/Proj-CFDM-NOTE_/` |
| Executável compilado | `/home/cfdm/Proj-CFDM-NOTE_/build/cfdmnote` |
| AppImage (portátil) | `/home/cfdm/Proj-CFDM-NOTE_/CfdmNote-2.2.0-linux-x86_64.AppImage` |
| Atalho do Desktop | `/home/cfdm/Desktop/CfdmNote.desktop` |

---

## 2. Problema: atalho do Desktop não abre o app

**Causa mais comum:** o atalho aponta para o caminho errado.

### Verificar
Abra um terminal e rode:
```bash
cat ~/Desktop/CfdmNote.desktop
```
A linha `Exec=` deve apontar para `/home/cfdm/Proj-CFDM-NOTE_/build/cfdmnote`.

### Corrigir manualmente
Abra o arquivo com um editor de texto:
```bash
gedit ~/Desktop/CfdmNote.desktop
```
Certifique-se que está assim:
```
Exec=/home/cfdm/Proj-CFDM-NOTE_/build/cfdmnote
Icon=/home/cfdm/Proj-CFDM-NOTE_/resources/icons/cfdmnote_128.png
```

### Verificar se o executável existe
```bash
ls /home/cfdm/Proj-CFDM-NOTE_/build/cfdmnote
```
- **Se aparecer o arquivo** → o app está compilado. Tente rodar direto: `./build/cfdmnote`
- **Se der "No such file or directory"** → precisa compilar (veja seção 3)

---

## 3. Compilar o projeto do zero

### Pré-requisitos (instalar uma vez)
```bash
sudo apt install -y cmake build-essential git \
  qt5-default qtwebengine5-dev qtbase5-dev qtbase5-dev-tools \
  libqt5sql5-sqlite libqt5svg5-dev libqt5x11extras5-dev \
  libbotan-2-dev libsodium-dev libargon2-dev \
  libminizip-dev zlib1g-dev libssl-dev libxi-dev libxtst-dev
```

### Compilar (modo Debug — para desenvolvimento)
```bash
cd /home/cfdm/Proj-CFDM-NOTE_
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

### Compilar (modo Release — mais rápido para usar)
```bash
cd /home/cfdm/Proj-CFDM-NOTE_
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j$(nproc)
```

### Se der erro no cmake (cache corrompido)
```bash
cd /home/cfdm/Proj-CFDM-NOTE_
rm -f build/CMakeCache.txt
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Rodar após compilar
```bash
/home/cfdm/Proj-CFDM-NOTE_/build/cfdmnote
```

---

## 4. Gerar AppImage (portátil, sem precisar compilar)

O AppImage roda em qualquer Linux sem instalar nada.

```bash
cd /home/cfdm/Proj-CFDM-NOTE_
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j$(nproc)
bash make-appimage.sh
```

O arquivo `CfdmNote-X.Y.Z-linux-x86_64.AppImage` é gerado na pasta do projeto.

Para usar:
```bash
chmod +x CfdmNote-*.AppImage
./CfdmNote-*.AppImage
```

---

## 5. Atualizar o código do GitHub

Se o código foi atualizado no GitHub e você quer baixar as novidades:
```bash
cd /home/cfdm/Proj-CFDM-NOTE_
git pull origin main
cmake --build build -j$(nproc)
```

---

## 6. Problemas comuns

### App abre mas a janela não aparece
```bash
# Matar processo e reiniciar
pkill cfdmnote
/home/cfdm/Proj-CFDM-NOTE_/build/cfdmnote
```

### Erro "libQt5WebEngine.so not found"
```bash
sudo apt install qtwebengine5-dev
```

### Erro "cannot find -lbotan-2"
```bash
sudo apt install libbotan-2-dev
```

### Build muito lento
Use `-j$(nproc)` para usar todos os núcleos do processador (já incluído nos comandos acima).

### Ver erros de compilação com mais detalhe
```bash
cmake --build build -j$(nproc) 2>&1 | grep -i error | head -20
```

---

## 7. Estrutura do projeto (resumo)

```
Proj-CFDM-NOTE_/
├── src/                  ← código C++ do CfdmNote
│   ├── main.cpp          ← ponto de entrada
│   ├── gui/              ← janelas e diálogos
│   ├── notes/            ← NoteManager (SQLite)
│   ├── reminders/        ← lembretes
│   └── db/               ← banco principal
├── resources/            ← ícones, HTML do mapa mental, manual
├── third_party/          ← KeePassXC + cmark-gfm (NÃO modificar)
├── build/                ← gerado pelo cmake (Debug)
├── build-release/        ← gerado pelo cmake (Release)
├── CMakeLists.txt        ← configuração do build
└── make-appimage.sh      ← script para gerar AppImage
```

---

## 8. Contatos e recursos

- **GitHub do projeto:** https://github.com/cfdmmarket-dot/CfdmNote
- **Versão atual:** ver `src/CfdmVersion.h`
- **Histórico de features:** `CLAUDE.md` e `DEVLOG.md`
