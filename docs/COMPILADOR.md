# Compilador UmaScript 2

> Referencia historica do modo bytecode. Para os comandos 3.0, AOT e instalacao atual, consulte [REFERENCIA_3.md](REFERENCIA_3.md) e o README principal.

O pacote inclui dois compiladores nativos prontos, para Windows x64 e Linux x86-64. Instalar, verificar, compilar e executar programas não requer Python, Node.js, Java, .NET, g++, Visual Studio ou acesso à internet. Os scripts `build.sh` e `build.bat` são opcionais e servem para quem deseja alterar a implementação.

## O que a compilação produz

1. O lexer identifica os tokens do `.uma`.
2. O parser constrói a árvore sintática.
3. O verificador valida os tipos do programa inteiro, incluindo funções e ramos não executados.
4. O compilador transforma a árvore em instruções de uma máquina de pilha. Condições e laços viram saltos; expressões viram operações de pilha.
5. A árvore é descartada. A VM executa as instruções compiladas.

`uma compile treino.uma` grava essas instruções em `treino.umac`. Esse arquivo é binário, versionado, inclui assinaturas e posições para diagnóstico e usa CRC-32 para detectar corrupção. Não contém o código-fonte original nem uma árvore sintática serializada. Literais de texto e nomes necessários ao programa continuam presentes. O bytecode não é criptografia nem proteção contra engenharia reversa.

`uma build treino.uma` combina o bytecode com um runtime nativo da plataforma escolhida. O resultado é um executável PE no Windows ou ELF no Linux, com a VM embutida. Nenhum compilador externo é iniciado e nenhum executável temporário é extraído em tempo de execução.

**O backend compila para bytecode. Não é um backend AOT que traduz cada operação UmaScript diretamente para instruções de máquina x86-64.** O arquivo distribuível é um executável nativo que contém e executa o bytecode. Ele funciona sem a instalação da UmaScript, sem o `.uma`, sem o `.umac` separado e sem runtimes de outras linguagens.

## Comandos

| Comando | Resultado |
| --- | --- |
| `uma --version` | Versão e plataforma do compilador |
| `uma --help` | Ajuda completa |
| `uma new corrida` | Cria `corrida/main.uma` e um README |
| `uma run main.uma` | Compila em memória e executa |
| `uma run main.umac` | Carrega e executa bytecode pronto |
| `uma check main.uma` | Verifica sintaxe, tipos e estrutura das instruções sem executar |
| `uma check main.umac` | Verifica formato, CRC e estrutura do bytecode |
| `uma compile main.uma` | Gera `main.umac` |
| `uma compile main.uma -o corrida.umac` | Escolhe o nome do bytecode |
| `uma build main.uma` | Gera `main.exe` no Windows ou `main` no Linux |
| `uma build main.umac -o corrida.exe --target windows-x64` | Gera um executável Windows a partir de bytecode |
| `uma build main.uma -o corrida --target linux-x64` | Gera um executável Linux |
| `uma disasm main.umac` | Mostra instruções, saltos e posições de código |
| `uma disasm main.uma -o main.asm` | Salva a listagem das instruções |

O atalho `uma main.uma` equivale a `uma run main.uma`. O atalho antigo `uma --check main.uma` também funciona. Use aspas em nomes de arquivos ou pastas com espaços.

`-o` pode sobrescrever um arquivo de saída existente. Há proteção contra substituir o arquivo de entrada, o compilador em execução e o runtime utilizado. A gravação usa um arquivo temporário na pasta de destino e uma substituição ao concluir.

## Compilação para outro sistema

Os alvos são `windows-x64` e `linux-x64`. O pacote e os instaladores incluem os dois runtimes, portanto ambos podem ser gerados em qualquer um dos sistemas sem instalar uma ferramenta de compilação cruzada.

```sh
uma build main.uma --target windows-x64 -o corrida.exe
uma build main.uma --target linux-x64 -o corrida
```

Um arquivo `.umac` funciona com a UmaScript 2 em ambos os sistemas. Um `.exe` deve ser executado no Windows; um ELF deve ser executado no Linux. A compilação para outro sistema não permite executar o binário desse sistema no host.

O binário Linux é estático. O Windows usa bibliotecas de sistema, com as bibliotecas de suporte de C++ ligadas estaticamente. Não há DLL de MinGW, libstdc++ ou Python para copiar.

