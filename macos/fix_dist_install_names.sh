#!/bin/bash
# Rewrite Homebrew /usr/local install names on every Mach-O in the .app
# (executable, frameworks, dylibs, plugins), not only the main binary.
# Replaces a bundled Qt*.framework when it lacks the versioned binary the
# load command needs (Qt 5 Versions/5 vs Qt 6 Versions/A).
set -euo pipefail
APP="${1:-$(cd "$(dirname "$0")/.." && pwd)/dist/EiskaltDC++.app}"
python3 - "$APP" <<'PY'
import glob, shutil, stat, subprocess, sys
from pathlib import Path

app = Path(sys.argv[1])
contents = (app / 'Contents').resolve()
fw = contents / 'Frameworks'
bin_path = contents / 'MacOS' / 'EiskaltDC++'
exe_rpath = '@executable_path/../Frameworks'
MAGICS = {
    b'\xfe\xed\xfa\xce', b'\xce\xfa\xed\xfe',
    b'\xfe\xed\xfa\xcf', b'\xcf\xfa\xed\xfe',
    b'\xca\xfe\xba\xbe', b'\xbe\xba\xfe\xca',
}

def is_macho(path: Path) -> bool:
    try:
        with path.open('rb') as f:
            return f.read(4) in MAGICS
    except OSError:
        return False

def machos():
    seen = set()
    if not contents.is_dir():
        return
    for p in contents.rglob('*'):
        if not p.is_file() or p.is_symlink():
            continue
        rp = p.resolve()
        if rp in seen or not is_macho(p):
            continue
        seen.add(rp)
        yield p

def run(cmd, check=True):
    return subprocess.run(cmd, text=True, capture_output=True, check=check)

def unsign(path: Path):
    run(['codesign', '--remove-signature', str(path)], check=False)

def chmod_u(path: Path):
    path.chmod(path.stat().st_mode | stat.S_IWUSR | stat.S_IXUSR)

def otool_L(path: Path):
    deps = []
    for line in run(['otool', '-L', str(path)]).stdout.splitlines()[1:]:
        line = line.strip()
        if line:
            deps.append(line.split(' (')[0])
    return deps

def otool_id(path: Path):
    lines = [ln.strip() for ln in run(['otool', '-D', str(path)], check=False).stdout.splitlines() if ln.strip()]
    return lines[-1] if len(lines) > 1 else ''

def otool_rpaths(path: Path):
    paths, take = [], False
    for line in run(['otool', '-l', str(path)]).stdout.splitlines():
        s = line.strip()
        if s.startswith('cmd ') and 'LC_RPATH' in s:
            take = True
        elif take and s.startswith('path '):
            paths.append(s[5:].split(' (')[0])
            take = False
    return paths

def is_host_lib(p: str) -> bool:
    return p.startswith('/opt/homebrew/') or p.startswith('/usr/local/')

def is_bundle_ref(p: str) -> bool:
    return p.startswith('@executable_path/') or p.startswith('@loader_path/')

def exe_rel(path: Path) -> str:
    return '@executable_path/../' + str(path.resolve().relative_to(contents))

def resolve_dep(m: Path, load: str):
    if load.startswith('@executable_path/'):
        return bin_path.parent / load[len('@executable_path/'):]
    if load.startswith('@loader_path/'):
        return m.parent / load[len('@loader_path/'):]
    if load.startswith('/'):
        return Path(load)
    return None

def fw_parts(load: str):
    if '.framework/' not in load:
        return None
    left, right = load.split('.framework/', 1)
    return left.rsplit('/', 1)[-1], right

def bundled_fw(load: str):
    parts = fw_parts(load)
    if not parts:
        return None
    name, rest = parts
    p = fw / f'{name}.framework' / rest
    return p if p.is_file() else None

def host_fw(name: str, load: str):
    if is_host_lib(load) and '.framework/' in load:
        src = Path(load.split('.framework/')[0] + '.framework')
        if src.is_dir():
            return src
    for pat in (
        f'/opt/homebrew/opt/*/lib/{name}.framework',
        f'/opt/homebrew/lib/{name}.framework',
        f'/usr/local/opt/*/lib/{name}.framework',
        f'/usr/local/lib/{name}.framework',
    ):
        hits = glob.glob(pat)
        if hits:
            return Path(hits[0])
    return None

