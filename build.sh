#!/bin/sh
# Opcional: apenas para desenvolver/recompilar o compilador.
set -eu
cd -- "$(dirname -- "$0")"
python3 tools/generate_vocabulary.py
mkdir -p bin/linux-x64
"${CXX:-g++}" -std=c++17 -O2 -Wall -Wextra -Wpedantic -static -static-libgcc -static-libstdc++ src/uma.cpp -o bin/linux-x64/uma
python3 -c "import tomllib,pathlib;pathlib.Path('bin/linux-x64/version.txt').write_text(tomllib.loads(pathlib.Path('palavras.toml').read_text())['version'])"
echo "Compilador recompilado: bin/linux-x64/uma"
