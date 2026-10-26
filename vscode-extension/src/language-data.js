'use strict';
const builtins = [
  ['bakushin', 'power bakushin(valor, ...)', 'Imprime valores em ordem de avaliacao, separados por espaco, e uma quebra de linha. Arrays usam [a, b], referencias nulas usam nature. Efeito: escreve em stdout.'],
  ['suzuka', 'wit suzuka()', 'Le uma linha UTF-8 de stdin, sem CR/LF finais. Fim de entrada ou codificacao invalida e erro. Operacao bloqueante.'],
  ['oguri', 'speed oguri(wit | T[] valor)', 'Comprimento: conta valores escalares Unicode em wit (O(1) apos indexacao inicial) e elementos em array (O(1)). nature produz erro.'],
  ['falcon', 'power falcon(T[] array, T valor)', 'Acrescenta valor ao final. Preserva o tipo T e aplica speed → stamina quando permitido. Altera todos os aliases do array. Custo amortizado O(1).'],
  ['dober', 'T dober(T[] array)', 'Remove e retorna o ultimo elemento. Array vazio ou nature produz erro de execucao. Altera aliases. O(1).'],
  ['taiki', 'speed taiki(speed | stamina | wit valor)', 'Converte para inteiro com sinal de 64 bits. Reais truncam em direcao a zero. Texto deve ser decimal valido. Fora do intervalo produz erro.'],
  ['creek', 'stamina creek(speed | stamina | wit valor)', 'Converte para IEEE 754 binary64. Inteiros grandes podem perder precisao. Texto invalido, NaN e infinito produzem erro.'],
  ['maruzensky', 'wit maruzensky(valor)', 'Cria representacao textual usando as mesmas regras de bakushin. Nao aceita power ou funcoes. Arrays ciclicos limitam a profundidade de formatacao.'],
  ['hayahide', 'speed hayahide(guts caractere)', 'Retorna o valor escalar Unicode do caractere, de 0 a 0x10FFFF, excluindo substitutos.'],
  ['ticket', 'guts ticket(speed codigo)', 'Converte valor escalar Unicode para caractere. Negativos, substitutos U+D800..U+DFFF e valores acima de U+10FFFF produzem erro.'],
  ['flash', 'power flash(track condicao, wit mensagem)', 'Verificacao em tempo de execucao, ativa tambem em compilacoes otimizadas. Se condicao for dirt, encerra com erro. A mensagem e opcional.'],
  ['uma_sqrt', 'stamina uma_sqrt(stamina valor)', 'Raiz quadrada real. Negativos produzem erro; NaN e infinito sao rejeitados.'],
  ['uma_pow', 'stamina uma_pow(stamina base, stamina expoente)', 'Potencia real. Resultados nao finitos produzem erro.'],
  ['uma_sin', 'stamina uma_sin(stamina radianos)', 'Seno real em radianos.'],
  ['uma_cos', 'stamina uma_cos(stamina radianos)', 'Cosseno real em radianos.'],
  ['uma_floor', 'stamina uma_floor(stamina valor)', 'Arredonda para menos infinito; resultado permanece stamina.'],
  ['uma_ceil', 'stamina uma_ceil(stamina valor)', 'Arredonda para mais infinito; resultado permanece stamina.'],
  ['uma_log', 'stamina uma_log(stamina valor)', 'Logaritmo natural. Exige valor positivo; resultado nao finito e erro.'],
  ['uma_read_text', 'wit uma_read_text(wit caminho)', 'Le todo o arquivo em UTF-8. Falhas de E/S e codificacao sao erros. Memoria O(tamanho do arquivo).'],
  ['uma_write_text', 'power uma_write_text(wit caminho, wit conteudo)', 'Substitui arquivo por conteudo UTF-8. Erros de abertura/escrita/fechamento sao reportados. Nao cria diretorios.'],
  ['uma_append_text', 'power uma_append_text(wit caminho, wit conteudo)', 'Acrescenta conteudo ao fim do arquivo; cria arquivo ausente. Falhas de E/S sao erros.'],
  ['uma_exists', 'track uma_exists(wit caminho)', 'Consulta existencia de caminho. Erros do sistema de arquivos sao reportados.'],
  ['uma_clock', 'speed uma_clock()', 'Relogio monotono em nanossegundos. Origem nao especificada: use diferencas, nao como data civil.'],
  ['uma_sleep', 'power uma_sleep(speed milissegundos)', 'Suspende a thread atual por uma duracao nao negativa em milissegundos.'],
  ['uma_slice', 'wit uma_slice(wit texto, speed inicio, speed quantidade)', 'Substring por valores escalares Unicode. Inicio inclusivo; quantidade nao negativa. Intervalo fora dos limites produz erro.'],
  ['uma_find', 'speed uma_find(wit texto, wit trecho)', 'Primeira ocorrencia como indice Unicode. Retorna -1 se ausente, 0 para trecho vazio.'],
  ['uma_env', 'wit uma_env(wit nome)', 'Consulta variavel de ambiente UTF-8. Ausente retorna texto vazio. Byte nulo no nome e erro.'],
  ['uma_read_bytes', 'speed[] uma_read_bytes(wit caminho)', 'Le arquivo binario em valores de 0 a 255. Cada byte logico ocupa um speed de 64 bits.'],
  ['uma_write_bytes', 'power uma_write_bytes(wit caminho, speed[] bytes)', 'Grava bytes em arquivo. Rejeita nature e elementos fora de 0..255 antes de abrir o arquivo.']
].map(([name, signature, documentation]) => ({ name, signature, documentation, kind: 'function', builtin: true, type: signature.split(' ')[0] }));
const words = {
  speed: 'Inteiro com sinal de 64 bits. Aritmetica verifica overflow; divisao trunca em direcao a zero.',
  stamina: 'Real IEEE 754 binary64. Conversao implicita de speed permitida. Resultados nao finitos sao rejeitados.',
  guts: 'Um valor escalar Unicode, incluindo U+0000. Indexacao de texto retorna guts.',
  wit: 'Texto UTF-8. Indices e comprimentos contam valores escalares, nao bytes nem grafemas.',
  track: 'Booleano: turf (verdadeiro) ou dirt (falso). Condicionais exigem track.',
  power: 'Ausencia de valor de retorno. Permitido como retorno de funcao/metodo.',
  rudolf: 'Declara classe de referencia com campos, metodos e construtor opera.',
  diamond: 'Heranca simples. Metodos publicos usam despacho virtual.',
  condor: 'Visibilidade publica, que tambem e o padrao.',
  groove: 'Visibilidade privada: acesso somente na classe que declarou o membro.',
  opera: 'Construtor. Base explicita: opera(...) : mcqueen(...) { ... }.',
  tachyon: 'Cria instancia e executa os construtores da base para a classe derivada.',
  teio: 'Referencia a instancia atual; existe em metodos e inicializadores de campos.',
  mcqueen: 'Referencia estatica a base. mcqueen.metodo() ignora a sobrescrita derivada.',
  scarlet: 'Condicional: scarlet(condicao track) { ... }.',
  vodka: 'Ramo alternativo de scarlet.',
  goldship: 'Laco enquanto a condicao track for turf.',
  kitasan: 'Laco contado: kitasan(inicio; condicao; atualizacao) { ... }.',
  rice: 'Interrompe o laco mais interno.', urara: 'Continua o laco mais interno, executando a atualizacao de kitasan.',
  special: 'Retorna da funcao/metodo. Funcoes power usam special; sem expressao.',
  import: 'Importa arquivo relativo ou std/modulo. Importacoes sao deduplicadas; ciclos sao erros.',
  nature: 'Referencia nula. Compativel com classes e arrays; acesso produz erro.',
  turf: 'Literal track verdadeiro.', dirt: 'Literal track falso.'
};
const vocabulary = require('./vocabulary.json').words;
for (const native of require('./native-data.json')) {
  if (!builtins.some(item => item.name === native.name)) builtins.push(native);
}
for (const [uma, classic] of Object.entries(vocabulary)) {
  if (!words[uma]) words[uma] = `UmaScript: ${uma}; equivalente classico: ${classic}.`;
  if (uma !== classic) words[classic] = `${classic}: dialeto classico de ${uma}. ${words[uma]}`;
}
module.exports = { builtins, words };
