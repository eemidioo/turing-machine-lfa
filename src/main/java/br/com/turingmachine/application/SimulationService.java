package br.com.turingmachine.application;

import br.com.turingmachine.domain.MachineConfiguration;
import br.com.turingmachine.domain.MachineDefinition;
import br.com.turingmachine.domain.TuringMachine;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Service;

import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;

/** Cria e administra as sessões de simulação, mantendo o estado no servidor. */
@Service
public class SimulationService {

    public static final long MAX_STEPS = 10_000;
    private static final Duration SESSION_TTL = Duration.ofMinutes(30);
    private static final int MAX_SESSIONS = 1000;

    private final Map<String, SimulationSession> sessions = new ConcurrentHashMap<>();
    private final MachineDefinition definition = MachineDefinition.anBnCn();

    @Scheduled(fixedDelayString = "60000", initialDelayString = "60000")
    public int evictExpiredSessions() {
        List<String> expired = sessions.values().stream()
                .filter(session -> session.isExpired(SESSION_TTL))
                .map(SimulationSession::id)
                .toList();
        expired.forEach(sessions::remove);
        return expired.size();
    }

    public SimulationSession create(String rawInput) {
        String input = rawInput == null ? "" : rawInput.trim();
        validateInput(input);

        evictExpiredSessions();
        if (sessions.size() >= MAX_SESSIONS) {
            evictOldest();
        }

        String id = UUID.randomUUID().toString();
        MachineConfiguration initial = MachineConfiguration.initial(definition, input);
        SimulationSession session = new SimulationSession(
                id, input, new TuringMachine(definition), initial);
        sessions.put(id, session);
        return session;
    }

    public SimulationSession get(String id) {
        SimulationSession session = sessions.get(id);
        if (session == null) {
            throw SimulationException.notFound(id);
        }
        return session;
    }

    public void delete(String id) {
        if (sessions.remove(id) == null) {
            throw SimulationException.notFound(id);
        }
    }

    public MachineDefinition definition() {
        return definition;
    }

    public long maxSteps() {
        return MAX_STEPS;
    }

    private void evictOldest() {
        List<SimulationSession> ordered = new ArrayList<>(sessions.values());
        ordered.sort(Comparator.comparing(SimulationSession::createdAt));
        int excess = ordered.size() - MAX_SESSIONS + 1;
        for (int i = 0; i < excess && i < ordered.size(); i++) {
            sessions.remove(ordered.get(i).id());
        }
    }

    private void validateInput(String input) {
        if (input.isEmpty()) {
            throw SimulationException.invalidInput("A cadeia de entrada não pode ser vazia");
        }
        for (char symbol : input.toCharArray()) {
            if (!definition.isInInputAlphabet(symbol)) {
                throw SimulationException.invalidInput(String.format(
                        "Símbolo '%s' não pertence ao alfabeto {a, b, c} da linguagem", symbol));
            }
        }
    }

    /** Estado de uma simulação; métodos sincronizados evitam corrida entre requisições. */
    public static final class SimulationSession {

        private final String id;
        private final String input;
        private final TuringMachine machine;
        private final Instant createdAt;

        private MachineConfiguration configuration;
        private Instant lastAccessedAt;

        private SimulationSession(String id, String input, TuringMachine machine,
                                  MachineConfiguration initialConfiguration) {
            this.id = id;
            this.input = input;
            this.machine = machine;
            this.configuration = initialConfiguration;
            this.createdAt = Instant.now();
            this.lastAccessedAt = this.createdAt;
        }

        public synchronized MachineConfiguration step() {
            configuration = machine.step(configuration);
            touch();
            return configuration;
        }

        public synchronized MachineConfiguration current() {
            touch();
            return configuration;
        }

        public synchronized boolean isExpired(Duration ttl) {
            return lastAccessedAt.plus(ttl).isBefore(Instant.now());
        }

        public String id() {
            return id;
        }

        public String input() {
            return input;
        }

        public Instant createdAt() {
            return createdAt;
        }

        private void touch() {
            lastAccessedAt = Instant.now();
        }
    }
}
