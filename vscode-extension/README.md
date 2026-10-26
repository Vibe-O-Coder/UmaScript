# UmaScript para VSCode

Suporte a `.uma` com sintaxe tematica e documentacao tecnica em portugues.

- Diagnosticos do checker real para o buffer ainda nao salvo.
- Conclusao de nomes, classes e membros com heranca/visibilidade.
- Hover com assinatura, tipos, efeitos, erros e comentarios `///`.
- Parametros, definicoes, simbolos, snippets e delimitadores automaticos.
- Comandos de verificar, executar, gerar bytecode e compilar nativo.

Instale `umascript-3.1.0.vsix` com **Extensions: Install from VSIX**. Inclui compilador Windows 3.1, biblioteca e referencia, sem baixar npm em tempo de execucao. No Linux, use o compilador do pacote portatil 3.1 e configure `umascript.compilerPath`.

AOT no pacote completo Windows inclui GCC/MinGW. O VSIX sozinho usa GCC/Clang externo ou o compilador do pacote completo via compilerPath. No Linux, AOT usa GCC/Clang do sistema. Bytecode dispensa essa ferramenta. Configuracoes: `umascript.compilerPath`, `umascript.cxxPath`, `umascript.diagnosticsDelay` (350 ms). Analise/execucao respeitam a confianca do workspace.

Use **UmaScript: Abrir referencia tecnica**. Para documentar codigo:

```uma
/// Retorna o dobro. Overflow de speed e erro. O(1).
speed dobro(speed valor) { special valor * 2; }
```

Imports usam arquivos salvos. Conclusao cobre receptores simples e pode ser aproximada durante edicoes incompletas. Nao inclui debugger proprio, rename semantico ou servidor LSP separado.

Desenvolvimento: abra esta pasta e pressione F5. Testes: `npm test`. Empacotamento offline na raiz: `python tools/package_vscode.py`.
