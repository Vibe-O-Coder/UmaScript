# Referencia tecnica UmaScript 3.0

Esta referencia descreve recursos implementados. A semantica original dos tipos e de OOP em `LINGUAGEM.md` continua valida; esta versao acrescenta modulos, operacoes de bits, biblioteca padrao, metadados para editor e AOT tipado.

## Tipos

| Tipo | Contrato |
| --- | --- |
| `speed` | Inteiro com sinal de exatamente 64 bits; padrao zero. |
| `stamina` | IEEE 754 binary64; padrao zero; resultados nao finitos sao erros. |
| `track` | Booleano `turf`/`dirt`; padrao dirt. |
| `guts` | Um valor escalar Unicode, excluindo substitutos; padrao U+0000. |
| `wit` | Texto UTF-8 por valor, inclusive U+0000; padrao vazio. |
| `power` | Ausencia de retorno; proibido como variavel/parametro. |
| `Classe` | Referencia com identidade; padrao nature. |
| `T[]` | Array tipado mutavel e redimensionavel; padrao vazio. |
| `nature` | Referencia nula compativel com classes e arrays. |

Tipos sao explicitos. `speed → stamina` e a unica conversao numerica implicita. Arrays sao invariantes. Uma referencia derivada pode ser atribuida a uma referencia da base. Arrays/objetos compartilham referencia; textos possuem semantica de valor. Igualdade de referencias compara identidade, igualdade de textos compara conteudo. Comparacoes mistas entre inteiros e reais preservam a distincao de inteiros acima de 2^53.

## Operadores e avaliacao

Precedencia crescente:

1. `= += -= *= /= %= &= |= ^= <<= >>=` (associativas a direita).
2. `||`.
3. `&&`.
4. `|`.
5. `^`.
6. `&`.
7. `== !=`.
8. `< > <= >=`.
9. `<< >>`.
10. `+ -`.
11. `* / %`.
12. Unarios `! ~ + - ++ --`.
13. Chamada, membro, indice e incremento/decremento posfixo.

`&&`/`||` usam curto-circuito. Argumentos e operandos sao avaliados da esquerda para a direita nos dois backends. A atribuicao resolve o destino antes do lado direito; a composta le o valor antigo antes do lado direito. Referencias a elementos de array verificam limites novamente caso o array mude durante a expressao.

Aritmetica inteira verifica overflow, incluindo negacao de INT64_MIN e INT64_MIN/-1. INT64_MIN%-1 vale zero. Divisao inteira trunca em direcao a zero; resto conserva o sinal do dividendo. Divisao por zero e erro tambem para reais. As verificacoes continuam ativas em `-O3`.

Operacoes de bits exigem `speed` e palavras de 64 bits em complemento de dois. `<<` descarta bits altos; `>>` propaga o sinal. Contadores fora de 0..63 sao erros. Bits descartados nao produzem overflow aritmetico.

## Fluxo, funcoes e objetos

`scarlet`/`vodka` sao condicionais, `goldship` e while e `kitasan` e for. Condicoes exigem track. `rice` interrompe e `urara` continua o laco mais interno. `special` retorna da funcao. Todo caminho que pode terminar uma funcao com retorno precisa retornar um valor compativel.

`rudolf` declara classe; `diamond` especifica uma unica base. `condor` e publico e padrao; `groove` e privado na classe declarante. `opera` e construtor; a base pode ser inicializada com `: mcqueen(...)`. `tachyon` aloca a instancia. `teio` referencia o objeto atual e `mcqueen.metodo()` invoca a base sem despacho derivado.

Metodos publicos usam polimorfismo por referencia. Sobrescritas conservam assinatura e visibilidade. Campos nao podem ocultar membros herdados. Inicializa-se a base antes dos campos/corpo derivados. Campos derivados acessados antecipadamente por uma chamada virtual produzem erro. Globais sao inicializadas em ordem; leitura antecipada tambem produz erro.

## Modulos

```uma
import "utilidades.uma";
import "std/math";
```

Imports aparecem no escopo global. Caminhos locais sao relativos ao arquivo importador; `.uma` e opcional. `std/` usa a biblioteca da instalacao. Caminhos canonicos deduplicam imports. Ciclos e nomes globais duplicados sao rejeitados. Nao existem namespaces/exportacoes seletivas. Limites: 16 MiB de fontes, 512 arquivos e 64 niveis de importacao.

## Biblioteca padrao

Funcoes publicas em `stdlib/` possuem comentarios `///` com contratos visiveis no VSCode. As nativas originais continuam disponiveis: bakushin, suzuka, oguri, falcon, dober, taiki, creek, maruzensky, hayahide, ticket e flash.

| Modulo | API |
| --- | --- |
| math | min, max, clamp, abs, sqrt, pow, sin, cos, floor, ceil, log, pi, gcd |
| text | substring, indexOf, contains, startsWith, endsWith, split, join, trim |
| collections | sum, findIndex, reverse, copy, sort, binarySearch, IntStack, StringIntMap |
| io | readText, writeText, appendText, exists, readBytes, writeBytes |
| time | nanoTime, sleep, Stopwatch |
| memory | ByteBuffer |
| random | Random |
| system | environment |

