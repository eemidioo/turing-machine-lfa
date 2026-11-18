package br.com.turingmachine.domain;

import java.util.ArrayList;
import java.util.List;

/**
 * Fita imutável. Leituras fora dos limites devolvem o símbolo branco sem alterar a fita;
 * escritas e movimentos expandem a fita automaticamente para ambos os lados.
 */
public final class Tape {

    /** Quantidade mínima de células brancas mantidas à direita do conteúdo. */
    private static final int MIN_TRAILING_BLANKS = 3;

    private final List<Character> cells;
    private final char blank;

    private Tape(List<Character> cells, char blank) {
        this.cells = List.copyOf(cells);
        this.blank = blank;
    }

    /**
     * Fita inicial no mesmo formato da versão JavaScript: um branco à esquerda,
     * a entrada e três brancos à direita. O cabeçote inicia na posição 1.
     */
    public static Tape of(String input, char blank) {
        String safeInput = input == null ? "" : input;
        List<Character> initial = new ArrayList<>();
        initial.add(blank);
        for (char symbol : safeInput.toCharArray()) {
            initial.add(symbol);
        }
        for (int i = 0; i < MIN_TRAILING_BLANKS; i++) {
            initial.add(blank);
        }
        return new Tape(initial, blank);
    }

    public char read(int index) {
        if (index < 0 || index >= cells.size()) {
            return blank;
        }
        return cells.get(index);
    }

    /** Escreve em uma posição já válida, expandindo à direita se necessário. */
    public Tape write(int index, char symbol) {
        List<Character> next = new ArrayList<>(cells);
        while (index >= next.size()) {
            next.add(blank);
        }
        if (index < 0) {
            throw new IllegalArgumentException("Não é possível escrever em índice negativo: " + index);
        }
        next.set(index, symbol);
        return new Tape(next, blank);
    }

    /**
     * Garante que {@code head} aponta para uma célula existente, acrescentando brancos
     * à esquerda (e reindexando o cabeçote) ou à direita conforme necessário.
     */
    public TapeAdjustment adjustForHead(int head) {
        if (head >= 0 && head < cells.size()) {
            return new TapeAdjustment(this, head);
        }
        if (head < 0) {
            int padding = -head;
            List<Character> next = new ArrayList<>(padding + cells.size());
            for (int i = 0; i < padding; i++) {
                next.add(blank);
            }
            next.addAll(cells);
            return new TapeAdjustment(new Tape(next, blank), 0);
        }
        List<Character> next = new ArrayList<>(cells);
        while (next.size() <= head) {
            next.add(blank);
        }
        return new TapeAdjustment(new Tape(next, blank), head);
    }

    /**
     * Remove brancos supérfluos nas extremidades, preservando um branco à esquerda e
     * pelo menos três à direita. O cabeçote é reindexado de acordo.
     */
    public NormalizedTape normalize(int head) {
        int firstContent = 0;
        while (firstContent < cells.size() && cells.get(firstContent) == blank) {
            firstContent++;
        }
        int lastContent = cells.size() - 1;
        while (lastContent >= 0 && cells.get(lastContent) == blank) {
            lastContent--;
        }

        if (firstContent > lastContent) {
            List<Character> onlyBlank = new ArrayList<>();
            for (int i = 0; i <= head + MIN_TRAILING_BLANKS; i++) {
                onlyBlank.add(blank);
            }
            int safeHead = Math.min(Math.max(head, 0), onlyBlank.size() - 1);
            return new NormalizedTape(new Tape(onlyBlank, blank), safeHead);
        }

        int start = Math.min(Math.max(firstContent - 1, 0), Math.max(head, 0));
        int end = Math.max(lastContent + MIN_TRAILING_BLANKS, head);
        end = Math.min(Math.max(end, start), cells.size() - 1);

        List<Character> next = new ArrayList<>(cells.subList(start, end + 1));
        return new NormalizedTape(new Tape(next, blank), head - start);
    }

    public List<Character> snapshot() {
        return cells;
    }

    public int size() {
        return cells.size();
    }

    public char blankSymbol() {
        return blank;
    }

    /** Resultado de um ajuste de cabeçote: fita eventualmente expandida e nova posição. */
    public record TapeAdjustment(Tape tape, int headPosition) {
    }

    /** Fita normalizada e posição do cabeçote após a normalização. */
    public record NormalizedTape(Tape tape, int headPosition) {
    }
}
