# Estado do relatorio

| Area | Estado 3.1 |
| --- | --- |
| Retencao de weak_ptr e ciclos | Compactacao e trial deletion durante execucao, incluindo closures |
| Texto | Copy-on-write, ASCII/Unicode indexados em cache e append amortizado |
| VM | Locais indexados, operadores inteiros tipados e Value de 24 bytes; faltam 16 bytes, campos indexados e pilha unica |
| AOT | Overflow intrinsics, Frames recursivos/dinamicos, campos simples sem initialized, caminhos relativos, #line, O1 e cache GCC; bounds elimination/refcount intrusivo pendentes |
| Mapa e sort | Hash aberto generico em UmaScript; introsort e comparador estavel |
| Erros | treino/tropeco/desaquecer/desclassificar; catch recebe mensagem wit |
| Funcoes e genericos | Callbacks, closures por valor e monomorfizacao; sem inferencia/constraints |
| Modulos | Alias e isolamento; exportacao seletiva/privacidade pendentes |
| Interfaces e constantes | estilo/adota e trofeu; sem heranca de interfaces |
| Ownership/borrowing | Optativo, lexical e conservador; limites em OWNERSHIP.md |
| Inicializacao | Analise de dependencias de globais; protecoes dinamicas preservadas |
| Recursao | Limite configuravel na VM ate 4096; AOT com 4096 e pilha maior, sem checagem por endereco |
| Conforto | foreach, multiplas declaracoes, hex/bin, separadores e operadores circulares |
| Processo/stdin | argv, saida, stderr, EOF e lerTudo; exec e scanner de inteiros pendentes |
| Texto/biblioteca | replace, caixa ASCII, regex e Narrador; format/interpolacao/caixa Unicode completa pendentes |
| Arquivos/dados | Diretorios, caminhos, JSON estrito, CSV e base64; streams pendentes |
| Tempo/aleatorio | UTC/monotono e fonte do sistema; calendario/fusos, PRNG moderno e SHA-256 pendentes |
| Testes | VM/AOT, ciclos e LSP ampliados; fixtures e JUnit na CLI pendentes |
| Entrega | Binarios locais Windows/Linux 3.1, ZIP limpo, GCC no Windows completo e hashes |
| Vocabulario | palavras.toml gera compilador, dados do editor, regra da gramatica e tabela |
| Ferramentas | REPL, formatador e LSP basico; referencias e renomeacao semantica pendentes |
| Benchmarks/vitrine | 11 cargas reproduziveis e simulador de corrida de turnos |

Ainda pendentes: concorrencia/canais, rede, FFI, pacotes Git/lockfile, enums, structs por valor, static, tipos opcionais, referencias fracas, switch, byte compacto e playground WebAssembly.

O workflow de CI foi preparado para Windows/Linux. A execucao hospedada e publicacao remota exigem um repositorio; nao foram presumidos resultados de CI remoto.
