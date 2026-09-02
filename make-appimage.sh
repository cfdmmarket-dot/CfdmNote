#!/bin/bash
# make-appimage.sh — gera CfdmNote-x86_64.AppImage
# Uso: bash make-appimage.sh
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build-release"
APPDIR="${SCRIPT_DIR}/AppDir"
TOOLS_DIR="${SCRIPT_DIR}/.appimage-tools"
VERSION=$(grep 'CFDMNOTE_VERSION' "${SCRIPT_DIR}/src/CfdmVersion.h" | grep -oP '"\K[^"]+')

echo "=== CfdmNote AppImage Builder v${VERSION} ==="
echo ""

# ── 1. Verifica que o binário existe ─────────────────────────────────────────
if [ ! -f "${BUILD_DIR}/cfdmnote" ]; then
    echo "[!] Binário não encontrado. Execute primeiro:"
    echo "    cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j\$(nproc)"
    exit 1
fi
echo "[✓] Binário: ${BUILD_DIR}/cfdmnote"

# ── 2. Baixa ferramentas se necessário ───────────────────────────────────────
mkdir -p "${TOOLS_DIR}"

LINUXDEPLOY="${TOOLS_DIR}/linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_QT="${TOOLS_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage"
APPIMAGETOOL="${TOOLS_DIR}/appimagetool-x86_64.AppImage"

if [ ! -f "${LINUXDEPLOY}" ]; then
    echo "[↓] Baixando linuxdeploy..."
    wget -q --show-progress -O "${LINUXDEPLOY}" \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod +x "${LINUXDEPLOY}"
fi

if [ ! -f "${LINUXDEPLOY_QT}" ]; then
    echo "[↓] Baixando linuxdeploy-plugin-qt..."
    wget -q --show-progress -O "${LINUXDEPLOY_QT}" \
        "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
    chmod +x "${LINUXDEPLOY_QT}"
fi

if [ ! -f "${APPIMAGETOOL}" ]; then
    echo "[↓] Baixando appimagetool..."
    wget -q --show-progress -O "${APPIMAGETOOL}" \
        "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
    chmod +x "${APPIMAGETOOL}"
fi

echo "[✓] Ferramentas OK"

# ── 3. Monta AppDir ───────────────────────────────────────────────────────────
echo ""
echo "[→] Montando AppDir..."
rm -rf "${APPDIR}"
mkdir -p "${APPDIR}/usr/bin"
mkdir -p "${APPDIR}/usr/share/applications"
mkdir -p "${APPDIR}/usr/share/icons/hicolor/256x256/apps"
mkdir -p "${APPDIR}/usr/share/icons/hicolor/scalable/apps"
mkdir -p "${APPDIR}/usr/share/keepassxc"

# Binário
cp "${BUILD_DIR}/cfdmnote" "${APPDIR}/usr/bin/cfdmnote"

# Ícones
cp "${SCRIPT_DIR}/resources/icons/cfdmnote_256.png" \
   "${APPDIR}/usr/share/icons/hicolor/256x256/apps/cfdmnote.png"
cp "${SCRIPT_DIR}/resources/icons/cfdmnote.svg" \
   "${APPDIR}/usr/share/icons/hicolor/scalable/apps/cfdmnote.svg"

# Cópia raiz para AppImage (exigido pelo formato)
cp "${SCRIPT_DIR}/resources/icons/cfdmnote_256.png" "${APPDIR}/cfdmnote.png"

# .desktop
cp "${SCRIPT_DIR}/package/cfdmnote.desktop" "${APPDIR}/usr/share/applications/cfdmnote.desktop"
cp "${SCRIPT_DIR}/package/cfdmnote.desktop" "${APPDIR}/cfdmnote.desktop"

# AppRun personalizado
cp "${SCRIPT_DIR}/package/AppRun" "${APPDIR}/AppRun"
chmod +x "${APPDIR}/AppRun"

# Recursos do KeePassXC (traduções, ícones de tema, wordlists)
KPXC_SHARE="${SCRIPT_DIR}/third_party/keepassxc/share"
if [ -d "${KPXC_SHARE}/translations" ]; then
    cp -r "${KPXC_SHARE}/translations" "${APPDIR}/usr/share/keepassxc/"
fi
if [ -d "${KPXC_SHARE}/icons" ]; then
    cp -r "${KPXC_SHARE}/icons" "${APPDIR}/usr/share/keepassxc/"
fi
if [ -d "${KPXC_SHARE}/wordlists" ]; then
    cp -r "${KPXC_SHARE}/wordlists" "${APPDIR}/usr/share/keepassxc/"
fi

echo "[✓] AppDir montado"

# ── 4. linuxdeploy: resolve .so + plugins Qt ─────────────────────────────────
echo ""
echo "[→] Resolvendo dependências e plugins Qt..."
export APPDIR="${APPDIR}"
export OUTPUT="${SCRIPT_DIR}/CfdmNote-${VERSION}-linux-x86_64.AppImage"

# --appimage-extract-and-run: não precisa de FUSE no host
"${LINUXDEPLOY}" \
    --appimage-extract-and-run \
    --appdir "${APPDIR}" \
    --executable "${APPDIR}/usr/bin/cfdmnote" \
    --plugin qt \
    2>&1 | grep -v "^$" || true

echo "[✓] Dependências resolvidas"

# ── 5. Cria o AppImage final ──────────────────────────────────────────────────
echo ""
echo "[→] Gerando AppImage..."
# --appimage-extract-and-run: não precisa de FUSE instalado no host
ARCH=x86_64 "${APPIMAGETOOL}" \
    --appimage-extract-and-run \
    --no-appstream \
    "${APPDIR}" \
    "${OUTPUT}" 2>&1

echo ""
echo "╔══════════════════════════════════════════════════════╗"
echo "║  AppImage gerado com sucesso!                        ║"
echo "║  $(basename "${OUTPUT}")"
echo "║                                                      ║"
echo "║  Para executar:                                      ║"
echo "║    chmod +x CfdmNote-${VERSION}-linux-x86_64.AppImage     ║"
echo "║    ./CfdmNote-${VERSION}-linux-x86_64.AppImage            ║"
echo "╚══════════════════════════════════════════════════════╝"
