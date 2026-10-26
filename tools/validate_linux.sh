#!/bin/sh
set -eu
source_root=$(pwd)
validation_root=$(mktemp -d /tmp/umascript-validate.XXXXXXXX)
cleanup() {
    case "$validation_root" in /tmp/umascript-validate.*) rm -rf -- "$validation_root";; *) exit 2;; esac
}
trap cleanup EXIT
tar -cf - src stdlib tests examples docs bin uma build.sh palavras.toml tools benchmarks README.md LICENSE | tar -xf - -C "$validation_root"
cd "$validation_root"
chmod +x bin/linux-x64/uma
python3 tests/run_tests.py bin/linux-x64/uma source
python3 tests/run_tests.py bin/linux-x64/uma bytecode
python3 tests/run_tests.py bin/linux-x64/uma executable
python3 tests/test_compiler.py bin/linux-x64/uma
python3 tests/test_v3.py bin/linux-x64/uma
python3 tests/test_v31.py bin/linux-x64/uma
python3 tests/test_lsp.py bin/linux-x64/uma
g++ -std=c++17 -O2 tests/runtime_memory.cpp -o memory-test
./memory-test
echo "Linux validation passed."