`sort` usa heapsort: O(n log n) no pior caso e O(1) memoria auxiliar. `binarySearch` exige array ordenado. `copy` cria armazenamento independente. StringIntMap mantem chaves ordenadas: busca em O(log n) comparacoes, insercao/remocao O(n). Pilha e algoritmos de arrays sao especializados para inteiros.

Textos contam valores escalares Unicode, nao bytes ou grafemas. Nao ha normalizacao nem colacao por idioma. `substring` aceita inicio igual ao comprimento para quantidade zero. `split` preserva campos vazios e rejeita delimitador vazio. `trim` remove apenas espaco ASCII, tab, CR e LF.

Arquivos de texto exigem UTF-8. Operacoes sao sincronas e leem/gravam o arquivo inteiro; falhas de E/S encerram o programa. Bytes binarios sao inteiros 0..255. ByteBuffer e logico: cada byte ocupa um speed, e nao um byte fisico. Palavras usam little-endian e o intervalo inteiro e verificado antes da escrita.

Random usa Park-Miller com semente explicita e nao e criptografico. nextInt usa rejeicao para evitar vies de modulo. nanoTime e monotono e sua origem nao representa data civil.

## Comandos

| Comando | Resultado |
| --- | --- |
| `run arquivo.uma` | Verifica, gera bytecode e executa na VM. |
| `check arquivo.uma` | Verifica sintaxe, tipos e bytecode sem executar. |
| `compile arquivo.uma -o arquivo.umac` | Bytecode portavel com CRC-32. |
| `build arquivo.uma` | Executavel com bytecode e VM incorporados. |
| `build arquivo.uma --native` | C++17 tipado e GCC/Clang -O3, sem VM no resultado. |
| `build arquivo.uma --native --debug` | AOT com -O0 -g e verificacoes. |
| `emit-cpp arquivo.uma -o programa.cpp` | C++17 autossuficiente. |
| `inspect arquivo.uma` | JSON de assinaturas, tipos, membros e locais. |
| `check arquivo.uma --json --stdin` | Analisa o buffer de stdin no contexto do arquivo. |
| `test pasta` | Executa cada *.test.uma em processo isolado. |
| `new nome` | Cria um projeto inicial. |
| `disasm arquivo.umac` | Lista instrucoes de bytecode. |

AOT exige C++17: `--cc caminho` ou UMA_CXX escolhe o executavel, sem argumentos adicionais nesse campo. Processos iniciam sem shell. Caminhos com espacos/acentos sao suportados. A saida so e substituida depois de compilar com sucesso; temporarios sao removidos.

Windows AOT liga estaticamente os runtimes GCC/C++. AOT compila para a plataforma local. Para outro alvo, exporte C++ e use uma ferramenta configurada para esse alvo. O binario Linux original e 2.0; `build.sh` o recompila como 3.0. Recursos novos nao podem ser combinados com runtime 2.0 de outro alvo.

Saidas: 0 sucesso, 1 erro de linguagem/execucao/teste, 2 erro de uso/ferramenta, 130 interrupcao na VM. max-steps/max-depth sao limites da VM/test runner. AOT limita a 512 chamadas ativas e responde a SIGINT nos lacos/chamadas; nao aceita esses limites via argumentos do executavel gerado.

## VSCode

O checker real recebe `inspect --stdin`; analises obsoletas sao canceladas e resultados antigos nao substituem os da edicao atual. Conclusao/navegacao complementam o indice com analise lexical tolerante a codigo incompleto. Durante erros temporarios, o ultimo indice de imports e preservado. Imports usam a versao salva em disco, mesmo se outra aba tiver alteracoes.

Ha hover tecnico, assinaturas, parametros, conclusao de nomes/membros, visibilidade, definicoes, outline, snippets e comandos de terminal. Colunas UTF-8 viram UTF-16 do editor. `///` imediatamente antes de uma declaracao documenta o simbolo. Receptores simples como instancia., teio. e mcqueen. sao suportados; cadeias complexas podem exigir codigo completo. Nao ha servidor LSP separado, rename semantico ou adaptador de depuracao proprio. Para depurar AOT, use emit-cpp e ferramentas C++.

## Limites atuais

Ha codigo nativo, inteiros fixos, bits e formatos binarios. Esta versao ainda nao substitui C como linguagem de sistemas: faltam ponteiros brutos, FFI C, structs com layout ABI, inteiros sem sinal de varios tamanhos, alocacao manual e execucao sem runtime.

Tambem nao ha genericos de usuario, interfaces, traits, classes abstratas, sobrecarga, excecoes capturaveis, closures, async, threads da linguagem, sockets, gerenciador de pacotes, compilacao incremental ou debugger proprio. Portanto, nao e uma plataforma industrial completa.

Objetos/arrays usam contagem de referencias. Ciclos sao desfeitos no fim do processo, nao durante a execucao; programas longos que abandonam ciclos podem acumular memoria. Verificacoes e gerenciamento automatico tem custos diferentes de C/C++ sem essas verificacoes. Comparacoes de desempenho exigem programas e cargas equivalentes.
