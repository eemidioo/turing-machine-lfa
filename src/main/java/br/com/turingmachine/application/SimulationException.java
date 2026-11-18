package br.com.turingmachine.application;

import org.springframework.http.HttpStatus;

/** Exceção única da aplicação, com o tipo de falha já associado à resposta HTTP. */
public class SimulationException extends RuntimeException {

    public enum Kind {
        INVALID_INPUT(HttpStatus.BAD_REQUEST, "Entrada inválida"),
        NOT_FOUND(HttpStatus.NOT_FOUND, "Simulação não encontrada");

        private final HttpStatus status;
        private final String title;

        Kind(HttpStatus status, String title) {
            this.status = status;
            this.title = title;
        }

        public HttpStatus status() {
            return status;
        }

        public String title() {
            return title;
        }
    }

    private final Kind kind;

    public SimulationException(Kind kind, String message) {
        super(message);
        this.kind = kind;
    }

    public static SimulationException invalidInput(String message) {
        return new SimulationException(Kind.INVALID_INPUT, message);
    }

    public static SimulationException notFound(String id) {
        return new SimulationException(Kind.NOT_FOUND, "Simulação não encontrada: " + id);
    }

    public Kind kind() {
        return kind;
    }

    public HttpStatus status() {
        return kind.status();
    }

    public String title() {
        return kind.title();
    }
}
