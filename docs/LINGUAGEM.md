# Referência da linguagem

> Recursos 3.0 (AOT, modulos, bits e biblioteca): [REFERENCIA_3.md](REFERENCIA_3.md). Este documento conserva a semantica original de tipos e OOP.

## Arquivos e nomes

Arquivos `.uma` são textos UTF-8. É permitido BOM no começo do arquivo. A linguagem diferencia maiúsculas de minúsculas. Identificadores seguem `[A-Za-z_][A-Za-z0-9_]*`; textos e caracteres aceitam Unicode. Nomes de funções globais e classes não podem ser reutilizados como nomes de variáveis ou parâmetros.

Comentários usam `//` até o final da linha ou `/* ... */`. Comentários de bloco não são aninhados. Instruções terminam com `;`. Classes podem terminar com um `;` opcional. Corpos de funções, laços e condicionais usam `{ ... }` obrigatoriamente. `vodka scarlet` permite encadear condições.

## Declarações e valores

```uma
speed pontos = 42;
stamina energia = 95.5;
guts categoria = 'A';
wit nome = "Special Week";
track pronta = turf;
speed[] tempos = [60, 62, 59];
stamina[][] matriz = [[1, 2.5], []];
```

Cada declaração contém exatamente uma variável. `speed a = 1, b = 2;` não é aceito; use duas declarações.

`speed` vai de `-9223372036854775808` a `9223372036854775807`. Literais numéricos são decimais; ponto decimal ou expoente (`1.5`, `2e3`) produz `stamina`. Não há literais hexadecimais nem sufixos numéricos. `speed` é inteiro de 64 bits; overflow é um erro, sem retorno silencioso ao começo do intervalo. `stamina` usa `double`; resultados infinitos ou NaN são rejeitados.

Textos usam aspas duplas e caracteres usam aspas simples. Escapes aceitos: `\n`, `\r`, `\t`, `\0`, `\\`, `\"`, `\'` e `\uXXXX`. O escape `\u` tem quatro dígitos e representa um valor escalar, sem pares substitutos; caracteres fora do plano básico, como `'😀'`, podem ser escritos diretamente em UTF-8. `guts` contém um valor escalar, que pode ocupar vários bytes. Uma sequência visual formada por vários valores escalares não é um único `guts`.

Variáveis e campos sem inicializador recebem os valores iniciais indicados no README. Cada declaração de array sem inicializador cria um array vazio independente. Referências a objetos começam em `nature`. Acesso a membro ou índice por uma referência `nature` é erro de execução.

## Tipagem

A análise de tipos ocorre antes de qualquer saída ou efeito do programa. Ela inclui todos os corpos de funções e ramos, mesmo quando não são executados.

As conversões implícitas permitidas são identidade, `speed -> stamina`, classe derivada para classe base e `nature` para objeto ou array. Arrays são invariantes: `Filha[]` não pode ser atribuído a `Base[]`. Para construir um array da base, declare `Base[] itens = [tachyon Filha(...)];`.

Literais de array usam o tipo esperado na declaração, atribuição, argumento ou retorno. Isso permite `stamina[] a = [1, 2];` e `speed[][] a = [[], [1]];`. Fora de um contexto tipado, um literal não vazio obtém um tipo comum de seus elementos: tipos idênticos ou mistura numérica promovida a `stamina`. Um array vazio exige contexto tipado. Essa resolução de literais não elimina a obrigação de declarar os tipos de variáveis e funções.

```uma
stamina metade(stamina valor) {
    special valor / 2.0;
}
stamina resultado = metade(3); // speed pode ser convertido em stamina.
// speed errado = resultado; // Rejeitado: exige taiki(resultado).
```

## Operadores

Da maior para a menor precedência:

| Nível | Operadores | Associação |
| --- | --- | --- |
| 1 | Chamada `()`, acesso `.`, índice `[]`, pós `++`, `--` | Esquerda |
| 2 | Unários `!`, `+`, `-`, pré `++`, `--`, criação `tachyon` | Direita |
| 3 | `*`, `/`, `%` | Esquerda |
| 4 | `+`, `-` | Esquerda |
| 5 | `<`, `<=`, `>`, `>=` | Esquerda |
| 6 | `==`, `!=` | Esquerda |
| 7 | `&&` | Esquerda |
| 8 | `||` | Esquerda |
| 9 | `=`, `+=`, `-=`, `*=`, `/=`, `%=` | Direita |

