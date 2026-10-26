'use strict';
// Runs inside a separate VS Code Extension Development Host.
const vscode = require('vscode');
const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');
const pause = ms => new Promise(resolve => setTimeout(resolve, ms));
async function until(read, predicate, message) {
  for (let i = 0; i < 80; i++) { const result = await read(); if (predicate(result)) return result; await pause(100); }
  throw new Error(message);
}
async function run() {
  const extension = vscode.extensions.getExtension('umascript-language.umascript');
  assert.ok(extension); await extension.activate();
  const root = path.resolve(__dirname, '../..');
  const file = path.join(root, '.test-tmp', 'vscode-host.uma');
  fs.mkdirSync(path.dirname(file), { recursive: true });
  const code = 'import "std/math";\n/// Retorna dobro; overflow e erro.\nspeed dobro(speed valor){special valor*2;}\nspeed n=dobro(abs(-4));\n';
  fs.writeFileSync(file, code, 'utf8');
  const doc = await vscode.workspace.openTextDocument(file); await vscode.window.showTextDocument(doc);
  await until(() => vscode.commands.executeCommand('vscode.executeHoverProvider', doc.uri, new vscode.Position(3, 16)), h => h?.some(x => x.contents.some(c => String(c.value || c).includes('INT64_MIN'))), 'Imported hover missing');
  const definition = await vscode.commands.executeCommand('vscode.executeDefinitionProvider', doc.uri, new vscode.Position(3, 16));
  assert.ok(definition.some(d => (d.uri || d.targetUri).fsPath.endsWith('math.uma')));
  const hover = await vscode.commands.executeCommand('vscode.executeHoverProvider', doc.uri, new vscode.Position(3, 9));
  assert.ok(hover.some(x => x.contents.some(c => String(c.value || c).includes('Retorna dobro'))));
  const outline = await vscode.commands.executeCommand('vscode.executeDocumentSymbolProvider', doc.uri);
  assert.ok(outline.some(s => s.name === 'dobro'));
  const edit = new vscode.WorkspaceEdit(); edit.replace(doc.uri, new vscode.Range(new vscode.Position(0, 0), doc.positionAt(doc.getText().length)), 'speed n="bad";'); await vscode.workspace.applyEdit(edit);
  await until(() => vscode.languages.getDiagnostics(doc.uri), ds => ds.some(d => d.message.includes('Esperado speed')), 'Unsaved diagnostics missing');
  const fix = new vscode.WorkspaceEdit(); fix.replace(doc.uri, new vscode.Range(new vscode.Position(0, 0), doc.positionAt(doc.getText().length)), 'rudolf C{condor speed value;groove speed secret;} C c=tachyon C();c.'); await vscode.workspace.applyEdit(fix);
  const completion = await vscode.commands.executeCommand('vscode.executeCompletionItemProvider', doc.uri, doc.positionAt(doc.getText().length), '.');
  assert.ok(completion.items.some(i => i.label === 'value'));
  assert.ok(!completion.items.some(i => i.label === 'secret'));
  const result = { extensionHost: 'passed', version: extension.packageJSON.version, extensionPath: extension.extensionPath, checks: ['activation','imported hover','imported definition','technical documentation','outline','unsaved diagnostics','public/private member completion'] };
  fs.writeFileSync(path.join(root, '.test-tmp', 'vscode-host-result.json'), JSON.stringify(result, null, 2));
  // Close without persisting the deliberately invalid scratch buffer.
  await vscode.commands.executeCommand('workbench.action.revertAndCloseActiveEditor'); fs.unlinkSync(file);
}
module.exports = { run };
