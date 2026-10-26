'use strict';
const vscode = require('vscode');
const path = require('node:path');
const fs = require('node:fs');
const service = require('./language-service');

function activate(context) {
  const selector = { language: 'umascript', scheme: 'file' };
  const states = new Map(), timers = new Map();
  const diagnostics = vscode.languages.createDiagnosticCollection('UmaScript');
  const output = vscode.window.createOutputChannel('UmaScript');
  const status = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 20);
  status.command = 'umascript.check';
  context.subscriptions.push(diagnostics, output, status);
  function toolchain() {
    const config = vscode.workspace.getConfiguration('umascript');
    const bundled = path.join(context.extensionPath, 'toolchain');
    const root = fs.existsSync(bundled) ? bundled : path.resolve(context.extensionPath, '..');
    const executable = process.platform === 'win32' ? 'uma.exe' : 'uma';
    const compiler = config.get('compilerPath') || path.join(root, 'bin', process.platform === 'win32' ? 'windows-x64' : 'linux-x64', executable);
    return { root, compiler, cxx: config.get('cxxPath') };
  }
  function publish() {
    diagnostics.clear(); const byFile = new Map();
    for (const state of states.values()) for (const d of state.diagnostics || []) {
      const file = d.file || state.file; const key = `${file}:${d.line}:${d.column}:${d.message}`;
      if (!byFile.has(file)) byFile.set(file, new Map());
      const range = new vscode.Range(Math.max(0, d.line - 1), Math.max(0, d.column - 1), Math.max(0, d.line - 1), Math.max(0, d.column));
      const item = new vscode.Diagnostic(range, d.message, vscode.DiagnosticSeverity.Error); item.source = `UmaScript (${d.phase})`; byFile.get(file).set(key, item);
    }
    for (const [file, items] of byFile) diagnostics.set(vscode.Uri.file(file), [...items.values()]);
  }
  function updateStatus() {
    const doc = vscode.window.activeTextEditor?.document;
    if (!doc || doc.languageId !== 'umascript') { status.hide(); return; }
    const state = states.get(doc.uri.toString());
    status.text = state?.error ? '$(warning) UmaScript' : state?.pending ? '$(sync~spin) UmaScript' : `$(check) UmaScript${state?.diagnostics?.length ? `: ${state.diagnostics.length} erro(s)` : ''}`;
    status.tooltip = state?.error || 'UmaScript 3.0 — verificar tipos e modulos'; status.show();
  }
  async function refresh(doc) {
    if (doc.languageId !== 'umascript' || doc.uri.scheme !== 'file' || !vscode.workspace.isTrusted) return;
    const key = doc.uri.toString(), previous = states.get(key), version = doc.version;
    previous?.controller?.abort(); const controller = new AbortController();
    const state = { ...previous, file: doc.fileName, version, controller, pending: true };
    states.set(key, state); updateStatus();
    try {
      const result = await service.analyze({ ...toolchain(), text: doc.getText(), file: doc.fileName, signal: controller.signal });
      if (states.get(key) !== state || doc.version !== version) return;
      if (result.diagnostics.length) {
        const imported = (previous?.symbols || []).filter(s => s.file !== doc.fileName);
        result.symbols.push(...imported.filter(s => !result.symbols.some(x => x.name === s.name && x.file === s.file && x.container === s.container)));
      }
      Object.assign(state, result, { pending: false, error: null }); publish();
    } catch (error) {
      if (controller.signal.aborted) return;
      state.pending = false; state.error = error.message; output.appendLine(error.message);
    }
    updateStatus();
  }
  function schedule(doc) {
    const key = doc.uri.toString(); clearTimeout(timers.get(key));
    timers.set(key, setTimeout(() => { timers.delete(key); refresh(doc); }, vscode.workspace.getConfiguration('umascript').get('diagnosticsDelay', 350)));
  }
  function symbols(doc) {
    const state = states.get(doc.uri.toString());
    const locals = service.scan(doc.getText(), doc.fileName).symbols;
    return [...locals, ...(state?.symbols || []).filter(s => s.file !== doc.fileName)];
  }
  function markdown(symbol) {
    const doc = new vscode.MarkdownString(); doc.appendCodeblock(symbol.signature || symbol.name, 'umascript');
    if (symbol.documentation) doc.appendMarkdown(`\n${symbol.documentation}`);
    if (symbol.private) doc.appendMarkdown('\n\n`groove`: membro privado da classe declarante.');
    doc.isTrusted = false; return doc;
  }
  const completionKinds = { class: vscode.CompletionItemKind.Class, method: vscode.CompletionItemKind.Method, function: vscode.CompletionItemKind.Function, variable: vscode.CompletionItemKind.Variable, field: vscode.CompletionItemKind.Field, parameter: vscode.CompletionItemKind.Variable };
  context.subscriptions.push(vscode.languages.registerCompletionItemProvider(selector, {
    provideCompletionItems(doc, pos) {
      const text = doc.getText(), offset = doc.offsetAt(pos), before = text.slice(0, offset);
      if (/import\s+"[^"\n]*$/.test(before)) return ['math', 'text', 'io', 'time', 'collections', 'memory', 'system', 'random'].map(name => { const item = new vscode.CompletionItem(`std/${name}`, vscode.CompletionItemKind.Module); const start = before.lastIndexOf('"') + 1; item.range = new vscode.Range(doc.positionAt(start), pos); return item; });
      const result = service.context(text, offset, symbols(doc), doc.fileName), seen = new Set();
      const items = result.symbols.filter(s => { if (seen.has(s.name)) return false; seen.add(s.name); return true; }).map(s => { const item = new vscode.CompletionItem(s.name, completionKinds[s.kind]); item.detail = s.signature; item.documentation = markdown(s); return item; });
      if (!result.member) for (const [name, documentation] of Object.entries(service.words)) { const item = new vscode.CompletionItem(name, vscode.CompletionItemKind.Keyword); item.documentation = documentation; items.push(item); }
      return items;
    }
  }, '.', '"'));
  context.subscriptions.push(vscode.languages.registerHoverProvider(selector, {
    provideHover(doc, pos) {
      const range = doc.getWordRangeAtPosition(pos, /[A-Za-z_]\w*/), name = range && doc.getText(range); if (!name) return;
      const symbol = service.resolve(doc.getText(), doc.offsetAt(range.end), name, symbols(doc), doc.fileName);
      if (symbol) return new vscode.Hover(markdown(symbol), range);
      if (service.words[name]) return new vscode.Hover(service.words[name], range);
    }
  }));
  context.subscriptions.push(vscode.languages.registerSignatureHelpProvider(selector, {
    provideSignatureHelp(doc, pos) {
      const text = doc.getText(), call = service.activeCall(text, doc.offsetAt(pos)); if (!call) return;
      const symbol = service.resolve(text, call.offset + call.name.length, call.name, symbols(doc), doc.fileName); if (!symbol || !symbol.signature?.includes('(')) return;
      const info = new vscode.SignatureInformation(symbol.signature, symbol.documentation);
      const args = symbol.signature.slice(symbol.signature.indexOf('(') + 1, -1).split(',').map(s => s.trim()).filter(Boolean);
      info.parameters = args.map(s => new vscode.ParameterInformation(s));
      const help = new vscode.SignatureHelp(); help.signatures = [info]; help.activeSignature = 0; help.activeParameter = Math.min(call.parameter, Math.max(0, args.length - 1)); return help;
    }
  }, '(', ','));
  context.subscriptions.push(vscode.languages.registerDefinitionProvider(selector, {
    provideDefinition(doc, pos) {
      const range = doc.getWordRangeAtPosition(pos, /[A-Za-z_]\w*/), name = range && doc.getText(range); if (!name) return;
      const symbol = service.resolve(doc.getText(), doc.offsetAt(range.end), name, symbols(doc), doc.fileName); if (!symbol?.file) return;
      return new vscode.Location(vscode.Uri.file(symbol.file), new vscode.Position(Math.max(0, symbol.line - 1), Math.max(0, symbol.column - 1)));
    }
  }));
  context.subscriptions.push(vscode.languages.registerDocumentSymbolProvider(selector, {
    provideDocumentSymbols(doc) {
      const kinds = { class: vscode.SymbolKind.Class, method: vscode.SymbolKind.Method, function: vscode.SymbolKind.Function, field: vscode.SymbolKind.Field, constructor: vscode.SymbolKind.Constructor, variable: vscode.SymbolKind.Variable, parameter: vscode.SymbolKind.Variable };
      return service.scan(doc.getText(), doc.fileName).symbols.filter(s => s.kind !== 'parameter').map(s => { const start = doc.positionAt(s.offset), range = new vscode.Range(start, doc.positionAt(s.offset + s.name.length)); return new vscode.DocumentSymbol(s.name, s.signature, kinds[s.kind], range, range); });
    }
  }));
  async function execute(command) {
    const doc = vscode.window.activeTextEditor?.document;
    if (!doc || doc.languageId !== 'umascript') return;
    if (!vscode.workspace.isTrusted) { vscode.window.showWarningMessage('Confie neste projeto para executar o compilador UmaScript.'); return; }
    if (!(await doc.save())) return;
    const { compiler, root, cxx } = toolchain(); const args = command === 'native' ? ['build', doc.fileName, '--native'] : [command, doc.fileName];
    args.push('--package-root', root); if (command === 'native' && cxx) args.push('--cc', cxx);
    const task = new vscode.Task({ type: 'umascript', command }, vscode.TaskScope.Workspace, `UmaScript ${command}`, 'UmaScript', new vscode.ProcessExecution(compiler, args, { cwd: path.dirname(doc.fileName) }), []);
    task.presentationOptions = { reveal: vscode.TaskRevealKind.Always, focus: command === 'run', panel: vscode.TaskPanelKind.Dedicated };
    await vscode.tasks.executeTask(task);
  }
  for (const [name, command] of [['run', 'run'], ['buildNative', 'native'], ['buildBytecode', 'compile']]) context.subscriptions.push(vscode.commands.registerCommand(`umascript.${name}`, () => execute(command)));
  context.subscriptions.push(vscode.commands.registerCommand('umascript.check', async () => { const doc = vscode.window.activeTextEditor?.document; if (doc) await refresh(doc); }));
  context.subscriptions.push(vscode.commands.registerCommand('umascript.documentation', async () => { const { root } = toolchain(); await vscode.commands.executeCommand('markdown.showPreview', vscode.Uri.file(path.join(root, 'docs', 'REFERENCIA_3.md'))); }));
  context.subscriptions.push(vscode.workspace.onDidOpenTextDocument(refresh));
  context.subscriptions.push(vscode.workspace.onDidChangeTextDocument(event => schedule(event.document)));
  context.subscriptions.push(vscode.workspace.onDidSaveTextDocument(doc => { if (doc.languageId === 'umascript') for (const open of vscode.workspace.textDocuments) refresh(open); }));
  context.subscriptions.push(vscode.workspace.onDidCloseTextDocument(doc => { const key = doc.uri.toString(); clearTimeout(timers.get(key)); timers.delete(key); states.get(key)?.controller?.abort(); states.delete(key); publish(); }));
  context.subscriptions.push(vscode.workspace.onDidChangeConfiguration(event => { if (event.affectsConfiguration('umascript')) for (const doc of vscode.workspace.textDocuments) refresh(doc); }));
  context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(updateStatus));
  context.subscriptions.push(vscode.workspace.onDidGrantWorkspaceTrust(() => { for (const doc of vscode.workspace.textDocuments) refresh(doc); }));
  context.subscriptions.push({ dispose() { for (const timer of timers.values()) clearTimeout(timer); for (const state of states.values()) state.controller?.abort(); } });
  for (const doc of vscode.workspace.textDocuments) refresh(doc); updateStatus();
}
module.exports = { activate };
