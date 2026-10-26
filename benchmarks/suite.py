"""Eleven reproducible VM/native workloads, with equality checked before timing."""
from pathlib import Path
import argparse, json, os, statistics, subprocess, tempfile, time, sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tests'))
from support import temporary_directory
CASES={
 'integer_sum':'speed s=0;kitasan(speed i=0;i<100000;i++){s+=i;}bakushin(s);',
 'nested_loops':'speed s=0;kitasan(speed i=0;i<300;i++){kitasan(speed j=0;j<300;j++){s+=(i+j)%7;}}bakushin(s);',
 'fibonacci':'speed fib(speed n){scarlet(n<2){special n;}special fib(n-1)+fib(n-2);}bakushin(fib(22));',
 'sort':'import "std/collections";speed[] a=[];kitasan(speed i=20000;i>0;i--){falcon(a,(i*37)%20011);}sort(a);bakushin(a[0],a[19999],sum(a));',
 'sieve':'track[] p=[];kitasan(speed i=0;i<100000;i++){falcon(p,turf);}speed n=0;kitasan(speed i=2;i<100000;i++){scarlet(p[i]){n++;kitasan(speed j=i*2;j<100000;j+=i){p[j]=dirt;}}}bakushin(n);',
 'array_access':'speed[] a=[];kitasan(speed i=0;i<50000;i++){falcon(a,i);}speed s=0;kitasan(speed i=0;i<oguri(a);i++){s+=a[i];}bakushin(s);',
 'matrix':'speed[][] a=[];kitasan(speed i=0;i<40;i++){speed[] r=[];kitasan(speed j=0;j<40;j++){falcon(r,i+j);}falcon(a,r);}speed s=0;kitasan(speed i=0;i<40;i++){kitasan(speed j=0;j<40;j++){kitasan(speed k=0;k<40;k++){s+=a[i][k]*a[k][j];}}}bakushin(s);',
 'allocation':'rudolf C{condor speed n=7;}speed s=0;kitasan(speed i=0;i<50000;i++){C c=tachyon C();s+=c.n;}bakushin(s);',
 'text_append':'wit s="";kitasan(speed i=0;i<50000;i++){s+="x";}bakushin(oguri(s));',
 'text_index':'wit s="";kitasan(speed i=0;i<20000;i++){s+="x";}speed n=0;kitasan(speed i=0;i<oguri(s);i++){n+=hayahide(s[i]);}bakushin(n);',
 'hash_map':'import "std/collections";StringIntMap m=tachyon StringIntMap();kitasan(speed i=0;i<5000;i++){m.set(maruzensky(i),i);}speed s=0;kitasan(speed i=0;i<5000;i++){s+=m.get(maruzensky(i));}bakushin(s);',
}
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--uma',type=Path,required=True);parser.add_argument('--samples',type=int,default=3);parser.add_argument('--output',type=Path,default=ROOT/'benchmarks/results/suite-3.1.json');args=parser.parse_args();exe=args.uma.resolve();results={}
    def run(command):
        start=time.perf_counter();p=subprocess.run(list(map(str,command)),capture_output=True,text=True,encoding='utf-8',timeout=120);assert p.returncode==0,(command,p.stderr);return p.stdout,time.perf_counter()-start
    with temporary_directory('benchmark-') as folder:
        folder=Path(folder);src=folder/'benchmark.uma';native=folder/('benchmark.exe' if os.name=='nt' else 'benchmark')
        for name,code in CASES.items():
            src.write_text(code,encoding='utf-8');run([exe,'build',src,'--native','-o',native]);vm,first=run([exe,'run',src]);aot,firstNative=run([native]);assert vm==aot,(name,vm,aot);vmTimes=[first];nativeTimes=[firstNative]
            for _ in range(args.samples-1):vm,elapsed=run([exe,'run',src]);vmTimes.append(elapsed);aot,elapsed=run([native]);nativeTimes.append(elapsed);assert vm==aot
            results[name]={'stdout':vm,'vm_seconds':vmTimes,'native_seconds':nativeTimes,'vm_median':statistics.median(vmTimes),'native_median':statistics.median(nativeTimes)};print(name,results[name]['vm_median'],results[name]['native_median'],flush=True)
    args.output.parent.mkdir(exist_ok=True);args.output.write_text(json.dumps({'note':'Wall time includes startup; this is not the original report hardware or Python comparison.','cases':results},indent=2)+'\n',encoding='utf-8')
if __name__=='__main__':main()
