# Validacao UmaScript 3.0

Data: 24 de outubro de 2026. Plataforma executada: Windows x64, GCC/MinGW UCRT 15.2.0. O compilador foi recompilado com C++17, ligacao estatica, -O2 e -Wall -Wextra -Wpedantic -Werror.

## Resultados

- 154 casos da linguagem passaram em cada modo: fonte, bytecode salvo e executavel com VM (462 execucoes de casos). Cada suite inclui 180 operacoes inteiras diferenciais e todos os dez exemplos.
- 104 verificacoes de integracao passaram: bytecode deterministico, corrupcao/truncamento, CRC, opcodes invalidos, protecao de saida e executaveis autonomos.
- 72 verificacoes novas passaram: seis programas comparados entre VM/AOT, cinco programas com erros de runtime, modulos, imports duplicados/ciclicos, metadados, stdin/JSON, caminhos com espacos/acentos, protecao de fontes importadas e test runner.
- Os casos AOT cobrem tipos, arrays aninhados/aliases, booleanos, Unicode/emoji/U+0000, inteiros com zeros iniciais, comparacoes acima de 2^53, recursao, fluxo, construtores, heranca, polimorfismo, ciclos, arquivos binarios/texto, ambiente Unicode Windows e a biblioteca padrao.
- Executaveis AOT foram executados sem fonte e com PATH vazio. Os casos de erro verificaram overflow, limites, referencia nula, deslocamento invalido e resultados reais nao finitos.
- `uma test tests/stdlib` passou, incluindo mapa, pilha, ordenacao, buscas, texto, PRNG e palavras de 64 bits little-endian.
- Nove testes Node da extensao passaram: contratos, indice/documentacao, escopos e sombreamento, variaveis de laco, membros/heranca/privacidade, parametros aninhados, UTF-8/UTF-16, buffer nao salvo/imports e manifestos.
- Uma instancia real e separada do VSCode validou ativacao, hover de modulo importado, ir para definicao, comentarios tecnicos, outline, diagnostico de texto nao salvo e conclusao de membros publicos/privados.
- O VSIX foi aceito pelo instalador oficial do VSCode em uma pasta de extensoes isolada, como umascript-language.umascript@3.0.0.
- Os arquivos distribuiveis passaram por verificacao de CRC e SHA-256. O ZIP foi extraido em caminho com espacos/acentos; o compilador extraido verificou imports, executou os testes da biblioteca e gerou um AOT executado sem fonte/PATH.

## Desempenho medido

`benchmarks/run.py` executou o mesmo gerador congruencial e checksum em UmaScript AOT e C++. Dez milhoes de iteracoes, semente 17, entrada em tempo de execucao, uma rodada de aquecimento e nove amostras em ordem alternada. O relogio interno exclui inicializacao do processo e compilacao. Os checksums coincidiram (1558294560).

| Implementacao | Mediana |
| --- | --- |
| UmaScript AOT -O3 | 44,1882 ms |
| C++ -O3 | 43,2844 ms |

Razao UmaScript/C++: 1,02088, aproximadamente 2,1% a mais de tempo nesta carga. Amostras e ambiente completos: `benchmarks/results/numeric.json`. Isso nao estabelece paridade universal: arrays, alocacao, chamadas virtuais, Unicode, E/S e programas distintos precisam de medicoes proprias.

## Alcance

Os binarios 3.0 e a extensao foram validados no Windows. Nao foi recompilado/executado o alvo Linux 3.0 nesta maquina; o binario Linux original continua 2.0, identificado por version.txt, e nao entra no ZIP/VSIX Windows 3.0. O instalador grafico original tambem continua 2.0; o script de instalacao e o pacote novo usam 3.0.

Sanitizadores nao foram executados para a versao 3.0; o relatorio de sanitizadores em VALIDACAO.md pertence somente a versao 2.0. Os testes nao equivalem a uma auditoria de seguranca ou garantia industrial. Recursos ainda ausentes e limites de memoria estao documentados em REFERENCIA_3.md.
