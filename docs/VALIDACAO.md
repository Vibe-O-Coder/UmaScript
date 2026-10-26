# Validação da UmaScript 2.0.0

> Relatorio historico da versao original. A validacao 3.0 esta em [VALIDACAO_3.md](VALIDACAO_3.md).

Data: 24 de outubro de 2026.

## Verificações executadas

- Compilação do binário Linux x86-64 com C++17, ligação estática e `-Wall -Wextra -Wpedantic -Werror`.
- Compilação cruzada do binário Windows x64 com MinGW-w64, runtimes C++ estáticos e os mesmos avisos tratados como erro.
- **152 casos funcionais em cada um de três modos**, totalizando **456 casos**: fonte compilado em memória, bytecode salvo e executável independente.
- **117 verificações de integração**, incluindo bytecode determinístico, listagem de instruções, rejeição de arquivos corrompidos, proteção do fonte, instalação, atualização do PATH, reinstalação, remoção e preservação de arquivos adicionais do usuário.
- Em cada modo funcional, todos os nove exemplos foram verificados e executados. Cada execução da suíte inclui 180 operações inteiras comparadas a resultados calculados independentemente.
- A suíte funcional foi executada com AddressSanitizer e UndefinedBehaviorSanitizer nos modos fonte e bytecode: **304 casos adicionais**, sem erros detectados. A detecção de vazamentos foi desativada porque o ambiente não oferece o suporte necessário ao LeakSanitizer.
- 256 mutações de bytecode com CRC recalculado foram carregadas com os sanitizadores ativos, sem erros de memória ou comportamento indefinido detectados.
- Execução de um programa gerado em outra pasta, com o fonte apagado, sem arquivos auxiliares e com PATH vazio.
- Instalação Linux em HOME isolado, com espaços e apóstrofos no caminho; preservação das permissões do perfil e ausência de blocos duplicados no PATH.
- Geração de PE para Windows e ELF para Linux por `uma build --target`.
- Inspeção das importações do executável Windows: ADVAPI32, KERNEL32, msvcrt, ole32, SHELL32 e USER32. Nenhuma DLL de suporte de MinGW, libstdc++ ou outra linguagem precisa acompanhar o programa.
- Construção do instalador gráfico NSIS para Windows.

## Limites da validação de plataformas

**Linux:** compilação, instalação, PATH, compilação de programas, bytecode e execução de programas gerados foram verificados neste ambiente x86-64.

**Windows:** o pacote contém `uma.exe` e o instalador já compilados. Foram verificados compilação cruzada, formato PE e importações. Não foi possível executar os binários ou testar o instalador numa sessão Windows: o Wine disponível exige sockets locais que o ambiente bloqueia. A execução e o registro no PATH do Windows ainda precisam de verificação em uma máquina Windows real.

O caminho de recompilação por MSVC está incluído em `build.bat`, mas não foi executado. A compilação usada para entregar o binário Windows foi MinGW-w64.

Os binários prontos são de 64 bits, para Windows x64 e Linux x86-64. Não há executáveis ARM ou de 32 bits neste pacote.

## Comportamentos cobertos

Tipos explícitos, promoção numérica, limites inteiros, overflow, comparação numérica, divisão, precedência, escopos, recursão, retornos, curto-circuito, laços, `rice`, `urara`, arrays, matrizes, aliases, alterações durante atribuições, referências nulas, Unicode, entrada, conversões, classes, construtores, herança, polimorfismo e encapsulamento.

As verificações de tipo continuam acontecendo antes de executar o programa inteiro. Funções não chamadas e ramos não tomados também são verificados.

## Limites conhecidos

- O backend gera bytecode, executado por uma VM nativa. Não é tradução AOT de cada operação para x86-64.
- `speed` tem 64 bits, e `stamina` usa double de 64 bits nas plataformas entregues.
- Ciclos de objetos e arrays são liberados ao encerrar; não há coleta incremental desses ciclos.
- Não há módulos, sobrecarga, genéricos, funções como valores, exceções capturáveis em UmaScript, acesso a arquivos, chamadas de sistema ou API de argumentos.
- O checksum detecta corrupção; não torna um arquivo compilado confiável nem impede engenharia reversa.
- A completude de Turing considera o modelo com memória ilimitada. A implementação real depende da memória disponível, e testes finitos não substituem uma prova desse modelo.

## Reproduzir os testes opcionais

Com Python 3 disponível apenas na máquina de desenvolvimento:

```sh
python3 tests/run_tests.py ./bin/linux-x64/uma source
python3 tests/run_tests.py ./bin/linux-x64/uma bytecode
python3 tests/run_tests.py ./bin/linux-x64/uma executable
python3 tests/test_compiler.py ./bin/linux-x64/uma
```

A instalação e o uso normal de UmaScript não requerem Python.

