// Cliente da API REST da Máquina de Turing.
// O estado da simulação fica no servidor; aqui apenas guardamos o ID da sessão.
const API_BASE = "/api/simulations";

let simulationId = null;
let lastSnapshot = null;
let playIntervalId = null;
let isRunning = false;
let isRequesting = false;

// Elementos da DOM
const tapeTrack = document.getElementById("tape-track");
const headPointer = document.getElementById("head-pointer");
const currentStateEl = document.getElementById("current-state");
const stepCountEl = document.getElementById("step-count");
const machineStatusEl = document.getElementById("machine-status");
const lastTransitionEl = document.getElementById("last-transition");
const errorEl = document.getElementById("error-message");
const inputStringEl = document.getElementById("input-string");

const btnStep = document.getElementById("btn-step");
const btnPlay = document.getElementById("btn-play");
const btnPause = document.getElementById("btn-pause");
const btnReset = document.getElementById("btn-reset");
const btnLoad = document.getElementById("btn-load");

const STATUS_LABELS = {
    READY: ["Pronto", "status-pending"],
    RUNNING: ["Executando", "status-pending"],
    ACCEPTED: ["Aceito!", "status-accepted"],
    REJECTED: ["Rejeitado", "status-rejected"],
    HALTED: ["Interrompido", "status-rejected"]
};

const HALTED_STATUSES = ["ACCEPTED", "REJECTED", "HALTED"];

function isHalted(snapshot) {
    return !!snapshot && HALTED_STATUSES.includes(snapshot.status);
}

async function request(url, options = {}) {
    let response;
    try {
        response = await fetch(url, {
            headers: { "Content-Type": "application/json" },
            ...options
        });
    } catch (networkError) {
        throw new Error("Não foi possível conectar à API. Verifique se o servidor está em execução.");
    }

    const contentType = response.headers.get("content-type") || "";
    const body = contentType.includes("json") ? await response.json() : null;

    if (!response.ok) {
        const detail = body && (body.detail || body.title);
        throw new Error(detail || `Erro ${response.status} na requisição.`);
    }
    return body;
}

async function createSimulation() {
    pauseMachine();
    clearError();

    const input = inputStringEl.value.trim();
    if (!input) {
        showError("Informe uma cadeia de entrada.");
        return;
    }

    isRequesting = true;
    syncControls();
    try {
        const snapshot = await request(API_BASE, {
            method: "POST",
            body: JSON.stringify({ input })
        });
        simulationId = snapshot.id;
        lastSnapshot = snapshot;
        render(snapshot);
        applyStatus(snapshot);
    } catch (error) {
        simulationId = null;
        lastSnapshot = null;
        showError(error.message);
    } finally {
        isRequesting = false;
        syncControls();
    }
}

async function stepMachine() {
    if (!simulationId || isRequesting) return;
    if (isHalted(lastSnapshot)) {
        pauseMachine();
        syncControls();
        return;
    }

    isRequesting = true;
    syncControls();
    try {
        const snapshot = await request(`${API_BASE}/${simulationId}/step`, { method: "POST" });
        lastSnapshot = snapshot;
        render(snapshot);
        applyStatus(snapshot);
        if (isHalted(snapshot)) pauseMachine();
    } catch (error) {
        pauseMachine();
        showError(error.message);
    } finally {
        isRequesting = false;
        syncControls();
    }
}

function playMachine() {
    if (!simulationId || isRunning || isHalted(lastSnapshot)) return;
    clearError();
    isRunning = true;
    syncControls();
    playIntervalId = setInterval(() => stepMachine(), 300);
}

function pauseMachine() {
    isRunning = false;
    if (playIntervalId !== null) {
        clearInterval(playIntervalId);
        playIntervalId = null;
    }
    syncControls();
}

function syncControls() {
    const halted = isHalted(lastSnapshot);
    btnStep.disabled = isRequesting || !simulationId || isRunning || halted;
    btnPlay.disabled = isRequesting || !simulationId || isRunning || halted;
    btnPause.disabled = !isRunning;
    btnLoad.disabled = isRequesting;
    btnReset.disabled = isRequesting;
    inputStringEl.disabled = isRequesting;
}

function render(snapshot) {
    currentStateEl.textContent = snapshot.state;
    stepCountEl.textContent = snapshot.stepCount;

    tapeTrack.innerHTML = "";
    snapshot.tape.forEach((symbol, index) => {
        const cell = document.createElement("div");
        cell.className = `tape-cell ${index === snapshot.headPosition ? "active" : ""}`;
        cell.textContent = symbol === "_" ? " " : symbol;
        tapeTrack.appendChild(cell);
    });
    updateHeadPointer(snapshot.headPosition);

    if (snapshot.lastTransition) {
        const t = snapshot.lastTransition;
        lastTransitionEl.textContent =
            `${t.from}: ${symbolLabel(t.read)} → ${symbolLabel(t.write)}, ${t.direction} → ${t.to}`;
    } else {
        lastTransitionEl.textContent = "—";
    }
    if (isHalted(snapshot) && snapshot.haltReason) {
        lastTransitionEl.textContent += `  •  ${snapshot.haltReason}`;
    }
}

function updateHeadPointer(index) {
    const activeCell = tapeTrack.children[index];
    if (!activeCell) {
        headPointer.style.display = "none";
        return;
    }
    headPointer.style.display = "block";
    headPointer.style.left = `${activeCell.offsetLeft + activeCell.offsetWidth / 2}px`;
}

function applyStatus(snapshot) {
    const [label, className] = STATUS_LABELS[snapshot.status] || [snapshot.status, "status-pending"];
    machineStatusEl.textContent = label;
    machineStatusEl.className = className;
}

function symbolLabel(symbol) {
    return symbol === "_" ? "□" : symbol;
}

function showError(message) {
    errorEl.textContent = message;
    errorEl.hidden = false;
}

function clearError() {
    errorEl.textContent = "";
    errorEl.hidden = true;
}

btnLoad.addEventListener("click", createSimulation);
btnReset.addEventListener("click", createSimulation);
btnStep.addEventListener("click", stepMachine);
btnPlay.addEventListener("click", playMachine);
btnPause.addEventListener("click", pauseMachine);
inputStringEl.addEventListener("keydown", (event) => {
    if (event.key === "Enter") createSimulation();
});
window.addEventListener("resize", () => {
    if (lastSnapshot) updateHeadPointer(lastSnapshot.headPosition);
});

window.onload = createSimulation;