`&&`, `||` e `!` exigem `track`. `&&` e `||` fazem curto-circuito. Operadores aritméticos exigem números, com exceção de `+` para `wit + wit`, `wit + guts` ou `guts + wit`, cujo resultado é `wit`. Para concatenar um número, use `maruzensky(numero)`.

`speed / speed` produz um `speed`, truncado em direção a zero. `%` aceita somente `speed` e o sinal do resto acompanha o dividendo. Divisão ou resto por zero são erros. `MIN_SPEED / -1` causa overflow; `MIN_SPEED % -1` resulta em zero.

Comparações numéricas aceitam `speed` e `stamina`. Comparações entre dois inteiros mantêm todos os 64 bits; comparações entre inteiro e real não arredondam o inteiro antes de decidir o resultado. Textos ou caracteres do mesmo tipo podem ser ordenados. Igualdade entre objetos e entre arrays compara identidade, não conteúdo. Textos e caracteres comparam seu conteúdo. Não há normalização Unicode automática.

Argumentos, operandos e elementos de arrays são avaliados da esquerda para a direita. Em uma atribuição, o destino é localizado antes da avaliação do lado direito. Na atribuição composta, o valor antigo também é lido antes do lado direito.

Objetos e arrays são referências. Assim, `bakushin(array, dober(array))` avalia os dois argumentos antes de imprimir, e o primeiro argumento já reflete a retirada feita pelo segundo. Textos são valores imutáveis; `texto[i]` devolve `guts` e não pode receber atribuição.

## Fluxo e funções

```uma
speed dobrar(speed valor) {
    special valor * 2;
}
power anunciar(wit mensagem) {
    bakushin(mensagem);
    special;
}

kitasan (speed i = 0; i < 10; i++) {
    scarlet (i == 2) { urara; }
    scarlet (i == 7) { rice; }
    anunciar(maruzensky(dobrar(i)));
}
```

`special` encerra a função ou método atual. Retornos de `power` usam `special;`, sem expressão, ou chegam ao final do corpo. Uma função de outro tipo precisa devolver um valor compatível em todo caminho que possa alcançar seu final.

Uma repetição com condição literal `turf`, ou um `kitasan` sem condição, é tratada como potencialmente infinita. Se não houver caminho com `rice`, ela não pode alcançar o final da função e é aceita mesmo sem retorno. Outros casos são analisados de forma conservadora: uma condição calculada que sempre seja verdadeira pode exigir um retorno adicional.

Funções só podem ser declaradas no nível global; métodos pertencem a classes. Não há funções aninhadas, lambdas nem valores de função armazenáveis. A chamada deve fornecer exatamente os parâmetros declarados, sem argumentos padrão ou sobrecarga.

Escopos são léxicos. Blocos podem ocultar variáveis de blocos externos. Parâmetros e variáveis no corpo imediato de uma função compartilham escopo. O inicializador de uma variável é avaliado antes de o novo nome entrar em escopo. Variáveis do inicializador de `kitasan` ficam disponíveis na condição, atualização e corpo, mas não após o laço.

Funções e métodos enxergam as variáveis globais declaradas no programa. Se uma chamada tentar ler uma variável global antes de sua declaração ter sido executada, ocorre erro de execução. Instruções do nível global só enxergam declarações de variável anteriores.

## Classes

Uma classe pode declarar campos tipados, métodos tipados e um construtor `opera`. A visibilidade padrão é `condor`, pública. `groove` restringe o acesso ao código da própria classe que declarou o membro; subclasses não têm acesso direto aos membros privados da base.

Todos os campos são acessados explicitamente por uma referência: `teio.nome`, `outra.nome` ou `mcqueen.nome`. Não há campos estáticos, propriedades automáticas, interfaces, classes abstratas ou múltiplas bases.

Se não houver construtor declarado, a classe recebe um construtor sem parâmetros. O construtor implícito só é permitido quando a base também pode ser construída sem argumentos e de forma acessível. Para passar argumentos à base:

