package br.com.turingmachine.application;

import br.com.turingmachine.application.SimulationException.Kind;
import br.com.turingmachine.domain.MachineConfiguration;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import static br.com.turingmachine.domain.MachineEnums.MachineStatus.ACCEPTED;
import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

class SimulationServiceTest {

    private SimulationService service;

    @BeforeEach
    void setUp() {
        service = new SimulationService();
    }

    @Test
    void rejectsInvalidInputs() {
        assertThatThrownBy(() -> service.create(""))
                .isInstanceOf(SimulationException.class)
                .hasMessageContaining("não pode ser vazia");
        assertThatThrownBy(() -> service.create("abd"))
                .isInstanceOf(SimulationException.class)
                .hasMessageContaining("alfabeto");
        assertThatThrownBy(() -> service.create("a b c"))
                .isInstanceOf(SimulationException.class)
                .hasMessageContaining("alfabeto")
                .extracting(e -> ((SimulationException) e).kind())
                .isEqualTo(Kind.INVALID_INPUT);
    }

    @Test
    void trimsInputBeforeCreating() {
        assertThat(service.create("  aabbcc  ").input()).isEqualTo("aabbcc");
    }

    @Test
    void stepsUntilAcceptance() {
        SimulationService.SimulationSession session = service.create("aabbcc");

        MachineConfiguration result = session.current();
        while (!result.isHalted()) {
            result = session.step();
        }

        assertThat(result.status()).isEqualTo(ACCEPTED);
        assertThat(result.stepCount()).isPositive();
        assertThat(session.current().status()).isEqualTo(ACCEPTED);
    }

    @Test
    void unknownSessionThrowsNotFound() {
        assertThatThrownBy(() -> service.get("inexistente"))
                .isInstanceOf(SimulationException.class)
                .extracting(e -> ((SimulationException) e).kind())
                .isEqualTo(Kind.NOT_FOUND);
    }

    @Test
    void deleteRemovesSession() {
        String id = service.create("abc").id();

        service.delete(id);

        assertThatThrownBy(() -> service.get(id)).isInstanceOf(SimulationException.class);
    }

    @Test
    void cleanupDoesNotRemoveActiveSessions() {
        service.create("abc");

        assertThat(service.evictExpiredSessions()).isZero();
        assertThat(service.definition().transitions()).hasSize(16);
    }
}
