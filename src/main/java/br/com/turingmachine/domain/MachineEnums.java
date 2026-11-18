package br.com.turingmachine.domain;

/** Tipos enumerados usados pelo motor da máquina. */
public final class MachineEnums {

    private MachineEnums() {
    }

    /** Direção de movimento do cabeçote; STAY substitui o "H" da versão JavaScript. */
    public enum Direction {
        LEFT, RIGHT, STAY
    }

    /** Situação da execução. */
    public enum MachineStatus {
        READY, RUNNING, ACCEPTED, REJECTED, HALTED
    }
}
