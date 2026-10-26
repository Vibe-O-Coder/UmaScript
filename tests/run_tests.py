#!/usr/bin/env python3
"""Testes de comportamento da linguagem. Python e usado so nos testes."""
from pathlib import Path
import random
import subprocess
import sys
import tempfile
from support import temporary_directory

ROOT = Path(__file__).resolve().parents[1]
EXE = Path(sys.argv[1] if len(sys.argv) > 1 else ROOT / "bin" / ("windows-x64" if sys.platform == "win32" else "linux-x64") / ("uma.exe" if sys.platform == "win32" else "uma")).resolve()
MODE = sys.argv[2] if len(sys.argv) > 2 else 'source'
assert MODE in ('source', 'bytecode', 'executable')
COUNT = 0


def case(code, output=None, error=None, *, args=(), stdin="", check=False):
    global COUNT
    with temporary_directory() as folder:
        path = Path(folder) / "teste.uma"
        path.write_text(code, encoding="utf-8")
        cmd = [str(EXE), *args, *( ["--check"] if check else [] ), str(path)]
        result = None
        if MODE != 'source' and not check:
            out = Path(folder) / ('compiled.umac' if MODE == 'bytecode' else 'compiled.exe' if sys.platform == 'win32' else 'compiled')
            result = subprocess.run([str(EXE), 'compile' if MODE == 'bytecode' else 'build', str(path), '-o', str(out)], text=True, encoding='utf-8', capture_output=True, timeout=10)
            if result.returncode == 0:
                path.unlink()  # O artefato compilado precisa funcionar sem o fonte.
                cmd = [str(EXE), *args, str(out)] if MODE == 'bytecode' else [str(out), *args]
                result = None
        if result is None:
            result = subprocess.run(cmd, input=stdin, text=True, encoding="utf-8", capture_output=True, timeout=10)
        if error is not None:
            assert result.returncode == 1, (code, result.returncode, result.stdout, result.stderr)
            assert error.lower() in result.stderr.lower(), (code, error, result.stderr)
            assert ": erro de " in result.stderr, result.stderr
        else:
            assert result.returncode == 0, (code, result.returncode, result.stderr)
            if output is not None:
                assert result.stdout == output, (code, output, result.stdout)
        COUNT += 1


# Primitivos, precedencia, conversao numerica e escopos.
case('speed n=3; stamina d=n; guts c=\'馬\'; wit s="x"; track b=turf; bakushin(n,d,c,s,b,dirt,nature);', '3 3 馬 x turf dirt nature\n')
case('bakushin(2+3*4,(2+3)*4,-7/3,-7%3,!(turf&&dirt));', '14 20 -2 -1 turf\n')
case('speed x=1; { speed x=2; bakushin(x); } bakushin(x);', '2\n1\n')
case('speed n; stamina d; guts c; wit s; track t; speed[] a; bakushin(n,d,oguri(s),t,oguri(a)); flash(hayahide(c)==0);', '0 0 0 dirt 0\n')
case('speed x=3; bakushin(x++,++x,x--,--x,x);', '3 5 5 3 3\n')
case('stamina x=2; x+=3; x*=2; x/=4; bakushin(x);', '2.5\n')
case('speed a=0; speed b=0; a=b=7; bakushin(a,b);', '7 7\n')
case('wit a="x"+\'é\'; wit b=\'馬\'+"y"; bakushin(a,b);', 'xé 馬y\n')
case('// a\n/* b\nc */ bakushin("turf", "}", "rice");', 'turf } rice\n')
case('\ufeffbakushin("BOM");', 'BOM\n')

# Condicionais, todos os lacos e controle de fluxo.
case('speed x=0; goldship(x<5){ x++; scarlet(x==2){ urara; } scarlet(x==4){ rice; } bakushin(x); }', '1\n3\n')
case('speed s=0; kitasan(speed i=0;i<5;i++){ scarlet(i==2){ urara; } s+=i; } bakushin(s);', '8\n')
case('speed n=0; kitasan(;;){ n++; scarlet(n==3){ rice; } } bakushin(n);', '3\n')
case('scarlet(dirt){ bakushin(1); } vodka scarlet(turf){ bakushin(2); } vodka { bakushin(3); }', '2\n')
case('speed x=0; track bump(){ x++; special turf; } track a=dirt&&bump(); track b=turf||bump(); bakushin(x,a,b);', '0 dirt turf\n')
case('speed count=0; kitasan(speed i=0;i<3;i++){ kitasan(speed j=0;j<3;j++){ rice; } count++; } bakushin(count);', '3\n')
case('goldship(turf){}', error='Limite de passos', args=('--max-steps','100'))

