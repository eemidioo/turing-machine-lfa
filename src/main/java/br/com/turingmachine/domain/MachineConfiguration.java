package br.com.turingmachine.domain;

import br.com.turingmachine.domain.MachineEnums.MachineStatus;

/** Snapshot imutável da máquina: estado, cabeçote, fita, passos e situação da execução. */
public record MachineConfiguration(
        String state,
        int headPosition,
        Tape tape,
        long stepCount,
        MachineStatus status,
        String haltReason,
        MachineDefinition.Transition lastTransition
) {
    public MachineConfiguration {
        if (state == null || state.isBlank()) {
            throw new IllegalArgumentException("Estado atual inválido");
        }
        if (tape == null || status == null) {
            throw new IllegalArgumentException("Fita e status são obrigatórios");
        }
        if (stepCount < 0) {
            throw new IllegalArgumentException("Contador de passos não pode ser negativo");
        }
    }

    public static MachineConfiguration initial(MachineDefinition definition, String input) {
        return new MachineConfiguration(
                definition.initialState(), 1, Tape.of(input, definition.blankSymbol()),
                0, MachineStatus.READY, null, null);
    }

    public boolean isHalted() {
        return status == MachineStatus.ACCEPTED
                || status == MachineStatus.REJECTED
                || status == MachineStatus.HALTED;
    }

    public MachineConfiguration withStatus(MachineStatus newStatus, String reason) {
        return new MachineConfiguration(state, headPosition, tape, stepCount, newStatus, reason, lastTransition);
    }
}
