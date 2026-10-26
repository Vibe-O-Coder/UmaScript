#!/bin/sh
set -eu
case "$(uname -s):$(uname -m)" in
  Linux:x86_64|Linux:amd64) ;;
  *) echo "Este pacote pronto e para Linux x86-64. Consulte README.md." >&2; exit 1 ;;
esac
umascript_package_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
chmod +x "$umascript_package_dir/bin/linux-x64/uma"
exec "$umascript_package_dir/bin/linux-x64/uma" install --package-root "$umascript_package_dir" "$@"
