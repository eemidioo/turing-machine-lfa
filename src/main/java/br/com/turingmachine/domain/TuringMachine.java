package br.com.turingmachine.domain;

import br.com.turingmachine.domain.MachineDefinition.Transition;
import br.com.turingmachine.domain.MachineEnums.MachineStatus;

import java.util.Objects;
import java.util.Optional;

/** Motor stateless: cada chamada recebe e devolve uma configuração imutável. */
public final class TuringMachine {

    private final MachineDefinition definition;

    public TuringMachine(MachineDefinition definition) {
        this.definition = Objects.requireNonNull(definition, "Definição da máquina obrigatória");
    }

    /** Executa exatamente uma transição. */
    public MachineConfiguration step(MachineConfiguration configuration) {
        Objects.requireNonNull(configuration, "Configuração obrigatória");
        if (configuration.isHalted()) {
            return configuration;
        }

        char read = configuration.tape().read(configuration.headPosition());
        Optional<Transition> transition = definition.transitionFor(configuration.state(), read);

        if (transition.isEmpty()) {
            String reason = String.format("Transição indefinida para (%s, %s)",
                    configuration.state(), describe(read));
            return configuration.withStatus(MachineStatus.REJECTED, reason);
        }

        Transition applied = transition.get();

        Tape.TapeAdjustment adjusted = configuration.tape().adjustForHead(configuration.headPosition());
        Tape written = adjusted.tape().write(adjusted.headPosition(), applied.write());

        int moved = switch (applied.direction()) {
            case LEFT -> adjusted.headPosition() - 1;
            case RIGHT -> adjusted.headPosition() + 1;
            case STAY -> adjusted.headPosition();
        };

        Tape.TapeAdjustment finalAdjustment = written.adjustForHead(moved);
        Tape.NormalizedTape normalized = finalAdjustment.tape().normalize(finalAdjustment.headPosition());

        MachineStatus nextStatus;
        String haltReason = null;
        if (definition.isAccepting(applied.toState())) {
            nextStatus = MachineStatus.ACCEPTED;
            haltReason = "Cadeia aceita: " + configuration.state() + " --(" + describe(read) + " / "
                    + describe(applied.write()) + ", " + applied.direction() + ")--> " + applied.toState();
        } else if (definition.isRejecting(applied.toState())) {
            nextStatus = MachineStatus.REJECTED;
            haltReason = "Cadeia rejeitada no estado " + applied.toState();
        } else {
            nextStatus = MachineStatus.RUNNING;
        }

        return new MachineConfiguration(
                applied.toState(), normalized.headPosition(), normalized.tape(),
                configuration.stepCount() + 1, nextStatus, haltReason, applied);
    }

    /** Executa até parar ou atingir o limite de passos. */
    public MachineConfiguration run(MachineConfiguration configuration, long maxSteps) {
        Objects.requireNonNull(configuration, "Configuração obrigatória");
        if (maxSteps <= 0) {
            throw new IllegalArgumentException("O limite de passos deve ser positivo");
        }

        MachineConfiguration current = configuration;
        while (!current.isHalted() && current.stepCount() < maxSteps) {
            current = step(current);
        }
        if (current.isHalted()) {
            return current;
        }
        return current.withStatus(MachineStatus.HALTED,
                "Limite de " + maxSteps + " passos atingido antes da parada");
    }

    private static String describe(char symbol) {
        return symbol == '_' ? "_ (branco)" : String.valueOf(symbol);
    }
}
