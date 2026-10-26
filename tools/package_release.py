"""Build clean portable releases from an allowlist, including an optional C++ toolchain."""
from pathlib import Path
import argparse, hashlib, json, os, re, shutil, subprocess, zipfile
ROOT=Path(__file__).resolve().parents[1]
def stage_toolchain(source,destination):
    source=source.resolve();destination=destination.resolve()
    if not (source/'bin/g++.exe').is_file():raise RuntimeError('Toolchain g++ missing')
    if not destination.is_relative_to(ROOT):raise RuntimeError('Toolchain output escaped project')
    def copy(p):
        dest=destination/p.relative_to(source);dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,dest)
    selected=[]
    for directory in ('include','lib/gcc'):
        selected.extend(p for p in (source/directory).rglob('*') if p.is_file())
    selected.extend(p for p in (source/'lib').iterdir() if p.is_file() and p.suffix in ('.a','.o'))
    selected.extend(p for p in (source/'share/licenses').rglob('*') if p.is_file())
    selected.extend(source/'bin'/name for name in ('g++.exe','gcc.exe','as.exe','ld.exe','ar.exe','nm.exe','strip.exe'))
    pending=[p for p in selected if p.suffix in ('.exe','.dll')];seen=set()
    env={**os.environ,'PATH':str(source/'bin')+os.pathsep+os.environ.get('PATH','')}
    while pending:
        binary=pending.pop()
        if binary in seen:continue
        seen.add(binary)
        output=subprocess.check_output([str(source/'bin/objdump.exe'),'-p',str(binary)],env=env,text=True,errors='replace')
        for name in re.findall(r'DLL Name:\s*(\S+)',output):
            dependency=source/'bin'/name
            if dependency.is_file() and dependency not in selected:selected.append(dependency);pending.append(dependency)
    for p in sorted(set(selected)):copy(p)
    notice='''This directory contains the MSYS2 UCRT64 GCC/MinGW toolchain.
It is optional for running UmaScript bytecode; included for offline AOT compilation.
Licenses are preserved in share/licenses/ and the UmaScript docs/licenses/ directory.
Corresponding upstream source archives and build recipes:
GCC 15.2.0: https://ftp.gnu.org/gnu/gcc/gcc-15.2.0/
MSYS2 package recipes: https://github.com/msys2/MINGW-packages
MinGW-w64: https://www.mingw-w64.org/downloads/
Binutils: https://ftp.gnu.org/gnu/binutils/
Installed package metadata is recorded in SOURCE-PACKAGES.txt.
'''
    (destination/'README.txt').write_text(notice,encoding='utf-8')
    database=source.parent/'var/lib/pacman/local'
    descriptions=[]
    for p in sorted(database.glob('mingw-w64-ucrt-x86_64-*/desc')):
        descriptions.append(p.read_text(encoding='utf-8'))
    (destination/'SOURCE-PACKAGES.txt').write_text('\n'.join(descriptions),encoding='utf-8')

def archive(platform,include_toolchain):
    version=json.loads((ROOT/'vscode-extension/package.json').read_text())['version']
    exe=ROOT/'bin'/platform/('uma.exe' if platform.startswith('windows') else 'uma')
    if (exe.parent/'version.txt').read_text().strip()!=version:raise RuntimeError(f'Stale {platform} runtime')
    suffix='completo' if include_toolchain else 'portatil'
    dest=ROOT/'dist'/f'UmaScript-{version}-{platform}-{suffix}.zip'
    files=[]
    for name in ('README.md','LICENSE','palavras.toml','uma','uma.cmd','instalar-linux.sh','instalar-windows.cmd','build.sh','build.bat','Makefile'):
        files.append(ROOT/name)
    for name in ('src','stdlib','docs','examples','tests','benchmarks','vscode-extension','tools','.github'):
        files.extend(p for p in (ROOT/name).rglob('*') if p.is_file() and not any(part in {'.test-tmp','__pycache__','node_modules','results'} for part in p.relative_to(ROOT).parts) and not p.name.startswith(('upgrade_','finalize_')) and p.suffix not in {'.exe','.pyc','.o','.obj'})
    for target in ('windows-x64','linux-x64'):
        runtime=ROOT/'bin'/target/('uma.exe' if target=='windows-x64' else 'uma');stamp=runtime.parent/'version.txt'
        if runtime.is_file() and stamp.is_file() and stamp.read_text().strip()==version:files.extend((runtime,stamp))
    benchmark=ROOT/'benchmarks/results/suite-3.1.json'
    if benchmark.is_file():files.append(benchmark)
    if include_toolchain:files.extend(p for p in (ROOT/'toolchain').rglob('*') if p.is_file())
    dest.parent.mkdir(exist_ok=True)
    with zipfile.ZipFile(dest,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for p in sorted(set(files)):
            name=p.relative_to(ROOT).as_posix();info=zipfile.ZipInfo(name,(2026,1,1,0,0,0));info.create_system=3;info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=(0o755 if name in {'uma','build.sh','instalar-linux.sh','bin/linux-x64/uma'} else 0o644)<<16
            z.writestr(info,p.read_bytes())
    with zipfile.ZipFile(dest) as z:
        assert z.testzip() is None
        assert not any('.test-tmp' in n or '__pycache__' in n or n.endswith('Instalar_Windows.exe') for n in z.namelist())
    print(dest,flush=True)
    return dest

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--toolchain-root',type=Path);parser.add_argument('--include-toolchain',action='store_true');parser.add_argument('--platform',default='windows-x64',choices=['windows-x64','linux-x64']);args=parser.parse_args()
    if args.toolchain_root:stage_toolchain(args.toolchain_root,ROOT/'toolchain')
    if args.include_toolchain and not (ROOT/'toolchain/bin/g++.exe').is_file():raise RuntimeError('Stage the bundled toolchain first')
    outputs=[archive(args.platform,bool(args.toolchain_root) or args.include_toolchain)]
    if args.platform=='windows-x64' and (args.toolchain_root or args.include_toolchain):outputs.append(archive(args.platform,False))
    sums=ROOT/'dist/SHA256SUMS.txt';existing={}
    if sums.exists():
        for line in sums.read_text().splitlines():
            digest,name=line.split('  ',1)
            if (ROOT/'dist'/name).is_file() and '3.1.0' in name:existing[name]=digest
    for p in outputs:existing[p.name]=hashlib.sha256(p.read_bytes()).hexdigest()
    sums.write_text(''.join(f'{digest}  {name}\n' for name,digest in sorted(existing.items())),encoding='ascii')
