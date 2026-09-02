#!/bin/bash
# CfdmNote — Corrigir atalho do Desktop
# Uso: bash corrigir-atalho.sh

PROJETO="$(cd "$(dirname "$0")" && pwd)"
DESKTOP="$HOME/Desktop/CfdmNote.desktop"
EXEC="$PROJETO/build/cfdmnote"
ICON="$PROJETO/resources/icons/cfdmnote_128.png"

echo "=== CfdmNote — Corrigir Atalho do Desktop ==="
echo ""

# Verificar executável
if [ ! -f "$EXEC" ]; then
    echo "AVISO: Executável não encontrado em: $EXEC"
    echo "       Rode primeiro: bash compilar.sh"
    echo ""
fi

# Criar/corrigir .desktop
cat > "$DESKTOP" <<EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=CfdmNote
Comment=Gerenciador de senhas com notas Markdown integradas
Exec=$EXEC
Icon=$ICON
Terminal=false
Categories=Utility;Security;
StartupWMClass=cfdmnote
StartupNotify=true
EOF

chmod +x "$DESKTOP"

echo "Atalho corrigido: $DESKTOP"
echo "  Exec: $EXEC"
echo "  Icon: $ICON"
echo ""

# Verificar se ícone existe
if [ ! -f "$ICON" ]; then
    echo "AVISO: Ícone não encontrado em: $ICON"
fi

echo "Pronto! Clique duas vezes no atalho do Desktop para abrir."