# Funcoes, retorno, referencia a globais e recursao.
case('speed f(speed n){ scarlet(n==0){ special 1; } vodka { special n*f(n-1); }} bakushin(f(6));', '720\n')
case('bakushin(f(5)); speed f(speed x){ special x+1; }', '6\n')
case('stamina f(speed x){ special x; } bakushin(f(9));', '9\n')
case('power f(){ special; bakushin("impossivel"); } f();', '')
case('speed f(){goldship(turf){ special 7; }} bakushin(f());', '7\n')
case('speed n=0; power f(){n++;} f(); bakushin(n);', '1\n')
case('power f(){f();} f();', error='Limite de chamadas', args=('--max-depth','8'))
case('power f(){bakushin(n);} f(); speed n=1;', error='ainda nao inicializada')

# Arrays tipados, identidade, alias, indices e mutacoes durante a atribuicao.
case('speed[] a=[1,2]; speed[] b=a; falcon(b,3); a[0]=4; bakushin(a,dober(b),oguri(a),a==b);', '[4, 2] 3 2 turf\n')
case('stamina[][] m=[[],[1,2.5]]; falcon(m[0],4); m[1][0]=8; bakushin(m);', '[[4], [8, 2.5]]\n')
case('stamina[] a=[1,2]; bakushin(a[0]+0.25);', '1.25\n')
case('speed[] f(){ special []; } bakushin(f());', '[]\n')
case('speed[] a=[1]; speed push(){falcon(a,2);special 9;} a[0]=push(); bakushin(a);', '[9, 2]\n')
case('speed[] a=[1]; speed pop(){special dober(a);} a[0]=pop();', error='Array mudou')
case('speed[] a=nature; bakushin(a==nature);', 'turf\n')
case('speed[] a=nature; bakushin(oguri(a));', error='nature')
case('speed[] a=[]; dober(a);', error='vazio')
case('speed[] a=[1]; bakushin(a[-1]);', error='Indice fora')
case('speed[] a=[1]; bakushin(a[1]);', error='Indice fora')

# OOP: construtores, heranca, despacho virtual, encapsulamento e identidades.
BASE = '''
rudolf Base {
    groove speed n;
    opera(speed x){teio.n=x;}
    condor speed valor(){special teio.n;}
    condor power subir(){teio.n++;}
}
rudolf Filha diamond Base {
    condor speed extra=3;
    opera(speed x):mcqueen(x){}
    condor speed valor(){special mcqueen.valor()+teio.extra;}
}
'''
case(BASE+'Base a=tachyon Filha(7); a.subir(); bakushin(a.valor());', '11\n')
case(BASE+'Base[] xs=[tachyon Base(1),tachyon Filha(2)]; bakushin(xs[0].valor(),xs[1].valor());', '1 5\n')
case(BASE+'Base a=tachyon Base(1); Base b=a; Base c=tachyon Base(1); bakushin(a==b,a==c,a!=nature);', 'turf dirt turf\n')
case('rudolf A{condor speed[] xs=[];} A a=tachyon A(); A b=tachyon A(); falcon(a.xs,1); bakushin(oguri(a.xs),oguri(b.xs));', '1 0\n')
case('rudolf A{condor A proximo=nature; condor speed n=4;} A a=tachyon A(); a.proximo=a; bakushin(a.proximo.proximo.n);', '4\n')
case('rudolf A{condor speed n=2; condor speed outro=teio.n+3;} rudolf B diamond A {} B b=tachyon B(); bakushin(b.outro);', '5\n')
case('rudolf A{groove opera(){} condor A criar(){special tachyon A();}} A a=nature;', '')
case(BASE+'Base a=tachyon Base(1); bakushin(a.n);', error='privado')
case(BASE+'Filha a=tachyon Filha(1); a.extra="x";', error='Esperado speed')
case(BASE+'Filha a=tachyon Base(1);', error='Esperado Filha')
case(BASE+'Base[] a=[tachyon Filha(1)]; Filha[] b=a;', error='Esperado Filha[]')
case('rudolf A{groove opera(){}} A a=tachyon A();', error='privado')
case('rudolf A{condor power f(){}} rudolf B diamond A{groove power f(){}}', error='Sobrescrita')
case('rudolf A{condor speed f(){special 1;}} rudolf B diamond A{condor stamina f(){special 2.0;}}', error='Sobrescrita')
case('rudolf A{groove speed f(){special 1;}} rudolf B diamond A{condor speed f(){special 2;}}', error='privado herdado')
case('rudolf A{condor speed n;} rudolf B diamond A{condor speed n;}', error='ocultar')
case('rudolf A diamond B{} rudolf B diamond A{}', error='Ciclo')
case('rudolf A diamond Inexistente{}', error='inexistente')
case('rudolf A{opera(speed n){}} rudolf B diamond A{}', error='Declare opera')
case('rudolf A{opera(speed n){}} rudolf B diamond A{opera(){}}', error='argumentos')
case('rudolf A{opera():mcqueen(){}}', error='exige diamond')
case('rudolf A{condor speed x;} A a=nature; bakushin(a.x);', error='Acesso a nature')

