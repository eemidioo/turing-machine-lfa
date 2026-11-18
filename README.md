# Simulador de Máquina de Turing — L = { aⁿ bⁿ cⁿ | n ≥ 1 }

Aplicação Java/Spring Boot com uma interface web para simular, passo a passo, uma máquina de Turing que reconhece cadeias com quantidades iguais de `a`, `b` e `c`.

## Stack

- Java 17 (executado nos testes com JDK 21)
- Spring Boot 3.5.16 — `spring-boot-starter-web`, `spring-boot-starter-validation`
- Maven Wrapper 3.3.4 / Maven 3.9.16 (não exige Maven instalado)
- JUnit 5 + AssertJ + MockMvc

## Como executar

```bash
./mvnw spring-boot:run
```

Acesse **http://localhost:8080**. Os arquivos da interface estão em `src/main/resources/static/` e são servidos pelo próprio Spring Boot.

Para gerar o JAR:

```bash
./mvnw clean package
java -jar target/turing-machine-lfa-1.0.0.jar
```

Para rodar os testes:

```bash
./mvnw test
```

## Estrutura

```
src/main/java/br/com/turingmachine/
├── TuringMachineApplication.java   # bootstrap Spring Boot
├── domain/                         # motor puro e imutável
│   ├── MachineEnums.java           # Direction + MachineStatus
│   ├── MachineDefinition.java      # tabela de transições + fábrica anBnCn()
│   ├── Tape.java                   # fita com expansão nos dois lados
│   ├── MachineConfiguration.java   # snapshot da execução
│   └── TuringMachine.java          # step() e run()
├── application/
│   ├── SimulationService.java      # serviço + sessões + limpeza periódica
│   └── SimulationException.java    # INVALID_INPUT / NOT_FOUND
└── api/
    ├── SimulationController.java   # endpoints REST
    ├── ApiDtos.java                # contratos JSON
    └── GlobalExceptionHandler.java # ProblemDetail (RFC 7807)
```

O motor é `stateless`: `step()` recebe uma configuração e devolve outra, sem mutar a anterior. O estado da simulação fica no servidor, associado a um ID; o navegador guarda apenas esse ID.

## API REST

| Método | Rota | Descrição |
|---|---|---|
| `POST` | `/api/simulations` | Cria simulação a partir de `{ "input": "aabbcc" }` → `201` + `Location` |
| `GET` | `/api/simulations/{id}` | Snapshot atual |
| `POST` | `/api/simulations/{id}/step` | Executa uma transição |
| `DELETE` | `/api/simulations/{id}` | Encerra a sessão → `204` |

Erros usam `application/problem+json`: `400` para entrada inválida, `404` para ID desconhecido.

### Exemplos

```bash
curl -i -X POST http://localhost:8080/api/simulations \
  -H 'Content-Type: application/json' \
  -d '{"input":"aabbcc"}'

curl -X POST http://localhost:8080/api/simulations/{id}/step

curl -X POST http://localhost:8080/api/simulations \
  -H 'Content-Type: application/json' \
  -d '{"input":"abcc"}'      # alfabeto válido, mas rejeitada pela máquina

curl -i -X POST http://localhost:8080/api/simulations \
  -H 'Content-Type: application/json' \
  -d '{"input":"abd"}'       # símbolo fora do alfabeto → 400
```

Exemplo de snapshot:

```json
{
  "id": "7f3c1a...",
  "input": "aabbcc",
  "status": "RUNNING",
  "state": "qFindB",
  "headPosition": 3,
  "stepCount": 2,
  "tape": ["_", "X", "a", "Y", "b", "c", "c", "_", "_", "_"],
  "lastTransition": {
    "from": "q0", "read": "a", "to": "qFindB", "write": "X", "direction": "RIGHT"
  },
  "haltReason": null,
  "accepted": false
}
```

## Testes

- Aceitos: `abc`, `aabbcc`, `aaabbbccc`, `aaaaabbbbbccccc`
- Rejeitados: vazia, `abcc`, `aabbc`, `aabc`, `abcabc`, `aaabbbcccc`, `ba`, `aabbcca`, `acb`, `abbbcc`
- Fita inicial idêntica à versão original, expansão da fita, pureza do `step()`, limite de passos, fluxo HTTP completo e formato dos erros.
