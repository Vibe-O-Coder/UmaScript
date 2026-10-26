"""Reproducible numeric benchmark. One workload is not a universal performance claim."""
from pathlib import Path
import argparse
import json
import os
import platform
import statistics
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--uma', required=True)
parser.add_argument('--cc', required=True)
parser.add_argument('--count', type=int, default=10000000)
parser.add_argument('--runs', type=int, default=9)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
out = root/'benchmarks'/'results'
out.mkdir(exist_ok=True)
suffix = '.exe' if os.name == 'nt' else ''
uma = Path(args.uma).resolve()
cc = Path(args.cc).resolve()
native, cpp = out/('numeric-uma'+suffix), out/('numeric-cpp'+suffix)
subprocess.run([str(uma), 'build', str(root/'benchmarks/numeric.uma'), '--native', '--cc', str(cc), '-o', str(native)], check=True)
env = {**os.environ, 'PATH': str(cc.parent)+os.pathsep+os.environ.get('PATH','')}
flags = ['-std=c++17', '-O3', '-fno-fast-math']
if os.name == 'nt': flags += ['-static', '-static-libgcc', '-static-libstdc++']
subprocess.run([str(cc), *flags, str(root/'benchmarks/numeric.cpp'), '-o', str(cpp)], check=True, env=env)
samples = {'UmaScript AOT': [], 'C++': []}
checksum = None
stdin = f'{args.count}\n17\n'
# Warm up both, then alternate execution order across samples.
for run in range(-1, args.runs):
    targets = [('UmaScript AOT', native), ('C++', cpp)]
    if run % 2: targets.reverse()
    for name, binary in targets:
        result = subprocess.check_output([str(binary)], input=stdin, encoding='utf-8').split()
        current, nanos = map(int, result)
        if checksum is None: checksum = current
        assert checksum == current, 'Different results invalidate the comparison'
        if run >= 0: samples[name].append(nanos/1e6)
report = {
    'platform': platform.platform(), 'processor': platform.processor(),
    'compiler': subprocess.check_output([str(cc), '--version'], encoding='utf-8', env=env).splitlines()[0],
    'count': args.count, 'runs': args.runs, 'seed': 17, 'checksum': checksum,
    'samples_ms': samples, 'median_ms': {k: statistics.median(v) for k,v in samples.items()},
    'method': 'Internal monotonic clock; excludes startup/compilation; warm-up then alternating order. Input read at runtime.',
    'limits': 'One scalar integer workload; not a claim about arrays, allocation, virtual calls, I/O or all C/C++ programs.'
}
report['uma_to_cpp_ratio'] = report['median_ms']['UmaScript AOT']/report['median_ms']['C++']
(out/'numeric.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps(report, indent=2))
