DIRECOES = {"<": "esquerda", ">": "direita", "-": "parado"}


class Transition:
    def __init__(self, current_state, read_symbol, next_state, write_symbol, direction):
        self.current_state = current_state
        self.read_symbol = read_symbol
        self.next_state = next_state
        self.write_symbol = write_symbol
        self.direction = direction


class StepSnapshot:
    def __init__(self, step, state, head, tape, log_text, prev_head=0, applied_rule=None):
        self.step = step
        self.state = state
        self.head = head
        self.prev_head = prev_head
        self.tape = tape
        self.log_text = log_text
        self.applied_rule = applied_rule


class SimulationResult:
    def __init__(self, status, steps, message):
        self.status = status
        self.steps = steps
        self.message = message


class TuringMachine:
    def __init__(self, init_state, accept_states, transitions, blank="_"):
        self.init_state = init_state
        self.accept_states = set(accept_states)
        self.transitions = transitions
        self.blank = blank

    def get_states(self):
        estados = {self.init_state} | set(self.accept_states)
        for tr in self.transitions.values():
            estados.add(tr.current_state)
            estados.add(tr.next_state)
        return sorted(estados)

    def get_tape_alphabet(self):
        simbolos = {self.blank}
        for tr in self.transitions.values():
            simbolos.add(tr.read_symbol)
            simbolos.add(tr.write_symbol)
        return sorted(simbolos)

    def run(self, input_str, max_steps=1000):
        fita = {i: ch for i, ch in enumerate(input_str)}
        pos = 0
        estado = self.init_state

        entrada_txt = input_str if input_str else "vazio"
        passos = [
            StepSnapshot(
                0,
                estado,
                pos,
                dict(fita),
                f"inicio: fita='{entrada_txt}', estado={estado}, pos=0",
                prev_head=0,
            )
        ]

        if estado in self.accept_states:
            return SimulationResult("ACCEPTED", passos, f"aceita no estado {estado} (0 passos)")

        for passo in range(1, max_steps + 1):
            simbolo = fita.get(pos, self.blank)
            regra = self.transitions.get((estado, simbolo))

            if regra is None:
                return SimulationResult(
                    "REJECTED",
                    passos,
                    f"rejeitada: sem transicao para ({estado}, '{simbolo}')",
                )

            pos_ant = pos
            fita[pos] = regra.write_symbol
            if regra.direction == ">":
                pos += 1
            elif regra.direction == "<":
                pos -= 1
            estado = regra.next_state

            log = (
                f"{regra.current_state} leu '{regra.read_symbol}' -> "
                f"escreveu '{regra.write_symbol}', moveu {DIRECOES[regra.direction]}, foi para {regra.next_state}"
            )
            passos.append(
                StepSnapshot(passo, estado, pos, dict(fita), log, prev_head=pos_ant, applied_rule=regra)
            )

            if estado in self.accept_states:
                return SimulationResult("ACCEPTED", passos, f"aceita no estado {estado} ({passo} passos)")

        return SimulationResult("LIMIT_REACHED", passos, f"parou no limite de {max_steps} passos")


def parse_file_text(content, default_init="q0", default_accept="qAccept", default_blank="_"):
    init_state = default_init
    accept_str = default_accept
    blank = default_blank
    regras = []

    for linha in content.splitlines():
        sem_com = linha.split("//", 1)[0].strip()
        if not sem_com:
            if linha.strip().startswith("//"):
                regras.append(linha.strip())
            continue

        low = sem_com.lower()
        if low.startswith("name:") or low.startswith("nome:"):
            continue
        if low.startswith("init:") or low.startswith("inicial:"):
            val = sem_com.split(":", 1)[1].strip()
            if val:
                init_state = val
            continue
        if low.startswith("accept:") or low.startswith("aceitar:") or low.startswith("aceitacao:"):
            val = sem_com.split(":", 1)[1].strip()
            if val:
                accept_str = val
            continue
        if low.startswith("blank:") or low.startswith("branco:"):
            val = sem_com.split(":", 1)[1].strip()
            if len(val) == 1:
                blank = val
            continue

        regras.append(linha.rstrip())

    return init_state, accept_str, blank, "\n".join(regras).strip()


