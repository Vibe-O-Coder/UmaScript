# Validacao 3.1 — 24/10/2026

Validacao local concluida no Windows x64 com GCC 15.2 e no Ubuntu x86-64 com GCC 13. O Linux foi executado pelo WSL, com fontes e testes copiados para uma pasta temporaria Linux; os perfis e diretorios reais do usuario nao foram alterados pelos testes de instalacao.

| Verificacao | Windows | Linux |
| --- | --- | --- |
| Linguagem em codigo-fonte | 158 casos passaram | 158 casos passaram |
| Linguagem em bytecode serializado | 158 casos passaram | 158 casos passaram |
| Executavel com VM embutida | 158 casos passaram | 158 casos passaram |
| CLI, corrupcao, instalacao e protecao de arquivos | 104 verificacoes | 117 verificacoes |
| Regressao AOT/stdlib 3.0 | 72 verificacoes | 72 verificacoes |
| Recursos 3.1 e rejeicoes estaticas | 80 verificacoes | 80 verificacoes |
| LSP | Passou | Passou |
| GC: 100 mil ciclos nativos e 100 mil ciclos de closures VM | Passou | Passou |
| Extensao/editor | 9 testes Node passaram | Executada no Windows |

Os testes 3.1 comparam codigo-fonte, bytecode carregado e AOT. Cobrem movimento, emprestimos, conflitos/escapes, erros e finally com retorno/break/continue, callbacks, captura por valor, genericos, hash com remocao/rehash, interfaces, foreach, numeros, Unicode, aliases de modulos, dialetos, formatador, REPL, JSON, CSV e base64.

O teste de memoria tambem confirma que o coletor preserva raizes ainda vivas. Nao e uma prova de ausencia de todo defeito de memoria; o teste mede os contratos exercitados.

O LSP foi exercitado por mensagens JSON-RPC reais com Content-Length e Unicode: inicializacao, diagnosticos, edicao, hover, conclusao, formatacao, simbolos e encerramento.

## Entrega

tools/package_release.py usa lista permitida de fontes e artefatos. Exclui caches, .test-tmp, __pycache__, resultados executaveis e o instalador antigo. Gera SHA256SUMS.txt e datas fixas nas entradas ZIP.

tools/verify_package.py confere hashes, conteudo dos arquivos, VSIX, extracao com caminhos seguros, compilacao AOT usando o GCC incluido com PATH vazio e execucao de programas VM/AOT sem seus fontes, incluindo argumentos Unicode.

Os resultados do teste de extracao sao apresentados ao final da execucao dessa ferramenta. CI hospedado e publicacao remota nao foram executados neste ambiente.

## Benchmarks

As 11 cargas de benchmarks/suite.py validam igualdade entre VM e AOT antes de medir. Foram usadas tres amostras por backend. Os tempos incluem inicio do processo e, na VM, carregamento/verificacao do fonte. As cargas e a maquina diferem do relatorio original; nao representam comparacao nova com Python nem sustentam promessa de ganho de 10 a 30 vezes.

Resultados brutos: benchmarks/results/suite-3.1.json. Tabela: BENCHMARKS_3_1.md.

