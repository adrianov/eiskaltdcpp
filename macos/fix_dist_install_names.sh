#!/bin/bash
# Rewrite Homebrew absolute dylib/framework paths in dist app to @executable_path/../Frameworks.
set -euo pipefail
APP="${1:-$(cd "$(dirname "$0")/.." && pwd)/dist/EiskaltDC++.app}"
BIN="$APP/Contents/MacOS/EiskaltDC++"
FW="$APP/Contents/Frameworks"
codesign --remove-signature "$BIN" 2>/dev/null || true
for fw in QtWidgets QtXml QtMultimedia QtConcurrent QtSql QtNetwork QtGui QtCore; do
  install_name_tool -change \
    "/opt/homebrew/opt/qt@5/lib/${fw}.framework/Versions/5/${fw}" \
    "@executable_path/../Frameworks/${fw}.framework/Versions/5/${fw}" \
    "$BIN" || true
done
python3 - "$BIN" "$FW" <<'PY'
import subprocess, sys
from pathlib import Path
bin_path, fw = Path(sys.argv[1]), Path(sys.argv[2])
for line in subprocess.check_output(['otool','-L',str(bin_path)], text=True).splitlines()[1:]:
    path = line.strip().split(' (')[0]
    if not path.startswith('/opt/homebrew/'):
        continue
    name = Path(path).name
    cands = [name] + (['liblua.5.5.0.dylib'] if name=='liblua.5.5.dylib' else [])
    dest = next((c for c in cands if (fw/c).exists()), None)
    if not dest:
        print('skip', path); continue
    subprocess.check_call(['install_name_tool','-change',path,f'@executable_path/../Frameworks/{dest}',str(bin_path)])
    print(dest)
PY
otool -l "$BIN" | awk '/LC_RPATH/{getline; getline; print}' | grep -q '@executable_path/../Frameworks' \
  || install_name_tool -add_rpath '@executable_path/../Frameworks' "$BIN"
codesign -s - --force --deep "$APP"
echo "fixed $APP"