# UTF-8, escapes e conversoes.
case('wit s="é馬😀"; bakushin(oguri(s),s[0],s[1],s[2]);', '3 é 馬 😀\n')
case('bakushin(ticket(hayahide(\'😀\')), ticket(0x1));', '😀 \x01\n')
case('bakushin(ticket(hayahide(\'😀\')),"\\u00e9",\'\\u99ac\');', '😀 é 馬\n')
case('bakushin(taiki("-12"),taiki(4.9),creek("2.5"),maruzensky(turf));', '-12 4 2.5 turf\n')
case('wit n=suzuka(); speed v=taiki(suzuka()); bakushin(n,v);', 'Teio 7\n', stdin='Teio\n7\n')
case('suzuka();', error='fim da entrada')
case('taiki("x");', error='nao representa')
case('creek("nan");', error='nao representa')
case('taiki(1e100);', error='fora do intervalo')
case('ticket(55296);', error='Unicode valido')
case('ticket(1114112);', error='Unicode valido')
case('wit s="x"; bakushin(s[2]);', error='Indice fora')
case('flash(dirt,"Falhou o teste");', error='Falhou o teste')

# Numeros: intervalos, precisao das comparacoes e overflow controlado.
case('speed n=-9223372036854775808; bakushin(n,n%-1,9223372036854775807);', '-9223372036854775808 0 9223372036854775807\n')
case('bakushin(9007199254740993==9007199254740992,9007199254740993==9007199254740992.0,9007199254740993>9007199254740992.0);', 'dirt dirt turf\n')
case('bakushin(-1 > -1.5, -1 < -0.5, 9223372036854775807 < 9223372036854775808.0);', 'turf turf turf\n')
for code in ('9223372036854775807+1', '-9223372036854775808-1', '3037000500*3037000500', '-9223372036854775808/-1', '-(-9223372036854775808)'):
    case('bakushin('+code+');', error='Overflow')
case('bakushin(1/0);', error='Divisao por zero')
case('bakushin(1.0/0.0);', error='Divisao por zero')
case('bakushin(1e308*1e308);', error='nao finito')
case('speed n=9223372036854775808;', error='Literal fora')
case('stamina n=1e999;', error='Literal fora')

