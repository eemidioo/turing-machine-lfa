package br.com.turingmachine.api;

import br.com.turingmachine.domain.MachineConfiguration;
import br.com.turingmachine.domain.MachineDefinition;
import br.com.turingmachine.domain.MachineEnums.MachineStatus;
import jakarta.validation.constraints.NotBlank;

import java.util.List;

/** Contratos JSON da API e conversões a partir do domínio. */
public final class ApiDtos {

    private ApiDtos() {
    }

    public record CreateSimulationRequest(
            @NotBlank(message = "Informe a cadeia de entrada") String input
    ) {
    }

    public record TransitionView(String from, String read, String to, String write, String direction) {
        public static TransitionView from(MachineDefinition.Transition transition) {
            return new TransitionView(
                    transition.fromState(),
                    String.valueOf(transition.read()),
                    transition.toState(),
                    String.valueOf(transition.write()),
                    transition.direction().name());
        }
    }

    public record SimulationResponse(
            String id,
            String input,
            String status,
            String state,
            int headPosition,
            long stepCount,
            List<String> tape,
            TransitionView lastTransition,
            String haltReason,
            boolean accepted
    ) {
        public static SimulationResponse from(String id, String input, MachineConfiguration configuration) {
            TransitionView last = configuration.lastTransition() == null
                    ? null
                    : TransitionView.from(configuration.lastTransition());

            return new SimulationResponse(
                    id,
                    input,
                    configuration.status().name(),
                    configuration.state(),
                    configuration.headPosition(),
                    configuration.stepCount(),
                    configuration.tape().snapshot().stream().map(String::valueOf).toList(),
                    last,
                    configuration.haltReason(),
                    configuration.status() == MachineStatus.ACCEPTED);
        }
    }
}
