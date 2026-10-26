# Benchmarks 3.1

Medianas de tres amostras no Windows, em milissegundos. Incluem inicio do processo; VM inclui leitura e verificacao do fonte. As saidas foram iguais nos dois backends. Sem comparacao com Python nesta medicao.

| Carga | VM (ms) | Nativo (ms) |
| --- | ---: | ---: |
| integer_sum | 179.32 | 19.31 |
| nested_loops | 250.77 | 18.70 |
| fibonacci | 1552.97 | 18.52 |
| sort | 156.53 | 21.14 |
| sieve | 798.20 | 39.63 |
| array_access | 203.01 | 21.56 |
| matrix | 198.21 | 33.19 |
| allocation | 416.05 | 30.87 |
| text_append | 154.42 | 26.89 |
| text_index | 133.89 | 28.51 |
| hash_map | 1056.62 | 49.26 |
