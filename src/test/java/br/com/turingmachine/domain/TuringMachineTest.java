package br.com.turingmachine.domain;

import br.com.turingmachine.domain.MachineDefinition.Transition;
import br.com.turingmachine.domain.MachineDefinition.TransitionKey;
import br.com.turingmachine.domain.MachineEnums.Direction;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.NullAndEmptySource;
import org.junit.jupiter.params.provider.ValueSource;

import java.util.Map;
import java.util.Set;

import static br.com.turingmachine.domain.MachineEnums.MachineStatus.ACCEPTED;
import static br.com.turingmachine.domain.MachineEnums.MachineStatus.HALTED;
import static br.com.turingmachine.domain.MachineEnums.MachineStatus.REJECTED;
import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

class TuringMachineTest {

    private final MachineDefinition definition = MachineDefinition.anBnCn();
    private final TuringMachine machine = new TuringMachine(definition);

    @ParameterizedTest
    @ValueSource(strings = {"abc", "aabbcc", "aaabbbccc", "aaaaabbbbbccccc"})
    void acceptsWordsInLanguage(String input) {
        MachineConfiguration result = machine.run(initial(input), 10_000);

        assertThat(result.status()).isEqualTo(ACCEPTED);
        assertThat(result.stepCount()).isPositive();
        assertThat(result.haltReason()).isNotBlank();
    }

    @ParameterizedTest
    @NullAndEmptySource
    @ValueSource(strings = {
            "abcc", "aabbc", "aabc", "abcabc", "aaabbbcccc", "ba", "aabbcca", "acb", "abbbcc"
    })
    void rejectsWordsOutsideLanguage(String input) {
        MachineConfiguration result = machine.run(initial(input), 10_000);

        assertThat(result.status()).isEqualTo(REJECTED);
        assertThat(result.haltReason()).contains("Transição indefinida");
    }

    @Test
    void stepDoesNotMutatePreviousConfiguration() {
        MachineConfiguration initial = initial("abc");

        MachineConfiguration next = machine.step(initial);

        assertThat(initial.stepCount()).isZero();
        assertThat(initial.state()).isEqualTo("q0");
        assertThat(initial.lastTransition()).isNull();
        assertThat(next.stepCount()).isEqualTo(1);
        assertThat(next.state()).isEqualTo("qFindB");
        assertThat(next.headPosition()).isEqualTo(2);
        assertThat(next.lastTransition().write()).isEqualTo('X');
    }

    @Test
    void headNeverBecomesNegativeDuringExecution() {
        MachineConfiguration configuration = initial("aaabbbccc");

        while (!configuration.isHalted()) {
            configuration = machine.step(configuration);
            assertThat(configuration.headPosition()).isGreaterThanOrEqualTo(0);
            assertThat(configuration.tape().size()).isPositive();
        }

        assertThat(configuration.status()).isEqualTo(ACCEPTED);
    }

    @Test
    void runStopsAtStepLimitForNonHaltingDefinition() {
        MachineDefinition loop = new MachineDefinition(
                "s0", Set.of("accept"), Set.of("reject"), '_', Set.of('a'),
                Map.of(new TransitionKey("s0", 'a'),
                        new Transition("s0", 'a', "s0", 'a', Direction.STAY)));

        MachineConfiguration result = new TuringMachine(loop).run(
                MachineConfiguration.initial(loop, "a"), 5);

        assertThat(result.status()).isEqualTo(HALTED);
        assertThat(result.stepCount()).isEqualTo(5);
        assertThat(result.haltReason()).contains("Limite de 5 passos");
    }

    @Test
    void initialTapeReproducesJavascriptLayout() {
        Tape tape = Tape.of("abc", definition.blankSymbol());

        assertThat(tape.snapshot()).containsExactly('_', 'a', 'b', 'c', '_', '_', '_');
        assertThat(tape.read(1)).isEqualTo('a');
        assertThat(tape.read(-1)).isEqualTo('_');
        assertThat(tape.read(99)).isEqualTo('_');
    }

    @Test
    void writeExpandsToTheRightWithoutMutatingOriginal() {
        Tape tape = Tape.of("a", '_');
        Tape written = tape.write(10, 'X');

        assertThat(written.read(10)).isEqualTo('X');
        assertThat(written.size()).isGreaterThan(tape.size());
        assertThat(tape.read(10)).isEqualTo('_');
    }

    @Test
    void adjustForHeadPrependsBlanksAndReindexes() {
        Tape.TapeAdjustment adjustment = Tape.of("abc", '_').adjustForHead(-2);

        assertThat(adjustment.headPosition()).isZero();
        assertThat(adjustment.tape().size()).isEqualTo(9);
        assertThat(adjustment.tape().read(2)).isEqualTo('_');
        assertThat(adjustment.tape().read(3)).isEqualTo('a');
    }

    @Test
    void normalizeRestoresOriginalLayoutAfterLeftExpansion() {
        Tape.TapeAdjustment adjustment = Tape.of("abc", '_').adjustForHead(-2);
        Tape.NormalizedTape normalized = adjustment.tape().normalize(3);

        assertThat(normalized.headPosition()).isEqualTo(1);
        assertThat(normalized.tape().snapshot()).containsExactly('_', 'a', 'b', 'c', '_', '_', '_');
    }

    @Test
    void writeRejectsNegativeIndex() {
        assertThatThrownBy(() -> Tape.of("abc", '_').write(-1, 'X'))
                .isInstanceOf(IllegalArgumentException.class);
    }

    private MachineConfiguration initial(String input) {
        return MachineConfiguration.initial(definition, input);
    }
}