def replace_fw(name: str, src: Path):
    dst = fw / f'{name}.framework'
    if dst.exists():
        shutil.rmtree(dst)
    fw.mkdir(parents=True, exist_ok=True)
    shutil.copytree(src, dst, symlinks=True)

def locate(load: str):
    if fw_parts(load):
        return bundled_fw(load)
    names = [Path(load).name]
    if names[0] == 'liblua.5.5.dylib':
        names.append('liblua.5.5.0.dylib')
    for n in names:
        p = fw / n
        if p.exists():
            return p
    for m in machos():
        if m.name in names:
            return m
    return None

def copy_into_fw(load: str):
    parts = fw_parts(load)
    if parts:
        name, rest = parts
        src = host_fw(name, load)
        if src is None:
            return None
        needed = fw / f'{name}.framework' / rest
        if not needed.is_file():
            replace_fw(name, src)
        return bundled_fw(load)
    src = Path(load)
    if src.is_symlink() or src.is_file():
        src = src.resolve()
    if not src.is_file():
        return None
    fw.mkdir(parents=True, exist_ok=True)
    dest = fw / src.name
    shutil.copy2(src, dest)
    chmod_u(dest)
    return dest

def dest_for(load: str):
    found = locate(load)
    if found is not None:
        return found
    return copy_into_fw(load)

def resolve_rpath(m: Path, load: str):
    if not load.startswith('@rpath/'):
        return None
    rest = load[len('@rpath/'):]
    bases = []
    for rp in otool_rpaths(m) + [exe_rpath]:
        if rp.startswith('@executable_path/'):
            bases.append(bin_path.parent / rp[len('@executable_path/'):])
        elif rp.startswith('@loader_path/'):
            bases.append(m.parent / rp[len('@loader_path/'):])
        else:
            bases.append(Path(rp))
    bases.append(fw)
    for base in bases:
        cand = base / rest
        if cand.is_file():
            return cand
    return None

def needs_copy(m: Path, load: str, ident: str) -> bool:
    if not load or load == ident:
        return False
    if is_host_lib(load):
        return True
    if is_bundle_ref(load):
        p = resolve_dep(m, load)
        return p is None or not p.is_file()
    if load.startswith('@rpath/'):
        return resolve_rpath(m, load) is None
    return False

def tool(path: Path, args):
    unsign(path)
    chmod_u(path)
    run(['install_name_tool', *args, str(path)], check=False)

def fw_entries():
    return sum(1 for _ in fw.rglob('*')) if fw.exists() else 0

for _ in range(16):
    before = fw_entries()
    for m in list(machos()):
        ident = otool_id(m)
        for load in otool_L(m) + [ident]:
            if not needs_copy(m, load, ident):
                continue
            if dest_for(load) is None:
                print('missing', load, 'from', m)
                sys.exit(1)
    if fw_entries() == before:
        break

for m in machos():
    ident = otool_id(m)
    if ident and is_host_lib(ident):
        tool(m, ['-id', exe_rel(m)])
    for load in otool_L(m):
        if not is_host_lib(load) and not load.startswith('@rpath/'):
            continue
        dest = dest_for(load)
        if dest is None:
            print('missing', load, 'from', m)
            sys.exit(1)
        new = exe_rel(dest)
        if load != new:
            tool(m, ['-change', load, new])
    for rp in otool_rpaths(m):
        if is_host_lib(rp):
            tool(m, ['-delete_rpath', rp])

if bin_path.is_file() and exe_rpath not in otool_rpaths(bin_path):
    tool(bin_path, ['-add_rpath', exe_rpath])

left = []
broken = []
for m in machos():
    for p in otool_L(m) + [otool_id(m)] + otool_rpaths(m):
        if p and is_host_lib(p):
            left.append(f'{m}: {p}')
    for load in otool_L(m):
        if is_bundle_ref(load):
            dest = resolve_dep(m, load)
            if dest is None or not dest.is_file():
                broken.append(f'{m}: {load}')
        elif load.startswith('@rpath/') and resolve_rpath(m, load) is None:
            broken.append(f'{m}: {load}')
if left:
    print('Homebrew paths remain:')
    print('\n'.join(left))
    sys.exit(1)
if broken:
    print('Bundled load paths missing:')
    print('\n'.join(broken))
    sys.exit(1)
PY
codesign -s - --force --deep "$APP"
echo "fixed $APP"
