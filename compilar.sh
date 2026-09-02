#!/bin/bash
# CfdmNote — Compilar e rodar
# Uso: bash compilar.sh

set -e
cd "$(dirname "$0")"

echo "=== CfdmNote — Compilando ==="

# Limpa cache se corrompido
if [ -f build/CMakeCache.txt ]; then
    CACHED=$(grep 'CMAKE_HOME_DIRECTORY' build/CMakeCache.txt 2>/dev/null | cut -d= -f2)
    CURRENT=$(pwd)
    if [ "$CACHED" != "$CURRENT" ]; then
        echo ">> Cache corrompido detectado. Limpando..."
        rm -f build/CMakeCache.txt
    fi
fi

echo ">> Configurando cmake..."
cmake -B build -DCMAKE_BUILD_TYPE=Release -Wno-dev

echo ">> Compilando com $(nproc) núcleos..."
cmake --build build -j$(nproc)

echo ""
echo "=== Compilado com sucesso! ==="
echo "Executável: $(pwd)/build/cfdmnote"
echo ""

read -p "Deseja rodar o CfdmNote agora? [s/N] " resp
if [[ "$resp" =~ ^[sS]$ ]]; then
    ./build/cfdmnote &
fi
