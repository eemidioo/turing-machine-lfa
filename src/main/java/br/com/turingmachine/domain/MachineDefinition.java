package br.com.turingmachine.domain;

import br.com.turingmachine.domain.MachineEnums.Direction;

import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Optional;
import java.util.Set;

import static br.com.turingmachine.domain.MachineEnums.Direction.LEFT;
import static br.com.turingmachine.domain.MachineEnums.Direction.RIGHT;
import static br.com.turingmachine.domain.MachineEnums.Direction.STAY;

/**
 * Definição imutável de uma máquina de Turing, incluindo a tabela que reconhece
 * L = { a^n b^n c^n | n >= 1 } (transcrição da implementação JavaScript original).
 */
public final class MachineDefinition {

    /** Par (estado atual, símbolo lido). */
    public record TransitionKey(String state, char symbol) {
        public TransitionKey {
            if (state == null || state.isBlank()) {
                throw new IllegalArgumentException("O estado da transição não pode ser vazio");
            }
        }
    }

    /** Uma transição da máquina. */
    public record Transition(String fromState, char read, String toState, char write, Direction direction) {
        public Transition {
            if (fromState == null || fromState.isBlank()) {
                throw new IllegalArgumentException("Estado de origem inválido");
            }
            if (toState == null || toState.isBlank()) {
                throw new IllegalArgumentException("Estado de destino inválido");
            }
            if (direction == null) {
                throw new IllegalArgumentException("Direção inválida");
            }
        }

        public TransitionKey key() {
            return new TransitionKey(fromState, read);
        }
    }

    private final String initialState;
    private final Set<String> acceptingStates;
    private final Set<String> rejectingStates;
    private final Set<String> knownStates;
    private final char blankSymbol;
    private final Set<Character> inputAlphabet;
    private final Map<TransitionKey, Transition> transitions;

    public MachineDefinition(String initialState,
                             Set<String> acceptingStates,
                             Set<String> rejectingStates,
                             char blankSymbol,
                             Set<Character> inputAlphabet,
                             Map<TransitionKey, Transition> transitions) {
        if (initialState == null || initialState.isBlank()) {
            throw new IllegalArgumentException("Estado inicial inválido");
        }
        if (acceptingStates == null || rejectingStates == null || inputAlphabet == null || transitions == null) {
            throw new IllegalArgumentException("A definição da máquina não aceita coleções nulas");
        }
        if (acceptingStates.stream().anyMatch(rejectingStates::contains)) {
            throw new IllegalArgumentException("Um estado não pode ser aceitador e rejeitador ao mesmo tempo");
        }

        this.initialState = initialState;
        this.acceptingStates = Collections.unmodifiableSet(new LinkedHashSet<>(acceptingStates));
        this.rejectingStates = Collections.unmodifiableSet(new LinkedHashSet<>(rejectingStates));
        this.blankSymbol = blankSymbol;
        this.inputAlphabet = Collections.unmodifiableSet(new LinkedHashSet<>(inputAlphabet));
        this.transitions = Map.copyOf(transitions);

        Set<String> states = new LinkedHashSet<>();
        states.add(initialState);
        states.addAll(this.acceptingStates);
        states.addAll(this.rejectingStates);
        this.transitions.values().forEach(t -> {
            states.add(t.fromState());
            states.add(t.toState());
        });
        this.knownStates = Collections.unmodifiableSet(states);

        validateTransitions();
    }

    /** Cria a definição da linguagem L = { a^n b^n c^n | n >= 1 }. */
    public static MachineDefinition anBnCn() {
        Map<TransitionKey, Transition> t = new LinkedHashMap<>();
        add(t, "q0", 'a', "qFindB", 'X', RIGHT);
        add(t, "q0", 'Y', "qCheckDone", 'Y', RIGHT);
        add(t, "qFindB", 'a', "qFindB", 'a', RIGHT);
        add(t, "qFindB", 'Y', "qFindB", 'Y', RIGHT);
        add(t, "qFindB", 'b', "qFindC", 'Y', RIGHT);
        add(t, "qFindC", 'b', "qFindC", 'b', RIGHT);
        add(t, "qFindC", 'Z', "qFindC", 'Z', RIGHT);
        add(t, "qFindC", 'c', "qReturn", 'Z', LEFT);
        add(t, "qReturn", 'a', "qReturn", 'a', LEFT);
        add(t, "qReturn", 'b', "qReturn", 'b', LEFT);
        add(t, "qReturn", 'Y', "qReturn", 'Y', LEFT);
        add(t, "qReturn", 'Z', "qReturn", 'Z', LEFT);
        add(t, "qReturn", 'X', "q0", 'X', RIGHT);
        add(t, "qCheckDone", 'Y', "qCheckDone", 'Y', RIGHT);
        add(t, "qCheckDone", 'Z', "qCheckDone", 'Z', RIGHT);
        add(t, "qCheckDone", '_', "qAccept", '_', STAY);

        return new MachineDefinition(
                "q0",
                Set.of("qAccept"),
                Set.of("qReject"),
                '_',
                Set.of('a', 'b', 'c'),
                t
        );
    }

    private static void add(Map<TransitionKey, Transition> transitions,
                            String from, char read, String to, char write, Direction direction) {
        Transition transition = new Transition(from, read, to, write, direction);
        transitions.put(transition.key(), transition);
    }

    private void validateTransitions() {
        for (Map.Entry<TransitionKey, Transition> entry : transitions.entrySet()) {
            TransitionKey key = entry.getKey();
            Transition transition = entry.getValue();
            if (!key.equals(transition.key())) {
                throw new IllegalArgumentException("Transição inconsistente com sua chave: " + key);
            }
            if (!knownStates.contains(transition.toState())) {
                throw new IllegalArgumentException("Estado desconhecido: " + transition.toState());
            }
        }
    }

    public Optional<Transition> transitionFor(String state, char symbol) {
        return Optional.ofNullable(transitions.get(new TransitionKey(state, symbol)));
    }

    public boolean isAccepting(String state) {
        return acceptingStates.contains(state);
    }

    public boolean isRejecting(String state) {
        return rejectingStates.contains(state);
    }

    public boolean isInInputAlphabet(char symbol) {
        return inputAlphabet.contains(symbol);
    }

    public String initialState() {
        return initialState;
    }

    public char blankSymbol() {
        return blankSymbol;
    }

    public Set<Character> inputAlphabet() {
        return inputAlphabet;
    }

    public Map<TransitionKey, Transition> transitions() {
        return transitions;
    }
}
