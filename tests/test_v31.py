"""Differential and rejection tests for 3.1 (no test dependencies)."""
from pathlib import Path
import os, subprocess, sys
from support import temporary_directory
ROOT=Path(__file__).resolve().parents[1]
EXE=Path(sys.argv[1]).resolve()
COUNT=0
def call(args,stdin='',expected=0):
    global COUNT
    r=subprocess.run(list(map(str,args)),input=stdin,capture_output=True,text=True,encoding='utf-8',timeout=120)
    assert r.returncode==expected,(args,r.returncode,r.stdout,r.stderr)
    COUNT+=1
    return r
with temporary_directory('v31-') as folder:
    temp=Path(folder);src=temp/'main.uma';bc=temp/'main.umac';native=temp/('main.exe' if os.name=='nt' else 'main')
    def compare(code,output,stdin=''):
        src.write_text(code,encoding='utf-8')
        assert call([EXE,'run',src],stdin).stdout==output
        call([EXE,'compile',src,'-o',bc])
        assert call([EXE,'run',bc],stdin).stdout==output
        call([EXE,'build',src,'--native','-o',native])
        assert call([native],stdin).stdout==output
        print('3.1 differential passed',flush=True)
    compare('''
own speed[] a=[1,2];
{borrow speed[] b=&a; bakushin(b[1]);}
{borrow mut speed[] b=&mut a;falcon(b,3);b[0]=7;}
own speed[] c=mover(a);bakushin(c);
power inc(borrow mut speed[] a){a[0]++;}
inc(&mut c);bakushin(c);
''','2\n[7, 2, 3]\n[8, 2, 3]\n')
    compare('''
speed f(){treino{special 7;}desaquecer{bakushin("finally");}}
treino{bakushin(1/0);}tropeco(wit e){bakushin(e);}desaquecer{bakushin("done");}
bakushin(f());
treino{desclassificar "erro";}tropeco(wit e){bakushin(e);}
''','Divisao por zero.\ndone\nfinally\n7\nerro\n')
    compare('''
speed doubleIt(speed n){special n*2;}
tatica(speed)->speed f=doubleIt;bakushin(f(9));
speed offset=7;
tatica(speed)->speed add=tatica(speed n)->speed{special n+offset;};
offset=100;bakushin(add(3));
speed[] a=[1,9,2];ordenar(a,tatica(speed x,speed y)->track{special x>y;});bakushin(a);
''','18\n10\n[9, 2, 1]\n')
    compare('''wit s="";kitasan(speed i=0;i<20000;i++){s+="x";}wit t=s;s+="馬😀";bakushin(oguri(t),oguri(s),s[20000],s[20001]);''','20000 20002 馬 😀\n')
    compare('''wit s="a";wit change(){s="z";special "b";}s+=change();bakushin(s);''','ab\n')
    compare('''rudolf C{condor speed n=7;condor tatica()->speed f;} C c=tachyon C();c.f=tatica()->speed{special c.n;};uma_collect();bakushin(c.f());''','7\n')
    compare('''speed n=0;goldship(!uma_eof()){wit s=suzuka();n+=taiki(s);}bakushin(n,uma_replace("banana","na","!"),uma_hash("key")==uma_hash("key"));''','6 ba!! turf\n','1\n2\n3\n')
    compare('''
import "std/collections";
Lista<wit> l=tachyon Lista<wit>();l.push("a");l.push("b");bakushin(l.get(1),l.size());
Mapa<wit,speed> m=tachyon Mapa<wit,speed>();
kitasan(speed i=0;i<1000;i++){m.set(maruzensky(i),i);}
kitasan(speed i=0;i<500;i++){flash(m.remove(maruzensky(i)));}
kitasan(speed i=500;i<1000;i++){flash(m.get(maruzensky(i))==i);}
bakushin(m.size(),m.has("0"),m.getOr("0",-1));
Conjunto<speed> s=tachyon Conjunto<speed>();s.add(3);s.add(3);bakushin(s.size());
T identity<T>(T x){special x;}bakushin(identity<speed>(42));
''','b 2\n500 dirt -1\n1\n42\n')
    compare('''
estilo Corredor {speed ritmo();}
rudolf Velocista adota Corredor {condor speed ritmo(){special 1000;}}
Corredor c=tachyon Velocista();bakushin(c.ritmo());
speed soma=0;speed[] a=[1,2,3];kitasan(speed x em a){soma+=x;}bakushin(soma);
kitasan(guts c em "馬😀"){bakushin(c);}
speed x=0xFF,y=0b1010,z=1_000;bakushin(x,y,z,9223372036854775807+%1);
''','1000\n6\n馬\n😀\n255 10 1000 -9223372036854775808\n')
    compare('''
speed n=0;kitasan(speed i=0;i<5;i++){
treino{scarlet(i==1){urara;}scarlet(i==3){rice;}n+=i;}
desaquecer{bakushin(i);}}
bakushin(n);
treino{treino{desclassificar "first";}desaquecer{desclassificar "second";}}
tropeco(wit e){bakushin(e);}
''','0\n1\n2\n3\n2\nsecond\n')
    (temp/'a.uma').write_text('speed value(){special 7;}',encoding='utf-8')
    (temp/'b.uma').write_text('speed value(){special 9;}',encoding='utf-8')
    compare('convocar "a" como a;convocar "b" como b;bakushin(a.value(),b.value());','7 9\n')
    compare('int x=3;while(x>0){printDummy(x);x--;}void printDummy(int n){bakushin(n);}','3\n2\n1\n')
    compare(r'''
import "std/json";import "std/base64";import "std/csv";
Json j=parseJson("{\"nome\":\"Uma\",\"n\":42,\"a\":[true,null]}");
bakushin(j.get("nome").asText(),j.get("n").asInt(),j.get("a").at(0).asBool());
j.set("nome",jsonText("馬😀"));bakushin(j.get("nome").asText());
bakushin(base64Encode([0,1,254,255]),base64Decode("AAH+/w=="));
wit[][] rows=parseCsv("a,\"b,b\",\"c\"\"d\"\r\n");bakushin(rows);
flash(parseCsv(writeCsv(rows))[0][2]=="c\"d");
treino{parseJson("{bad}");}tropeco(wit e){bakushin("json error caught");}
''','Uma 42 turf\n馬😀\nAAH+/w== [0, 1, 254, 255]\n[[a, b,b, c"d]]\njson error caught\n')
    src.write_text('// if class inside comments\nint n=1;\nif(n==1){\nbakushin("if class");\n}\n',encoding='utf-8')
    call([EXE,'fmt',src,'--dialeto=uma']);formatted=src.read_text(encoding='utf-8')
    assert '// if class inside comments' in formatted and '"if class"' in formatted and 'scarlet' in formatted
    call([EXE,'fmt',src,'--dialeto=uma']);assert src.read_text(encoding='utf-8')==formatted
    repl=call([EXE,'repl'],'speed a=2;\na++;\nbakushin(a);\n:sair\n');assert repl.stdout=='3\n',repl.stdout
    src.write_text('bakushin(uma_args());uma_exit(7);',encoding='utf-8')
    assert call([EXE,'run',src,'--','馬','a b'],expected=7).stdout=='[馬, a b]\n'
    for code in [
        'own speed[] a=[1];own speed[] b=mover(a);bakushin(a);',
        'own speed[] a=[1];borrow speed[] b=&a;falcon(a,2);',
        'own speed[] a=[1];borrow speed[] b=&a;b[0]=2;',
        'own speed[] a=[1];borrow mut speed[] b=&mut a;borrow speed[] c=&a;',
        'own speed[] a=[1];speed[] b=a;',
        'speed[] leak(borrow speed[] a){special a;}',
        'trofeu speed n=1;n=2;',
        'power f(borrow mut speed[] a,borrow mut speed[] b){}own speed[] x=[1];f(&mut x,&mut x);',
        'own speed[] a=[1];speed[] b=mover(a);',
        'own speed[] a=[1];tatica()->speed f=tatica()->speed{special oguri(a);};',
        'rudolf C{condor C leak(){special teio;}}own C c=tachyon C();',
    ]:
        src.write_text(code,encoding='utf-8');r=call([EXE,'check',src],expected=1)
        assert 'propriedade' in r.stderr,r.stderr
    print(f'{COUNT} checks 3.1 passed')