# Rejeicao antecipada de erros de tipos, mesmo em ramos nao executados.
for code, error in [
    ('speed n="x";', 'Esperado speed'),
    ('speed n=1.2;', 'Esperado speed'),
    ('track t=1;', 'Esperado track'),
    ('scarlet(1){}', 'Esperado track'),
    ('speed f(){special "x";}', 'Esperado speed'),
    ('speed f(){scarlet(turf){special 1;}}', 'sem retornar'),
    ('power f(){special 1;}', 'sem valor'),
    ('power x;', 'Tipo invalido'),
    ('power[] x=[];', 'Tipo invalido'),
    ('power f(power x){}', 'Tipo invalido'),
    ('speed f(speed x){special x;} f();', 'argumentos'),
    ('speed f(speed x){special x;} f("x");', 'Esperado speed'),
    ('speed[] a=[1,"x"];', 'Esperado speed'),
    ('speed[] a=[1]; a[0]=turf;', 'Esperado speed'),
    ('stamina[] a=[1]; speed[] b=a;', 'Esperado speed[]'),
    ('speed[] a=[1]; speed[] b=a; falcon(b,2.5);', 'Esperado speed'),
    ('wit s="a"; s[0]=\'b\';', 'Destino'),
    ('scarlet(dirt){speed n="x";}', 'Esperado speed'),
    ('bakushin(x);', 'nao declarada'),
    ('speed n=1; speed n=2;', 'Nome duplicado'),
    ('speed f(){special 1;} speed f;', 'Nome duplicado'),
    ('special 1;', 'so pode aparecer'),
    ('rice;', 'exige um laco'),
    ('urara;', 'exige um laco'),
    ('bakushin(teio);', 'so existe'),
    ('bakushin(mcqueen);', 'exige uma classe'),
    ('oguri(1);', 'oguri exige'),
    ('falcon(1,2);', 'exige um array'),
    ('bakushin(bakushin(1));', 'imprimiveis'),
    ('speed n=1; n();', 'nao e uma funcao'),
    ('bakushin;', 'Use (...)'),
    ('bakushin(1+turf);', 'nao aceita'),
    ('bakushin(1.0%2.0);', 'nao aceita'),
    ('guts c=\'ab\';', 'exatamente um'),
    ('wit s="\\q";', 'Escape desconhecido'),
    ('/* sem fim', 'Comentario'),
    ('bakushin("sem fim);', 'sem aspas'),
    ('speed n=1', "Esperado ';'"),
    ('wit s="a"; s+=1;', 'nao aceita'),
    ('[];', 'contexto tipado'),
    ('bakushin(nature == 1);', 'incompativel'),
]:
    case(code, error=error)

case('bakushin("Nao deve imprimir");', 'Treino validado: sintaxe, tipos e bytecode corretos.\n', check=True)

# Comparacao diferencial de aritmetica inteira com resultados calculados em Python.
rng = random.Random(1042)
code = []
expected = []
for _ in range(180):
    x, y = rng.randint(-10000000,10000000), rng.randint(-10000000,10000000) or 1
    op = rng.choice(['+','-','*','/','%'])
    q = (abs(x)//abs(y)) * (-1 if (x < 0) != (y < 0) else 1)
    answer = {'+':x+y, '-':x-y, '*':x*y, '/':q, '%':x-q*y}[op]
    code.append(f'bakushin(({x}){op}({y}));')
    expected.append(str(answer))
case('\n'.join(code), '\n'.join(expected)+'\n')

# Todos os exemplos sao analisados; apenas o exemplo de entrada recebe stdin.
for path in sorted((ROOT / 'examples').glob('*.uma')):
    case(path.read_text(encoding='utf-8'), check=True)
    case(path.read_text(encoding='utf-8'), stdin='Teio\n12\n' if path.name.startswith('08') else '')

# CLI: erros de uso nao sao confundidos com erros da linguagem.
for args in ([], ['missing.uma'], ['teste.txt'], ['--max-depth','0'], ['--max-steps','-1']):
    result = subprocess.run([str(EXE), *args], capture_output=True, text=True, timeout=5)
    assert result.returncode == 2, (args, result.returncode, result.stderr)
    COUNT += 1
for args, expected in ((['--version'],'UmaScript 3.1.0'), (['--help'],'Uso:')):
    result = subprocess.run([str(EXE), *args], capture_output=True, text=True, timeout=5)
    assert result.returncode == 0 and expected in result.stdout
    COUNT += 1

print(f'{COUNT} casos passaram ({MODE}), incluindo 180 operacoes diferenciais e todos os exemplos.')
