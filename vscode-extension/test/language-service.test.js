'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const s = require('../src/language-service');
const file = path.resolve('main.uma');

test('technical built-in docs describe errors, Unicode and side effects', () => {
  assert.match(s.builtins.find(b => b.name === 'oguri').documentation, /Unicode/);
  assert.match(s.builtins.find(b => b.name === 'falcon').documentation, /aliases/);
  assert.match(s.builtins.find(b => b.name === 'taiki').documentation, /64 bits/);
});
test('declarations, doc comments and arrays are indexed without comment/string false positives', () => {
  const code = '/// Retorna soma.\nspeed soma(speed[] xs){wit texto="speed fake;"; // speed ghost;\nspecial 0;}';
  const symbols = s.scan(code, file).symbols;
  assert.equal(symbols.find(x => x.name === 'soma').documentation, 'Retorna soma.');
  assert.equal(symbols.find(x => x.name === 'xs').type, 'speed[]');
  assert.ok(!symbols.some(x => ['fake', 'ghost'].includes(x.name)));
});
test('completion respects nested local scopes and shadowing', () => {
  const code = 'speed n=1; power f(){wit n="x"; {track inner=turf;} n; }';
  const symbols = s.scan(code, file).symbols, offset = code.lastIndexOf('n;');
  const visible = s.visible(symbols, code, offset, file);
  assert.equal(visible.find(x => x.name === 'n').type, 'wit');
  assert.ok(!visible.some(x => x.name === 'inner'));
});
test('public inherited members complete after an instance and private members stay scoped', () => {
  const code = 'rudolf Base{groove speed secret;condor speed read(){special teio.secret;}} rudolf Child diamond Base{condor wit name;} Child runner=tachyon Child();runner.';
  const symbols = s.scan(code, file).symbols;
  const members = s.context(code, code.length, symbols, file).symbols;
  assert.ok(members.some(x => x.name === 'read'));
  assert.ok(members.some(x => x.name === 'name'));
  assert.ok(!members.some(x => x.name === 'secret'));
  const own = s.context(code, code.indexOf('teio.secret') + 'teio.'.length, symbols, file).symbols;
  assert.ok(own.some(x => x.name === 'secret'));
});
test('counted-loop variables are visible only in the loop', () => {
  const code = 'kitasan(speed i=0;i<5;i++){bakushin(i);} bakushin(1);';
  const symbols = s.scan(code, file).symbols;
  assert.ok(s.visible(symbols, code, code.indexOf('bakushin(i)'), file).some(x => x.name === 'i'));
  assert.ok(!s.visible(symbols, code, code.length, file).some(x => x.name === 'i'));
});
test('signature context counts arguments with nested calls and array literals', () => {
  const outer = 'bakushin(1, taiki("2"), [3,4], ';
  assert.deepEqual(s.activeCall(outer, outer.length), { name: 'bakushin', parameter: 3, offset: 0 });
  const code = 'falcon(items, soma(1, ';
  assert.equal(s.activeCall(code, code.length).name, 'soma');
  assert.equal(s.activeCall(code, code.length).parameter, 1);
});
test('UTF-8 compiler columns map to VS Code UTF-16 columns', () => {
  const line = 'bakushin("馬😀"); speed n="x";';
  const index = line.indexOf('n=');
  const byteColumn = Buffer.byteLength(line.slice(0, index), 'utf8') + 1;
  assert.equal(s.byteColumnToUtf16(line, 1, byteColumn), index + 1);
});
test('compiler integration supplies diagnostics for an unsaved buffer and imported documentation', async () => {
  const root = path.resolve(__dirname, '../..');
  const compiler = process.env.UMA_TEST_COMPILER || path.join(root, 'bin', process.platform === 'win32' ? 'windows-x64/uma.exe' : 'linux-x64/uma');
  const example = path.join(root, 'examples/01_bakushin.uma');
  const bad = await s.analyze({ text: 'speed n="x";', file: example, compiler, root });
  assert.match(bad.diagnostics[0].message, /Esperado speed/);
  const good = await s.analyze({ text: 'import "std/math"; speed n=abs(-4);', file: example, compiler, root });
  assert.equal(good.diagnostics.length, 0);
  assert.match(good.symbols.find(x => x.name === 'abs').documentation, /INT64_MIN/);
  assert.equal(good.symbols.find(x => x.name === 'n').type, 'speed');
});
test('extension assets and package entry points exist', () => {
  const root = path.resolve(__dirname, '..'), manifest = require('../package.json');
  for (const name of [manifest.main, manifest.contributes.languages[0].configuration, manifest.contributes.grammars[0].path, manifest.contributes.snippets[0].path]) assert.ok(fs.existsSync(path.join(root, name)), name);
  JSON.parse(fs.readFileSync(path.join(root, manifest.contributes.grammars[0].path), 'utf8'));
});
