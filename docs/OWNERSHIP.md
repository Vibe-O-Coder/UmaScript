# Ownership e borrowing 3.1

own T nome = inicializador cria um vinculo exclusivo para array ou instancia de classe. O inicializador e uma alocacao nova ou mover(outroDono). Texto e demais escalares mantem semantica de valor.

mover transfere para outro own ou parametro own, invalidando a origem. Reatribuir uma alocacao nova restaura a origem. borrow T leitura = &origem permite varias leituras. borrow mut T escrita = &mut origem exige exclusividade. O emprestimo dura ate o fim do bloco.

    power incrementar(borrow mut speed[] valores) { valores[0]++; }
    speed tamanho(borrow speed[] valores) { special oguri(valores); }
    own speed[] valores = [1, 2];
    incrementar(&mut valores);
    bakushin(tamanho(&valores));

Parametros own exigem movimento explicito ou alocacao nova. Parametros borrow exigem & ou &mut correspondente. Argumentos da mesma chamada sao verificados para conflitos.

Donos/emprestimos nao podem ser copiados para vinculos comuns, guardados em outros objetos/arrays, capturados por lambdas ou devolvidos como referencias. Classes usadas com own/borrow sao verificadas para impedir escape de teio. Seus construtores e chamadas de metodos recebem dados escalares nesta versao.

Ramos sao unidos conservadoramente. Movimento dentro de laco exige reinicializacao antes da proxima iteracao. Funcoes nao capturam globais own.

## Limites

- Ownership e optativo; referencias antigas continuam compartilhadas.
- Sem campos own/borrow, retornos own, reemprestimos locais ou lifetimes explicitos.
- Sem conversao de referencia compartilhada existente para own.
- Tipos de funcao nao transportam qualificadores de ownership; use funcoes nomeadas diretamente.
- Metodos sao potencialmente mutaveis. borrow imutavel permite acesso direto a campos, nao chamadas arbitrarias de metodos.
- Este contrato conservador nao e o modelo completo de Rust.

VM e AOT aplicam o mesmo verificador. Bytecode carregado mantem verificacoes dinamicas sem reconstruir a prova estatica de ownership.