## Entrada, saída e limites

`bakushin` escreve em stdout; `suzuka` lê stdin. Redirecionamentos normais do terminal funcionam. Texto-fonte, bytecode e mensagens usam UTF-8; o executável Windows configura o console para UTF-8 e aceita caminhos Unicode.

```sh
uma run main.uma --max-steps 1000000 --max-depth 128
uma run main.umac --max-steps 1000000
```

Programas gerados também aceitam `--max-steps N` e `--max-depth N`. Não há API de argumentos do programa na linguagem nesta versão.

O limite padrão de passos é `0`, sem orçamento fixo. Um passo corresponde a uma instrução da VM, portanto os números diferem dos passos internos da versão 1. O limite padrão de chamadas é 256, ajustável entre 1 e 512. `Ctrl+C` interrompe a execução.

O fonte tem limite de 16 MiB, cada texto serializado tem limite de 1 MiB e o bytecode tem limite de 64 MiB. Esses limites se referem à representação do programa, não ao tamanho da fita ou das estruturas alocadas durante sua execução. A memória continua limitada pela máquina real.

Erros no fonte incluem arquivo, linha, coluna e trecho. Bytecode e executáveis preservam nomes de arquivo, linha, coluna e a pilha resumida, mas dispensam o fonte para rodar.

| Código de saída | Significado |
| --- | --- |
| `0` | Sucesso |
| `1` | Erro da linguagem ou falha de execução |
| `2` | Uso incorreto, erro de arquivo, instalação ou bytecode inválido |
| `130` | Interrupção por `Ctrl+C` |

## Instalação e remoção

No Windows, execute `Instalar_Windows.exe`. A instalação é por usuário, em `%LOCALAPPDATA%\Programs\UmaScript`, sem exigir privilégios de administrador. O instalador registra a UmaScript na lista de aplicativos e cria `Desinstalar.exe`. O comando nativo também pode ser iniciado pelo script alternativo `instalar-windows.cmd`; nesse caso, não há registro de aplicativo pelo instalador gráfico.

No Linux, execute `sh instalar-linux.sh`. O destino padrão é `~/.local/opt/umascript`. O instalador adiciona um bloco identificável ao `~/.profile` e ao `~/.bashrc`; atualiza também `~/.zshrc` quando esse arquivo existe. Feche e reabra o terminal após instalar. Em outros shells, acrescente a pasta instalada ao PATH usando a configuração do próprio shell.

Instalação em pasta personalizada:

```sh
sh instalar-linux.sh --prefix "$HOME/ferramentas/umascript"
```

`--no-path` instala sem alterar o PATH. O binário nativo também tem `uma install --prefix PASTA --package-root PASTA_DO_PACOTE` para instalação manual. Uma pasta de destino não vazia precisa conter uma instalação UmaScript reconhecida.

Para remover no Linux, use `uma uninstall`. Se escolheu outro destino, use `uma uninstall --prefix PASTA`. São removidos apenas os arquivos registrados na instalação e o bloco adicionado ao PATH; arquivos adicionais do usuário são preservados. No Windows, use **Aplicativos instalados** ou `Desinstalar.exe`. Na instalação alternativa por `.cmd`, `uma uninstall` remove o PATH e os arquivos que não estão em uso; apague `uma.exe` após encerrar o processo.

## Desenvolvimento opcional

Os arquivos `src/language.hpp`, `src/bytecode.hpp` e `src/uma.cpp` contêm toda a implementação. A licença do código UmaScript é MIT. Os avisos de bibliotecas incorporadas ficam em `docs/licenses`.

Para recompilar o compilador, é necessário C++17. No Linux, `sh build.sh` usa g++. No Windows, `build.bat` contempla MinGW-w64 e MSVC com ligação estática dos runtimes. Apenas o caminho MinGW-w64 foi compilado neste ambiente.

Para reconstruir o instalador gráfico, é necessário NSIS 3. Na raiz do projeto:

```sh
makensis -NOCD packaging/windows.nsi
```

Os testes opcionais de desenvolvimento usam Python 3. Isso não é uma dependência da instalação, da compilação de `.uma` ou da execução dos programas.

Consulte `VALIDACAO.md` para distinguir o que foi executado e o que foi verificado estruturalmente.
