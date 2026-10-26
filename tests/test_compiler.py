#!/usr/bin/env python3
"""Integracao da CLI, bytecode, executaveis e instalacao. Somente desenvolvimento."""
from pathlib import Path
import os
import random
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib
from support import temporary_directory

ROOT = Path(__file__).resolve().parents[1]
EXE = Path(sys.argv[1]).resolve()
COUNT = 0


def expect(ok, detail=''):
    global COUNT
    assert ok, detail
    COUNT += 1


def call(*args, env=None, stdin='', cwd=None, expected=0):
    r = subprocess.run([str(EXE), *map(str, args)], cwd=cwd, env=env, input=stdin,
                       text=True, encoding='utf-8', capture_output=True, timeout=15)
    expect(r.returncode == expected, (args, r.returncode, r.stdout, r.stderr))
    return r


with temporary_directory(prefix='uma integration ') as folder:
    temp = Path(folder)
    src = temp / 'treino acentuação.uma'
    src.write_text('''
// Esta frase deve desaparecer do bytecode: AST_NAO_E_BYTECODE_74582
rudolf Base { condor speed valor(){special 1;} }
rudolf Filha diamond Base { condor speed valor(){special 7;} }
rudolf Outra diamond Base { condor speed valor(){special 9;} }
Base a=tachyon Filha(); Base b=tachyon Outra();
speed soma=0;
kitasan(speed i=0;i<6;i++){
  { scarlet(i==2){urara;} scarlet(i==5){rice;} soma+=i; }
}
bakushin(a.valor(), b.valor(), a==b, soma, "馬ção");
''', encoding='utf-8')
    expected = '7 9 dirt 8 馬ção\n'
    expect(call('run', src).stdout == expected)
    bc = temp/'treino.umac'
    call('compile', src, '-o', bc)
    original = bc.read_bytes()
    expect(original[:8] == b'UMABC03\n')
    expect(b'AST_NAO_E_BYTECODE_74582' not in original, 'O bytecode nao pode conter o fonte')
    call('compile', src, '-o', bc)
    expect(bc.read_bytes() == original, 'Compilacao precisa ser deterministica')
    expect(call('run', bc).stdout == expected)
    asm = call('disasm', bc).stdout
    expect('JUMP_FALSE' in asm and 'CALL' in asm and 'REF_VAR' in asm)
    native = temp/('corrida.exe' if sys.platform == 'win32' else 'corrida')
    call('build', bc, '-o', native)
    expect(native.read_bytes()[-8:] == b'UMARUN03')
    src.unlink()
    bc.unlink()
    portable = temp/'pasta sem compilador'
    portable.mkdir()
    standalone = portable/native.name
    shutil.copy2(native, standalone)
    r = subprocess.run([str(standalone)], env={**os.environ, 'PATH': ''}, cwd=portable,
                       text=True, encoding='utf-8', capture_output=True, timeout=10)
    expect(r.returncode == 0 and r.stdout == expected, r.stderr)
    expect(set(portable.iterdir()) == {standalone}, 'Execucao nao deve extrair arquivos auxiliares')

    # Entradas truncadas, bytes aleatorios e corrupcao detectada por CRC.
    damaged = temp/'bad.umac'
    for content in [b'', b'UMABC03\n', original[:-1], original + b'0', original[:15] + b'x' + original[16:]]:
        damaged.write_bytes(content)
        call('run', damaged, expected=2)
    rng = random.Random(8301)
    for _ in range(64):
        data = bytearray(original)
        for _ in range(rng.randrange(1, 5)):
            data[rng.randrange(len(data)-4)] = rng.randrange(256)
        damaged.write_bytes(data)
        r = subprocess.run([str(EXE), 'run', str(damaged)], capture_output=True, timeout=5)
        expect(r.returncode == 2, (r.returncode, r.stderr))
    # CRC valido nao deve permitir execucao de um opcode desconhecido.
    data = bytearray(original)
    name_len = struct.unpack_from('<I', data, 12)[0]
    first_opcode = 16 + name_len + 4 + 4 + 4
    data[first_opcode] = 255
    data[-4:] = struct.pack('<I', zlib.crc32(data[:-4]))
    damaged.write_bytes(data)
    call('run', damaged, expected=2)

    # Criacao de projeto e protecao do fonte/compilador/diretorios de saida.
    project = temp/'meu projeto'
    call('new', project)
    expect((project/'main.uma').exists())
    call('new', project, expected=2)
    project_src = project/'main.uma'
    before = project_src.read_bytes()
    call('build', project_src, '-o', project_src, expected=2)
    expect(project_src.read_bytes() == before)
    call('build', project_src, '-o', EXE, expected=2)
    call('compile', project_src, '-o', temp, expected=2)
    call('build', project_src, '--target', 'inexistente', expected=2)
    call('check', project_src)
    call('disasm', project_src, '-o', temp/'treino.asm')
    expect((temp/'treino.asm').is_file())
    call('compile', project_src)  # Caminho padrao .umac
    expect(project_src.with_suffix('.umac').exists())

    # Compilacao cruzada: um artefato PE e um ELF, cada qual com seu runtime.
    root = EXE.parent.parent.parent if EXE.parent.name in ('linux-x64','windows-x64') else ROOT
    if (root/'bin/windows-x64/uma.exe').exists() and (root/'bin/linux-x64/uma').exists():
        for target, signature in [('windows-x64', b'MZ'), ('linux-x64', b'\x7fELF')]:
            out = temp/target
            call('build', project_src, '-o', out, '--target', target)
            expect(out.read_bytes().startswith(signature))
            expect(out.read_bytes()[-8:] == b'UMARUN03')

    if sys.platform != 'win32':
        # HOME isolado: nunca modifica os perfis reais de quem roda os testes.
        home = temp/'home de teste'
        home.mkdir()
        profile = home/'.profile'
        profile.write_text('# perfil original\nexport UMA_EXEMPLO=1\n')
        profile.chmod(0o600)
        prefix = home/"instalacao com espaco e 'aspas'"
        env = {**os.environ, 'HOME': str(home)}
        call('install', '--prefix', prefix, '--package-root', root, env=env)
        installed = prefix/'uma'
        expect(installed.exists())
        expect('UmaScript PATH' in profile.read_text())
        expect(profile.stat().st_mode & 0o777 == 0o600, 'Permissoes do perfil precisam ser preservadas')
        r = subprocess.run([str(installed), '--version'], env={**env, 'PATH': ''}, capture_output=True, text=True)
        expect(r.returncode == 0 and '3.1.0' in r.stdout)
        call('install', '--prefix', prefix, '--package-root', root, env=env)
        expect(profile.read_text().count('# >>> UmaScript PATH >>>') == 1)
        r = subprocess.run(['/bin/bash', '-c', '. "$HOME/.profile"; command -v uma; uma --version'], env=env,
                           capture_output=True, text=True)
        expect(r.returncode == 0 and str(installed) in r.stdout, r.stderr)
        unrelated = prefix/'meu codigo.uma'
        unrelated.write_text('bakushin("preservar");')
        call('uninstall', '--prefix', prefix, env=env)
        expect(unrelated.exists(), 'A desinstalacao nao pode apagar arquivos do usuario')
        expect(not installed.exists())
        expect(profile.read_text() == '# perfil original\nexport UMA_EXEMPLO=1\n')
        call('uninstall', '--prefix', home, env=env, expected=2)

print(f'{COUNT} verificacoes de integracao passaram.')