```uma
opera(speed valor) : mcqueen(valor) {
    // Corpo do construtor da classe derivada.
}
```

Sem `: mcqueen(...)`, o construtor da base é chamado sem argumentos. A análise rejeita uma base que exija argumentos ausentes ou tenha construtor privado.

A ordem de inicialização é:

1. Avaliar os argumentos do construtor da base no contexto dos parâmetros do construtor derivado.
2. Inicializar a base e executar seu construtor.
3. Criar os campos próprios da classe com valores iniciais.
4. Executar os inicializadores explícitos de campos, na ordem em que aparecem.
5. Executar o corpo do construtor atual.

Inicializadores de campo enxergam globais e `teio`, mas não parâmetros do construtor. Antes da etapa 3, campos da classe derivada ainda não existem. Acessá-los antecipadamente, inclusive por uma chamada virtual no construtor da base, produz erro de execução.

Uma sobrescrita pública deve conservar exatamente o tipo de retorno e os tipos/quantidade dos parâmetros. Nomes dos parâmetros podem mudar. Não é permitido reduzir a visibilidade de um método público, ocultar campos herdados ou reutilizar o nome de um método privado herdado.

Chamadas normais usam a implementação do tipo real do objeto. `mcqueen.metodo()` começa a busca na base da classe que contém o código em execução. Construtores não são chamados por acesso de membro; a chamada da base usa exclusivamente `: mcqueen(...)`.

## Funções nativas

| Chamada | Retorno e comportamento |
| --- | --- |
| `bakushin(v1, v2, ...)` | `power`. Aceita zero ou mais valores; separados por espaço, seguidos de quebra de linha. Objetos são mostrados como `<Classe>` e arrays como `[a, b]`. |
| `suzuka()` | `wit`. Lê uma linha sem sua quebra final. Fim da entrada é erro. |
| `oguri(witOuArray)` | `speed`. Conta valores escalares Unicode em texto ou elementos no array. |
| `falcon(T[] array, T valor)` | `power`. Acrescenta ao final; permite as conversões implícitas compatíveis com `T`. |
| `dober(T[] array)` | `T`. Retira o último elemento; array vazio é erro. |
| `taiki(speedOuStaminaOuWit)` | `speed`. Reais são truncados em direção a zero. Textos devem representar um inteiro decimal. Intervalo inválido é erro. |
| `creek(speedOuStaminaOuWit)` | `stamina`. Converte números ou texto numérico. Rejeita NaN e infinito. |
| `maruzensky(valor)` | `wit`. Usa a mesma representação textual de `bakushin`. |
| `hayahide(guts caractere)` | `speed`. Retorna o código Unicode. |
| `ticket(speed codigo)` | `guts`. Rejeita códigos fora do intervalo Unicode e substitutos. |
| `flash(track condicao)` | `power`. Erro se a condição for `dirt`. |
| `flash(track condicao, wit mensagem)` | `power`. Mesmo comportamento, com mensagem personalizada. |

A resolução de `T` na tabela é interna às funções nativas; o programador não declara genéricos nesta versão. Arrays aceitam índices de `0` a `oguri(array) - 1`. Índices negativos ou fora do intervalo são erros. Adicionar/retirar elementos pode afetar aliases do mesmo array.

## Recursos e limites

A UmaScript 2 verifica os tipos e compila a árvore sintática para bytecode. A VM nativa executa essas instruções. O programa `.uma` não tem APIs para executar comandos do sistema ou importar código C++. Consulte `COMPILADOR.md` para a ferramenta de terminal e a geração de executáveis.

Objetos e arrays usam referências contadas. Ciclos são desfeitos quando o programa termina, mas não são coletados durante a execução. Um programa longo que cria e abandona muitos ciclos pode consumir memória crescente. Não há destrutores definidos pelo usuário.

Existem limites de aninhamento sintático e profundidade de expressão para proteger a pilha da implementação, além do limite configurável de chamadas. A quantidade de iterações de laços não possui limite padrão. `--max-steps` é opcional e conta instruções da VM, não linhas `.uma`.
