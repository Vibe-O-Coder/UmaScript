"""End-to-end tests: modules, native lowering, standard library and editor protocol."""
from pathlib import Path
import json
import os
import subprocess
import sys
from support import temporary_directory

ROOT = Path(__file__).resolve().parents[1]
EXE = Path(sys.argv[1]).resolve()
COUNT = 0
os.environ['UMA_TEST_UTF8'] = '馬ção😀'

def expect(value, detail=''):
    global COUNT
    assert value, detail
    COUNT += 1

def call(*args, stdin='', expected=0, timeout=120):
    p = subprocess.run([str(EXE), *map(str, args)], input=stdin, text=True, encoding='utf-8', errors='replace', capture_output=True, timeout=timeout, cwd=globals().get('temp', ROOT))
    expect(p.returncode == expected, (args, p.returncode, p.stdout, p.stderr))
    return p

with temporary_directory('v3 native espaço ') as folder:
    temp = Path(folder)
    src = temp / 'main.uma'
    native = temp / ('corrida.exe' if os.name == 'nt' else 'corrida')
    def compare(code, stdin='', expected=None):
        src.write_text(code, encoding='utf-8')
        vm = call('run', src, stdin=stdin)
        call('build', src, '--native', '-o', native)
        # Run with no compiler in PATH and without source.
        src.unlink()
        p = subprocess.run([str(native)], input=stdin, capture_output=True, encoding='utf-8', timeout=30, cwd=temp, env={**os.environ, 'PATH': ''})
        expect(p.returncode == 0, p.stderr)
        expect(p.stdout == vm.stdout, (vm.stdout, p.stdout))
        if expected is not None: expect(p.stdout == expected, p.stdout)
        print('native differential case passed', flush=True)

    compare('''
speed x=3; bakushin(x++,++x,x--,--x,x);
bakushin(010,008,hayahide('\0'),oguri("x\0y"));
stamina[] reals=[1,2,3.5]; falcon(reals,4); bakushin(reals);
track[] flags=[turf,dirt]; flags[1]=turf; bakushin(flags);
speed[][] a=[[1,2],[3]]; speed[] alias=a[0]; falcon(alias,4); a[0][0]+=5; bakushin(a);
bakushin(9007199254740993==9007199254740992.0,9007199254740993>9007199254740992.0);
bakushin(-1 > -1.5, -1 < -0.5, 9223372036854775807 < 9223372036854775808.0);
bakushin(1<<63,-1>>63,~0,8|3,7&3,7^3); speed mask=1; mask<<=8; mask|=3; bakushin(mask);
wit unicode="馬éx"; bakushin(unicode[0],oguri(unicode),hayahide(unicode[1]),ticket(0));
speed fact(speed n){scarlet(n==0){special 1;}special n*fact(n-1);}bakushin(fact(10));
speed sum=0;kitasan(speed i=0;i<10;i++){scarlet(i==2){urara;}scarlet(i==8){rice;}sum+=i;}bakushin(sum);
''')
    compare((ROOT/'examples/04_classes.uma').read_text(encoding='utf-8'))
    compare((ROOT/'examples/10_biblioteca_padrao.uma').read_text(encoding='utf-8'))
    compare((ROOT/'tests/stdlib/core.test.uma').read_text(encoding='utf-8'))
    compare(r'''
import "std/io";import "std/text";import "std/system";
flash(environment("UMA_TEST_UTF8") == "馬ção😀");
writeText("texto.txt","馬é\n");appendText("texto.txt","x");
flash(exists("texto.txt"));bakushin(readText("texto.txt"));
speed[] b=[0,1,127,128,255];writeBytes("dados.bin",b);bakushin(readBytes("dados.bin"));
bakushin(trim("  馬\t"),split("a,,b,",","),startsWith("馬é","馬"),endsWith("馬é","é"));
''')
    compare('''
rudolf Node { condor Node next; condor speed value=7; }
Node a=tachyon Node();a.next=a;bakushin(a,a.next.value);
rudolf Base { condor speed read(){special 2;} }
rudolf Child diamond Base { condor speed read(){special mcqueen.read()+5;} }
Base b=tachyon Child();Base c=nature;bakushin(b.read(),c==nature);
bakushin(suzuka());
''', stdin='Teio 馬\n')

    # Checked native runtime errors match VM failure behavior.
    for code, message in [
        ('bakushin(9223372036854775807+1);', 'Overflow'),
        ('speed[] a=[1]; bakushin(a[2]);', 'limites'),
        ('rudolf C{condor speed n;} C a=nature; bakushin(a.n);', 'nature'),
        ('bakushin(1<<64);', 'Deslocamento'),
        ('bakushin(uma_sqrt(-1));', 'finito'),
    ]:
        src.write_text(code, encoding='utf-8')
        expect(message.lower() in call('run', src, expected=1).stderr.lower())
        call('build', src, '--native', '-o', native)
        p = subprocess.run([str(native)], capture_output=True, encoding='utf-8', timeout=20)
        expect(p.returncode == 1 and message.lower() in p.stderr.lower(), p.stderr)
        print('native runtime error case passed', flush=True)

    # Diamond import deduplication, forward declarations and cyclic imports.
    (temp/'shared.uma').write_text('speed f(){special 9;}', encoding='utf-8')
    (temp/'a.uma').write_text('import "shared";', encoding='utf-8')
    (temp/'b.uma').write_text('import "shared";', encoding='utf-8')
    src.write_text('import "a";import "b";bakushin(f());', encoding='utf-8')
    expect(call('run', src).stdout == '9\n')
    preserved = (temp/'shared.uma').read_bytes()
    call('build', src, '--native', '-o', temp/'shared.uma', expected=2)
    call('compile', src, '-o', temp/'shared.uma', expected=2)
    expect((temp/'shared.uma').read_bytes() == preserved)
    call('build', src, '--native', '-o', native)
    expect(subprocess.check_output([str(native)], text=True) == '9\n')
    (temp/'shared.uma').write_text('import "a";', encoding='utf-8')
    expect('ciclica' in call('check', src, expected=1).stderr)

    # JSON protocol validates unsaved buffers and resolves imported locations.
    src.write_text('bakushin("saved");', encoding='utf-8')
    result = json.loads(call('check', src, '--stdin', '--json', stdin='speed n="bad";', expected=1).stdout)
    expect(result['diagnostics'][0]['line'] == 1)
    expect('Esperado speed' in result['diagnostics'][0]['message'])
    result = json.loads(call('inspect', src, '--stdin', stdin='import "std/math"; speed n=abs(-7);').stdout)
    expect(any(s['name']=='abs' and s['file'].endswith('math.uma') for s in result['symbols']))
    expect(any(s['name']=='n' and s['type']=='speed' for s in result['symbols']))
    (temp/'invalid.uma').write_text('speed n="bad";', encoding='utf-8')
    result = json.loads(call('check', src, '--json', '--stdin', stdin='import "invalid";', expected=1).stdout)
    expect(result['diagnostics'][0]['file'].endswith('invalid.uma'))
    src.write_text('bakushin(42);', encoding='utf-8')
    cpp = temp/'exportado.cpp'
    call('emit-cpp', src, '-o', cpp)
    expect('std::int64_t' in cpp.read_text() and 'enum class Op' not in cpp.read_text())
    before = src.read_bytes()
    call('build', src, '--native', '-o', src, expected=2)
    expect(src.read_bytes() == before)
    call('build', src, '--native', '--target', 'linux-x64' if os.name == 'nt' else 'windows-x64', expected=2)
    # An arbitrary nonexistent compiler leaves the previous artifact untouched.
    before = native.read_bytes()
    call('build', src, '--native', '--cc', str(temp/'missing-compiler'), '-o', native, expected=2)
    expect(native.read_bytes() == before)
    testdir = temp/'language-tests'
    testdir.mkdir()
    (testdir/'ok.test.uma').write_text('flash(2+2==4);', encoding='utf-8')
    expect('1 passaram; 0 falharam' in call('test', testdir).stdout)
    (testdir/'fail.test.uma').write_text('flash(dirt);', encoding='utf-8')
    expect('1 passaram; 1 falharam' in call('test', testdir, expected=1).stdout)

print(f'{COUNT} verificacoes UmaScript 3.0 passaram.')
