# UmaScript 3.1

Linguagem com verificacao de tipos, ownership e borrowing optativos, genericos, interfaces, closures, modulos, bytecode e AOT.

## Sem instalar dependencias

Windows: extraia **UmaScript-3.1.0-windows-x64-completo.zip**, em dist/. Inclui o compilador C++ usado pelo AOT, suas bibliotecas e licencas. Nao exige instalar Python, Node, MSYS2 ou configurar PATH.

    .\uma.cmd --version
    .\uma.cmd run examples\11_ownership.uma
    .\uma.cmd build examples\11_ownership.uma --native -o exemplo.exe
    .\exemplo.exe

O pacote **portatil** e menor. Executar, verificar, compilar bytecode e produzir executaveis com VM embutida dispensam ferramentas externas. AOT no pacote portatil usa GCC/Clang instalado ou o pacote completo.

Linux x86-64: extraia o pacote portatil e execute:

    chmod +x bin/linux-x64/uma
    ./uma run examples/11_ownership.uma

O compilador Linux 3.1 e estatico. Execucao e geracao de programas com VM embutida dispensam GCC. AOT no Linux usa GCC/Clang do sistema; o executavel resultante e estatico.

Os scripts instalar-windows.cmd e instalar-linux.sh fazem a instalacao opcional por usuario. O instalador 2.0 antigo nao integra os novos pacotes.

## Ownership e recursos novos

    own speed[] tempos = [10, 20];
    {
        borrow speed[] leitura = &tempos;
        bakushin(leitura[0]);
    }
    {
        borrow mut speed[] escrita = &mut tempos;
        falcon(escrita, 30);
    }
    own speed[] finalistas = mover(tempos);
    bakushin(finalistas);
    // bakushin(tempos); // U0004: uso depois de mover

O verificador rejeita uso depois de movimento, aliases de donos, mutacao por borrow imutavel, conflitos e escape de emprestimos. Os emprestimos duram ate o fim do bloco. Referencias antigas continuam compartilhadas, com coleta de ciclos. [Contrato e limites](docs/OWNERSHIP.md).

    convocar "std/collections" como c;
    c.Mapa<wit, speed> pontos = tachyon c.Mapa<wit, speed>();
    pontos.set("Sakura", 42);
    tatica(speed)->speed dobro = tatica(speed n)->speed { special n * 2; };
    treino {
        bakushin(dobro(pontos.get("Sakura")));
    } tropeco (wit mensagem) {
        bakushin(mensagem);
    } desaquecer {
        bakushin("fim");
    }

Tambem ha Lista<T>, Conjunto<T>, funcoes genericas, estilo/adota, trofeu, foreach com em, multiplas declaracoes, hex/bin, separadores numericos e operadores circulares +%, -%, *%.

## Ferramentas

    uma run main.uma -- argumento1 "argumento 2"
    uma check main.uma --json
    uma compile main.uma -o main.umac
    uma build main.uma -o programa
    uma build main.uma --native --dev -o programa
    uma build main.uma --native --debug -o programa
    uma emit-cpp main.uma -o programa.cpp
    uma fmt main.uma --dialeto=classico
    uma repl
    uma lsp
    uma inspect main.uma
    uma test tests/stdlib

AOT usa cache GCC em .uma/cache; --no-cache o desativa e --dev usa O1. emit-cpp e autossuficiente, com caminhos relativos e #line. Recompile bytecode antigo: o formato atual e 3.

LSP: diagnosticos, hover, conclusao, simbolos e formatacao. Referencias e renomeacao semantica estao pendentes. O formatador normaliza recuo/vocabulario e preserva comentarios/literais, sem reorganizar expressoes em uma linha.

## Biblioteca

| Modulo | Recursos |
| --- | --- |
| collections | Lista<T>, Mapa<K,V> hash, Conjunto<T>, StringIntMap, IntStack e ordenacao |
| text | substring, busca, split/join, replace, caixa ASCII, regex ECMAScript e Narrador |
| io | arquivos UTF-8/binarios, EOF, lerTudo, diretorios e caminhos |
| system | argv UTF-8, codigo de saida, stderr e ambiente |
| json/csv/base64 | dados estruturados, CSV com campos multilinha e codificacao de bytes |
| math/time/random/memory | matematica, relogio monotono/UTC, PRNG anterior e bytes/palavras |

ordenar(array) usa introsort; ordenar(array, comparador) usa ordenacao estavel. O comparador tatica(T,T)->track deve definir ordem estrita e nao modificar o array. lower/upper transformam ASCII. Regex opera sobre bytes UTF-8. Operacoes Json reanalisam o documento.

## Validacao e limites

[Estado por area do relatorio](docs/STATUS_3_1.md) · [Validacao](docs/VALIDACAO_3_1.md) · [Vocabulario](docs/VOCABULARIO.md)

**Esta entrega nao implementa todo o relatorio.** Concorrencia/canais, rede, FFI, pacotes Git, structs por valor, enums, static, opcionais, playground WebAssembly e LSP completo continuam pendentes. A VM ainda nao tem pilha unica nem campos indexados. Nao ha promessa de velocidade equivalente ao Python ou C++.

Desenvolvimento: Python 3.11+, C++17 e Node para testes da extensao. Essas ferramentas nao sao exigidas para usar os pacotes portateis. palavras.toml e a fonte do vocabulario/versao; CI e entrega estao em .github/workflows/ci.yml e tools/package_release.py.

Projeto independente de fa, sem recursos oficiais do jogo. Codigo MIT; componentes externos preservam suas licencas.

