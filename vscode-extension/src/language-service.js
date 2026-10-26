'use strict';
const fs = require('node:fs');
const path = require('node:path');
const { execFile } = require('node:child_process');
const { builtins, words } = require('./language-data');
const aliases = Object.fromEntries(Object.entries(require('./vocabulary.json').words).map(([a,b]) => [b,a]));

function tokenize(text) {
  const regex = /\/\/[^\n]*|\/\*[\s\S]*?(?:\*\/|$)|"(?:\\.|[^"\\])*"?|'(?:\\.|[^'\\])*'?|[A-Za-z_][A-Za-z_0-9]*|\d+(?:\.\d+)?|\S/g;
  return [...text.matchAll(regex)].filter(m => !m[0].startsWith('//') && !m[0].startsWith('/*')).map(m => ({ value: /^[A-Za-z_]\w*$/.test(m[0]) ? (aliases[m[0]] || m[0]) : m[0], offset: m.index }));
}
function position(text, offset) { const prefix = text.slice(0, offset); const line = (prefix.match(/\n/g) || []).length; return { line: line + 1, column: offset - prefix.lastIndexOf('\n') }; }
function offsetAt(text, line, column) { const lines = text.split('\n'); let n = 0; for (let i = 1; i < line && i <= lines.length; i++) n += lines[i - 1].length + 1; return Math.min(text.length, n + column - 1); }
function byteColumnToUtf16(text, line, column) { const sourceLine = text.split('\n')[Math.max(0, line - 1)] || ''; return Buffer.from(sourceLine, 'utf8').subarray(0, Math.max(0, column - 1)).toString('utf8').length + 1; }
function documentation(text, line) {
  const lines = text.split(/\r?\n/); const result = [];
  for (let i = line - 2; i >= 0 && /^\s*\/\/\//.test(lines[i]); --i) result.unshift(lines[i].replace(/^\s*\/\/\/\s?/, ''));
  return result.join('\n');
}
function scan(text, file) {
  const ts = tokenize(text), symbols = [], scopes = [], stack = [], starts = new Map(), loops = [];
  const id = x => /^[A-Za-z_]\w*$/.test(x || '');
  const value = i => ts[i]?.value;
  for (let i = 0; i < ts.length; i++) if (value(i) === 'kitasan' && value(i + 1) === '(') {
    let j = i + 2, depth = 1;
    for (; j < ts.length && depth; j++) { if (value(j) === '(') depth++; if (value(j) === ')') depth--; }
    const headerEnd = j - 1;
    if (depth || value(j) !== '{') continue;
    let end = j + 1; depth = 1;
    for (; end < ts.length && depth; end++) { if (value(end) === '{') depth++; if (value(end) === '}') depth--; }
    const loop = { kind: 'loop', start: ts[i + 1].offset, headerEnd: ts[headerEnd].offset, end: depth ? text.length : ts[end - 1].offset };
    loops.push(loop);
  }
  const add = (token, item) => { const pos = position(text, token.offset); const symbol = { ...item, ...pos, name: token.value, file, offset: token.offset, documentation: documentation(text, pos.line) }; symbols.push(symbol); return symbol; };
  const parent = () => stack.length ? stack[stack.length - 1] : { kind: 'global', name: '', start: 0, end: text.length };
  for (let i = 0; i < ts.length; i++) {
    if (value(i) === 'rudolf' && id(value(i + 1))) {
      const name = value(i + 1), base = value(i + 2) === 'diamond' ? value(i + 3) : '';
      add(ts[i + 1], { kind: 'class', type: name, container: '', signature: `rudolf ${name}${base ? ` diamond ${base}` : ''}`, base });
      let open = i + 2; while (open < ts.length && value(open) !== '{') open++;
      starts.set(open, { kind: 'class', name, className: name });
    }
    if (value(i) === '{') { const scope = { ...(starts.get(i) || { kind: 'block', name: parent().name, className: parent().className }), start: ts[i].offset, end: text.length, parent: parent() }; scopes.push(scope); stack.push(scope); }
    if (value(i) === '}') { const scope = stack.pop(); if (scope) scope.end = ts[i].offset; }
    let type = value(i), ni = i + 1;
    while (value(ni) === '[' && value(ni + 1) === ']') { type += '[]'; ni += 2; }
    const ctor = type === 'opera' && value(i + 1) === '(';
    if (!ctor && (!id(value(i)) || (words[value(i)] && !['speed', 'stamina', 'power', 'guts', 'wit', 'track'].includes(value(i))) || !id(value(ni)) || words[value(ni)])) continue;
    if (ctor) { ni = i; type = 'power'; }
    const next = value(ni + 1), current = parent();
    if (next === '(') {
      let close = ni + 2, nesting = 1;
      while (close < ts.length && nesting) { if (value(close) === '(') nesting++; if (value(close) === ')') nesting--; if (nesting) close++; }
      if (close >= ts.length) continue;
      const params = []; for (let j = ni + 2; j < close;) { let pt = value(j++); while (value(j) === '[' && value(j + 1) === ']') { pt += '[]'; j += 2; } if (!id(value(j))) break; params.push({ type: pt, token: ts[j++] }); if (value(j) === ',') j++; else break; }
      const name = value(ni), container = current.kind === 'class' ? current.name : '', functionName = container ? `${container}.${name}` : name;
      const signature = `${type} ${functionName}(${params.map(p => `${p.type} ${p.token.value}`).join(', ')})`;
      const symbol = add(ts[ni], { kind: ctor ? 'constructor' : container ? 'method' : 'function', type, signature, container, private: value(i - 1) === 'groove' });
      let open = close + 1; while (open < ts.length && !['{', ';'].includes(value(open))) open++;
      if (value(open) === '{') starts.set(open, { kind: 'function', name: functionName, className: container, symbol });
      for (const p of params) { const param = add(p.token, { kind: 'parameter', type: p.type, signature: `${p.type} ${p.token.value}`, container: functionName }); param.pendingScope = open; }
      i = close;
    } else if (['=', ';', ',', ')'].includes(next)) {
      const loop = loops.filter(loop => ts[ni].offset >= loop.start && ts[ni].offset <= loop.headerEnd).pop();
      add(ts[ni], { kind: current.kind === 'class' ? 'field' : 'variable', type, signature: `${type} ${value(ni)}`, container: current.name, private: value(i - 1) === 'groove', scope: loop || current });
      i = ni;
    }
  }
  for (const s of symbols) {
    if (s.pendingScope !== undefined) s.scope = scopes.find(scope => scope.start === ts[s.pendingScope]?.offset);
    if (s.scope && s.scope.kind !== 'global') { s.scopeStart = s.scope.start; s.scopeEnd = s.scope.end; }
    delete s.scope; delete s.pendingScope;
  }
  return { symbols, scopes };
}
function visible(symbols, text, offset, file) {
  const local = symbols.filter(s => !s.file || s.file === file || ['function', 'class', 'method', 'field', 'constructor'].includes(s.kind) || !s.container);
  return local.filter(s => {
    if (['method', 'field', 'constructor'].includes(s.kind)) return false;
    if (s.scopeStart !== undefined && (offset < s.scopeStart || offset > s.scopeEnd)) return false;
    if (s.kind === 'variable' && s.file === file && s.offset > offset) return false;
    return true;
  }).sort((a, b) => (b.scopeStart || -1) - (a.scopeStart || -1) || (b.offset || -1) - (a.offset || -1));
}
function membersFor(symbols, type, className = '') {
  const out = [], seen = new Set(), visited = new Set();
  while (type && !visited.has(type)) {
    visited.add(type);
    for (const s of symbols) if (s.container === type && ['field', 'method'].includes(s.kind) && (!s.private || type === className) && !seen.has(s.name)) { out.push(s); seen.add(s.name); }
    const cls = symbols.find(s => s.kind === 'class' && s.name === type);
    type = cls?.base || cls?.signature.match(/\bdiamond\s+(\w+)/)?.[1];
  }
  return out;
}
function context(text, offset, symbols, file) {
  const before = text.slice(0, offset), match = before.match(/\b([A-Za-z_]\w*)\s*\.\s*[A-Za-z_0-9]*$/);
  const index = scan(text, file);
  const enclosing = index.scopes.filter(s => s.start < offset && s.end >= offset && s.className).pop();
  let type;
  if (match) {
    if (match[1] === 'teio') type = enclosing?.className;
    else if (match[1] === 'mcqueen') type = symbols.find(s => s.name === enclosing?.className && s.kind === 'class')?.signature.match(/\bdiamond\s+(\w+)/)?.[1];
    else type = visible(symbols, text, offset, file).find(s => s.name === match[1])?.type;
  }
  return { member: !!match, symbols: match ? membersFor(symbols, type, enclosing?.className) : [...visible(symbols, text, offset, file), ...builtins] };
}
function resolve(text, offset, name, symbols, file) { return context(text, offset, symbols, file).symbols.find(s => s.name === name); }
function activeCall(text, offset) {
  const ts = tokenize(text.slice(0, offset)); let nested = 0, commas = 0;
  for (let i = ts.length - 1; i >= 0; i--) {
    const v = ts[i].value;
    if (v === ')' || v === ']') nested++;
    else if (v === '(' || v === '[') {
      if (nested) nested--;
      else if (v === '(' && /^[A-Za-z_]\w*$/.test(ts[i - 1]?.value || '')) return { name: ts[i - 1].value, parameter: commas, offset: ts[i - 1].offset };
      else return null;
    } else if (v === ',' && !nested) commas++;
  }
  return null;
}
async function analyze({ text, file, compiler, root, signal }) {
  const lexical = scan(text, file);
  const result = await new Promise((resolve, reject) => {
    const child = execFile(compiler, ['inspect', file, '--stdin', '--package-root', root], { encoding: 'utf8', windowsHide: true, timeout: 15000, maxBuffer: 8 * 1024 * 1024, signal }, (error, stdout, stderr) => {
      try { resolve(JSON.parse(stdout)); } catch { reject(new Error(stderr.trim() || error?.message || 'Resposta invalida do compilador UmaScript.')); }
    });
    child.stdin.on('error', () => {}); child.stdin.end(text);
  });
  const files = new Map([[file, text]]);
  const getSource = file => { if (!files.has(file)) { try { files.set(file, fs.readFileSync(file, 'utf8')); } catch { files.set(file, ''); } } return files.get(file); };
  const lexicalFiles = new Map([[file, lexical.symbols]]);
  for (const s of result.symbols || []) {
    const source = getSource(s.file);
    if (!lexicalFiles.has(s.file)) lexicalFiles.set(s.file, scan(source, s.file).symbols);
    s.column = byteColumnToUtf16(source, s.line, s.column);
    const local = lexicalFiles.get(s.file).find(x => x.name === s.name && x.line === s.line && x.kind === s.kind);
    if (local) Object.assign(s, local);
    else { s.documentation = documentation(source, s.line); s.offset = offsetAt(source, s.line, s.column); }
  }
  result.diagnostics = (result.diagnostics || []).map(d => ({ ...d, column: byteColumnToUtf16(getSource(d.file), d.line, d.column) }));
  const currentFile = path.resolve(file).toLowerCase();
  const imported = (result.symbols || []).filter(s => path.resolve(s.file).toLowerCase() !== currentFile);
  result.symbols = [...lexical.symbols, ...imported];
  return result;
}
module.exports = { tokenize, scan, visible, context, resolve, activeCall, analyze, byteColumnToUtf16, documentation, position, builtins, words };
