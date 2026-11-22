# Linguagem TM Task

## 1. Introdução

Esta é a estrutura de um programa TM Task:

```tm
/*
    ...
*/
```

## 2. Estrutura da máquina

```tm
/*
    ...
*/
```

## 3. Convenções léxicas

As palavras-chave são reservadas e não podem ser usadas como nomes de estados:

```text
build      endbuild   state     task      endtask
write      move       blank     tape      into
run        limit      initial   accept    reject
check      left       right     stop      halt
```

Os atalhos `L`, `R` e `S` equivalem a `left`, `right` e `stop`. Comentários começam com `#` e vão até o fim da linha.

## 4. Gramática

```ebnf
<programa>   ::= { <build> | <comando> }
<build>      ::= "build" <id> <NL> { <decl> | <task> } "endbuild" <NL>

<decl>       ::= "blank" <simbolo> <NL>
               | "state" <id> [ "initial" | "accept" | "reject" ] <NL>

<task>       ::= "task" <id> "," <simbolo> <NL>
                 [ "write" <simbolo> <NL> ]
                 [ "move" <direcao> <NL> ]
                 "endtask" ( <id> | "halt" ) <NL>

<comando>    ::= "tape" <fita> "into" <id> <NL>
               | "run" <id> [ <fita> ] [ "limit" <numero> ] <NL>
               | "check" <id> <NL>

<direcao>    ::= "L" | "R" | "S" | "left" | "right" | "stop"
<fita>       ::= '"' { <simbolo> } '"'
<id>         ::= <letra> { <letra> | <digito> | "_" }
<simbolo>    ::= <letra> | <digito> | "_"
<comentario> ::= "#" { <qualquer-exceto-NL> }
```

> **Nota:** na implementação atual, o parser ainda não trata `run` e `check`. A gramática acima descreve a forma completa planejada para a linguagem; as instruções aceitas hoje são `build`/`endbuild`, `blank`, `state`, `task`/`endtask`, `write`, `move`, `tape` e `limit`.

## 5. Exemplo

```tm
build incremento # Inicio do autômato

    blank '_'
    state q0 initial
    state q1
    state qf accept

    task q0, '0'
        move right
    endtask q0

    task q0, '1'
        move right
    endtask q0

    task q0, blank
        move left
    endtask q1

    task q1, '1'
        write '0'
        move left
    endtask q1

    task q1, '0'
        write '1'
        move stop
    endtask qf

    task q1, blank
        write '1'
        move stop
    endtask qf

endbuild

tape "1011" into incremento
```
