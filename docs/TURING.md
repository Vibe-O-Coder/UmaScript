# Por que UmaScript é Turing completa

Esta é uma justificativa sobre o poder de expressão da linguagem, considerando memória sem limite. Não é uma promessa de memória infinita na implementação física.

Uma máquina de dois contadores pode realizar computação universal usando dois contadores naturais sem limite e um programa finito composto de:

- Incrementar um contador e avançar para outra instrução.
- Testar se um contador é zero; se não for, decrementá-lo; escolher a próxima instrução conforme o resultado.

UmaScript expressa cada contador como uma cadeia de objetos `Elo`. Zero é `nature`. Incrementar cria um elo cujo campo `anterior` aponta para a cadeia atual. Decrementar substitui o topo por `topo.anterior`. Testar zero compara o topo com `nature`.

Nenhuma dessas três operações exige armazenar o valor do contador em um `speed`. Assim, o intervalo fixo dos inteiros da implementação não impõe um limite semântico ao comprimento da cadeia. Um controlador finito usa `scarlet`, atribuições de estado e `goldship` para selecionar e repetir as instruções. A simulação de uma máquina universal fixa precisa de um número fixo e pequeno de estados; não depende de recursão crescente.

`examples/07_contadores.uma` implementa os contadores e um programa que transfere unidades de B para A. Ele demonstra as operações da construção. O exemplo concreto de soma não é, sozinho, um programa universal; a universalidade está na possibilidade de expressar qualquer programa dessas operações, inclusive um simulador universal.

Também existe uma construção direta por fita de Turing: objetos `Celula` armazenam um símbolo e referências às vizinhas. Um movimento para uma posição ainda inexistente cria outra célula. `guts` fornece um alfabeto finito e condições selecionam as transições. A posição da cabeça é uma referência, sem índice numérico para limitar a extensão da fita.

`examples/09_maquina_turing.uma` executa uma máquina simples que transforma a fita `111` em `1111`. Para um simulador geral, as transições podem ser representadas por objetos ligados ou por código de controle finito. O exemplo é deliberadamente pequeno e verificável.

Na execução real, alocações podem falhar e o usuário pode interromper o programa. O limite opcional `--max-steps` encerra a avaliação após um orçamento; seu valor padrão é zero, sem orçamento fixo. Esses limites físicos ou operacionais não mudam a classificação do modelo de linguagem.