def format_file_text(init_state, accept_str, blank, rules_text):
    cabecalho = [
        f"init: {init_state.strip()}",
        f"accept: {accept_str.strip()}",
        f"blank: {blank}",
        "",
    ]
    return "\n".join(cabecalho) + rules_text.strip() + "\n"


def parse_rules(rules_text, init_state, accept_str, blank="_"):
    init_clean = init_state.strip()
    if not init_clean:
        raise ValueError("informe o estado inicial")

    accept_set = {s.strip() for s in accept_str.split(",") if s.strip()}
    if not accept_set:
        raise ValueError("informe pelo menos um estado de aceitacao")

    if len(blank) != 1:
        raise ValueError("o simbolo branco deve ter exatamente 1 caractere")

    transitions = {}
    pendente = None

    for num, raw in enumerate(rules_text.splitlines(), start=1):
        linha = raw.split("//", 1)[0].strip()
        if not linha:
            continue

        # formato 1 linha: estado, le -> proximo, escreve, direcao
        if "->" in linha:
            if pendente is not None:
                raise ValueError(f"linha {pendente[0]}: regra incompleta antes da linha {num}")
            esq, dir_txt = linha.split("->", 1)
            p_esq = [p.strip() for p in esq.split(",")]
            p_dir = [p.strip() for p in dir_txt.split(",")]
            if len(p_esq) != 2 or len(p_dir) != 3:
                raise ValueError(f"linha {num}: use 'estado, le -> proximo, escreve, direcao'")
            est, le = p_esq
            prox, esc, dirc = p_dir
            _registrar_regra(transitions, num, est, le, prox, esc, dirc)
            continue

        # formato 2 linhas (compativel com turingmachinesimulator)
        partes = [p.strip() for p in linha.split(",")]
        if pendente is None:
            if len(partes) != 2:
                raise ValueError(f"linha {num}: formato invalido '{linha}'")
            pendente = (num, partes[0], partes[1])
        else:
            num_ant, est, le = pendente
            if len(partes) != 3:
                raise ValueError(f"linha {num}: esperado 'proximo, escreve, direcao'")
            prox, esc, dirc = partes
            _registrar_regra(transitions, num_ant, est, le, prox, esc, dirc)
            pendente = None

    if pendente is not None:
        raise ValueError(f"linha {pendente[0]}: regra incompleta")

    return TuringMachine(init_clean, accept_set, transitions, blank=blank)


def _registrar_regra(transitions, num, est, le, prox, esc, dirc):
    if not est or not prox:
        raise ValueError(f"linha {num}: nome de estado vazio")
    if len(le) != 1 or len(esc) != 1:
        raise ValueError(f"linha {num}: simbolos devem ter exatamente 1 caractere")
    if dirc not in DIRECOES:
        raise ValueError(f"linha {num}: direcao '{dirc}' invalida (use <, > ou -)")
    chave = (est, le)
    if chave in transitions:
        raise ValueError(f"linha {num}: regra duplicada para ({est}, '{le}')")
    transitions[chave] = Transition(est, le, prox, esc, dirc)


def transitions_to_rows(transitions):
    return [
        {
            "estado": tr.current_state,
            "le": tr.read_symbol,
            "proximo": tr.next_state,
            "escreve": tr.write_symbol,
            "direcao": tr.direction,
        }
        for tr in transitions.values()
    ]


def rows_to_rules_text(rows):
    linhas = []
    for row in rows:
        est = str(row.get("estado") or "").strip()
        le = str(row.get("le") or "").strip()
        prox = str(row.get("proximo") or "").strip()
        esc = str(row.get("escreve") or "").strip()
        dirc = str(row.get("direcao") or "").strip()
        if not any([est, le, prox, esc, dirc]):
            continue
        linhas.append(f"{est}, {le} -> {prox}, {esc}, {dirc}")
    return "\n".join(linhas)