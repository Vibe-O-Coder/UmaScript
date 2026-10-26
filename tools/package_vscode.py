"""Build an offline VSIX and a Windows source/toolchain distribution with stdlib."""
from pathlib import Path
from hashlib import sha256
import json
import subprocess
import zipfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
EXT = ROOT/'vscode-extension'
DIST = ROOT/'dist'
DIST.mkdir(exist_ok=True)
manifest = json.loads((EXT/'package.json').read_text(encoding='utf-8'))
version = manifest['version']
binary = ROOT/'bin/windows-x64/uma.exe'
if (binary.parent/'version.txt').read_text().strip() != version:
    raise RuntimeError('Rebuild the Windows compiler before packaging')
if version not in subprocess.check_output([str(binary), '--version'], encoding='utf-8'):
    raise RuntimeError('The compiler binary is stale')

def files(directory):
    return sorted(p for p in directory.rglob('*') if p.is_file() and not any(x in {'node_modules','__pycache__','toolchain'} for x in p.relative_to(directory).parts))

xml = f'''<?xml version="1.0" encoding="utf-8"?>
<PackageManifest Version="2.0.0" xmlns="http://schemas.microsoft.com/developer/vsx-schema/2011">
  <Metadata>
    <Identity Language="pt-BR" Id="umascript" Version="{version}" Publisher="umascript-language"/>
    <DisplayName>UmaScript</DisplayName>
    <Description xml:space="preserve">UmaScript: IntelliSense, documentacao tecnica e compilacao nativa.</Description>
    <Tags>umascript,language,intellisense</Tags>
    <Categories>Programming Languages,Linters,Snippets</Categories>
    <Properties>
      <Property Id="Microsoft.VisualStudio.Code.Engine" Value="^1.85.0"/>
      <Property Id="Microsoft.VisualStudio.Code.ExtensionKind" Value="workspace"/>
      <Property Id="Microsoft.VisualStudio.Code.ExecutesCode" Value="true"/>
      <Property Id="Microsoft.VisualStudio.Services.GitHubFlavoredMarkdown" Value="true"/>
    </Properties>
    <License>extension/LICENSE</License>
  </Metadata>
  <Installation><InstallationTarget Id="Microsoft.VisualStudio.Code"/></Installation>
  <Dependencies/>
  <Assets>
    <Asset Type="Microsoft.VisualStudio.Code.Manifest" Path="extension/package.json" Addressable="true"/>
    <Asset Type="Microsoft.VisualStudio.Services.Content.Details" Path="extension/README.md" Addressable="true"/>
    <Asset Type="Microsoft.VisualStudio.Services.Content.License" Path="extension/LICENSE" Addressable="true"/>
  </Assets>
</PackageManifest>'''
types = '''<?xml version="1.0" encoding="utf-8"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
 <Default Extension="json" ContentType="application/json"/>
 <Default Extension="js" ContentType="application/javascript"/>
 <Default Extension="md" ContentType="text/markdown"/>
 <Default Extension="txt" ContentType="text/plain"/>
 <Default Extension="uma" ContentType="text/plain"/>
 <Default Extension="hpp" ContentType="text/plain"/>
 <Default Extension="exe" ContentType="application/octet-stream"/>
 <Default Extension="xml" ContentType="application/xml"/>
 <Default Extension="vsixmanifest" ContentType="text/xml"/>
 <Override PartName="/extension/LICENSE" ContentType="text/plain"/>
</Types>'''
ET.fromstring(xml); ET.fromstring(types)
vsix = DIST/f'umascript-{version}.vsix'
with zipfile.ZipFile(vsix, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    z.writestr('extension.vsixmanifest', xml)
    z.writestr('[Content_Types].xml', types)
    for p in files(EXT):
        if not any(x in {'test','.vscode'} for x in p.relative_to(EXT).parts): z.write(p, 'extension/'+p.relative_to(EXT).as_posix())
    z.write(ROOT/'LICENSE', 'extension/LICENSE')
    z.write(binary, 'extension/toolchain/bin/windows-x64/uma.exe')
    z.write(binary.parent/'version.txt', 'extension/toolchain/bin/windows-x64/version.txt')
    for directory in ('src', 'stdlib', 'docs'):
        for p in files(ROOT/directory): z.write(p, 'extension/toolchain/'+p.relative_to(ROOT).as_posix())
    # Examples let the bundled compiler also recognize its package root directly.
    for p in sorted((ROOT/'examples').glob('*.uma')): z.write(p, 'extension/toolchain/'+p.relative_to(ROOT).as_posix())
with zipfile.ZipFile(vsix) as z:
    assert z.testzip() is None
    assert json.loads(z.read('extension/package.json'))['version'] == version
    assert z.read('extension/toolchain/bin/windows-x64/uma.exe')[:2] == b'MZ'

checksums = DIST/'SHA256SUMS.txt'
lines=[]
if checksums.exists():
    lines=[line for line in checksums.read_text().splitlines() if '3.1.0' in line and not line.endswith(vsix.name)]
lines.append(f'{sha256(vsix.read_bytes()).hexdigest()}  {vsix.name}')
checksums.write_text('\n'.join(lines)+'\n',encoding='ascii')
print(vsix)
