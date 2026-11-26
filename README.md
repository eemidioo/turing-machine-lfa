# TM Task — simulador de Máquina de Turing

Implementação em C da linguagem **TM Task**, usada para descrever máquinas de Turing. O programa lê um arquivo `.tmt`, tokeniza o código, monta a lista de instruções e imprime o resultado da análise.

> **Estado atual:** o lexer e o parser estão implementados. A execução das instruções (`TMExecuteFunction`) ainda é um *stub* — o programa imprime as instruções reconhecidas em vez de simular a fita.

## Requisitos

- Compilador C com suporte a C11 (GCC, Clang ou `cc`)
- `make`

## Como compilar e executar

```bash
make                 # gera build/tm
make run             # executa a demonstração embutida (sem arquivo)
make example         # analisa examples/divisivel_por_3.tmt
make clean           # remove build/
```

Para analisar outro arquivo:

```bash
./build/tm caminho/para/programa.tmt
```

A compilação usa `-std=c11 -Wall -Wextra`. Para usar outro compilador ou outras flags:

```bash
make CC=clang CFLAGS="-std=c11 -Wall -Wextra -Wpedantic"
```

## Estrutura

```
.
├── Makefile
├── README.md
├── src/
│   ├── tm_lexer.c   # lexer e ponto de entrada
│   └── tm_lexer.h   # tokens, tipos e tabela de palavras reservadas
│   └── tm_parser.c  # parser e ponto de saída
│   └── tm_parser.h  # ações, conversão de tokens
├── examples/
│   └── divisivel_por_3.tmt
└── docs/
    └── linguagem.md # gramática e referência da linguagem
```

## Exemplo mínimo

```tm
build incremento
    blank '_'
    state q0 initial
    state qf accept

    task q0, '1'
        write '1'
        move R
    endtask qf
endbuild

tape "1011" into incremento
```

A sintaxe completa está em [docs/linguagem.md](docs/linguagem.md). Na implementação atual, o parser ainda não trata `run` e `check` — as instruções aceitas são `build`/`endbuild`, `blank`, `state`, `task`/`endtask`, `write`, `move`, `tape` e `limit`.

## Histórico

O projeto começou como uma aplicação Java/Spring Boot com API REST e interface web. No PR #1 a implementação foi reescrita em C; esta branch apenas limpa os artefatos deixados pela migração e organiza o código, os exemplos e a documentação.
