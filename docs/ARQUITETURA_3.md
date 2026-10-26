# Arquitetura 3.0

`language.hpp` implementa lexer, parser, AST, checker e semantica da VM. `modules.hpp` resolve imports e atribui arquivo/linha/coluna aos tokens. O checker precede os dois backends.

`bytecode.hpp` gera instrucoes de pilha, serializa UMABC02 com CRC-32, verifica entradas e executa a VM. O formato antigo continua compativel; bits e primitivas novas exigem runtime 3.0. O formato nao preserva o arquivo individual de cada instrucao importada: erros de execucao da VM usam o nome do modulo de entrada e a linha da instrucao. Erros estaticos e AOT preservam o arquivo original.

`native_backend.hpp` baixa a AST tipada para C++17: int64_t, double, bool, char32_t, std::string e vetores tipados compartilhados. Classes C++ usam metodos virtuais e inicializacao explicita para preservar a construcao da linguagem. Lambdas e temporarios mantem avaliacao esquerda → direita; GCC/Clang pode inlinear e eliminar esses temporarios.

`native_runtime.hpp` fornece referencias verificadas, arrays, formatacao e gerenciamento de referencias. Nao ha Value dinamico nem dispatch de bytecode no caminho AOT. `native_ops.hpp` compartilha contratos de aritmetica, Unicode, arquivos e tempo com a VM.

`native_driver.hpp` descobre a ferramenta e inicia processos sem shell. Windows usa CreateProcessW, citacao de argumentos e diretorio Unicode; intermediarios recebem nomes ASCII para evitar limitacoes MinGW. A publicacao do executavel conserva a gravacao atomica existente.

`metadata.hpp` exporta o indice JSON. A extensao combina um servico lexical testavel em Node com providers do VSCode. Analises recebem o buffer, nao executam o programa, e sao canceladas quando obsoletas. Execucao explicita usa ProcessExecution.

Os testes cobrem comportamento legado, corrupcao de bytecode, executaveis autonomos, comparacao VM/AOT, erros, modulos e editor. `tests/stdlib` usa a propria linguagem. `benchmarks` compara o mesmo algoritmo numerico com C++, registrando amostras e ambiente.
