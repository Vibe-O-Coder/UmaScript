"""Verify release hashes, allowlists, and actual offline execution."""
from pathlib import Path
from hashlib import sha256
import json, os, subprocess, sys, zipfile
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tests'))
from support import temporary_directory
dist=ROOT/'dist'
for line in (dist/'SHA256SUMS.txt').read_text().splitlines():
    digest,name=line.split('  ',1)
    assert sha256((dist/name).read_bytes()).hexdigest()==digest,name
for archive in dist.glob('*3.1.0*.zip'):
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        assert not any('.test-tmp' in n or '__pycache__' in n or n.endswith('Instalar_Windows.exe') or '/.uma/' in n for n in z.namelist())
with zipfile.ZipFile(dist/'umascript-3.1.0.vsix') as z:
    assert z.testzip() is None
    assert json.loads(z.read('extension/package.json'))['version']=='3.1.0'
    assert 'extension/toolchain/stdlib/json.uma' in z.namelist()
with temporary_directory('release-offline-') as folder:
    folder=Path(folder)
    with zipfile.ZipFile(dist/'UmaScript-3.1.0-windows-x64-completo.zip') as z:
        for name in z.namelist():assert (folder/name).resolve().is_relative_to(folder.resolve())
        z.extractall(folder)
    exe=folder/'bin/windows-x64/uma.exe'
    # The parent environment intentionally has no development tools in PATH.
    env={**os.environ,'PATH':''}
    env.pop('UMA_CXX',None)
    def run(*args,expected=0):
        p=subprocess.run(list(map(str,args)),cwd=folder,env=env,capture_output=True,text=True,encoding='utf-8',timeout=180)
        assert p.returncode==expected,(args,p.returncode,p.stdout,p.stderr)
        return p
    assert '3.1.0' in run(exe,'--version').stdout
    run(exe,'check',folder/'examples/12_corrida.uma')
    sample=folder/'main.uma'
    sample.write_text('own speed[] a=[40,2];bakushin(a[0]+a[1],uma_args());',encoding='utf-8')
    native=folder/'native.exe';portable=folder/'vm.exe'
    run(exe,'build',sample,'--native','-o',native)
    run(exe,'build',sample,'-o',portable)
    sample.unlink()
    assert run(native,'馬','two words').stdout=='42 [馬, two words]\n'
    assert run(portable,'馬','two words').stdout=='42 [馬, two words]\n'
    # Prove the bundled GCC resolves its include paths inside this extraction.
    p=run(folder/'toolchain/bin/g++.exe','-print-search-dirs')
    assert str(folder).replace('\\','/') in p.stdout.replace('\\','/'),p.stdout
print('Clean archives, SHA-256, VSIX, bundled AOT with empty PATH and standalone VM/AOT passed.')
