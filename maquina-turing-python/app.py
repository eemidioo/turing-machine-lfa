import json
import streamlit as st
import streamlit.components.v1 as components

from turing_engine import (
    SimulationResult,
    TuringMachine,
    format_file_text,
    parse_file_text,
    parse_rules,
    rows_to_rules_text,
    transitions_to_rows,
)

st.set_page_config(
    page_title="Máquina de Turing",
    layout="wide",
)

st.markdown(
    """
    <style>
        [data-testid="stDeployButton"] {
            display: none;
        }
        .block-container {
            padding-top: 2.6rem;
            padding-bottom: 2.5rem;
            max-width: 1220px;
        }
        textarea {
            font-family: Consolas, "Courier New", monospace !important;
            font-size: 13px !important;
            line-height: 1.45 !important;
        }
        .stButton > button {
            border-radius: 3px !important;
        }
    </style>
    """,
    unsafe_allow_html=True,
)


def init_state():
    if "initialized" not in st.session_state:
        st.session_state.initialized = True
        st.session_state.init_state = "q0"
        st.session_state.accept_str = "qAccept"
        st.session_state.blank_sym = "_"
        st.session_state.rules_text = ""
        st.session_state.tape_input = ""
        st.session_state.max_steps = 1000
        st.session_state.machine = None
        st.session_state.result = None
        st.session_state.parse_error = None


def execute_machine():
    raw_rules = st.session_state.rules_text.strip()
    if not raw_rules:
        st.session_state.machine = None
        st.session_state.result = None
        st.session_state.parse_error = None
        return

    ini, acc, blk, clean_rules = parse_file_text(
        raw_rules,
        default_init=st.session_state.init_state,
        default_accept=st.session_state.accept_str,
        default_blank=st.session_state.blank_sym,
    )
    st.session_state.init_state = ini
    st.session_state.accept_str = acc
    st.session_state.blank_sym = blk
    st.session_state.rules_text = clean_rules

    if not clean_rules.strip():
        st.session_state.machine = None
        st.session_state.result = None
        st.session_state.parse_error = "nenhuma regra de transicao encontrada"
        return

    try:
        tm = parse_rules(clean_rules, ini, acc, blk)
        res = tm.run(st.session_state.tape_input, max_steps=int(st.session_state.max_steps))
        st.session_state.machine = tm
        st.session_state.result = res
        st.session_state.parse_error = None
    except ValueError as err:
        st.session_state.machine = None
        st.session_state.result = None
        st.session_state.parse_error = str(err)


def on_file_upload():
    uploaded = st.session_state.get("tm_file_uploader")
    if uploaded is None:
        return
    text = uploaded.getvalue().decode("utf-8", errors="replace")
    ini, acc, blk, r_txt = parse_file_text(
        text,
        default_init=st.session_state.init_state,
        default_accept=st.session_state.accept_str,
        default_blank=st.session_state.blank_sym,
    )
    st.session_state.init_state = ini
    st.session_state.accept_str = acc
    st.session_state.blank_sym = blk
    st.session_state.rules_text = r_txt
    execute_machine()


def build_machine_view(machine: TuringMachine, result: SimulationResult) -> str:
    all_indices = [idx for s in result.steps for idx in list(s.tape.keys()) + [s.head]]
    min_idx = min(all_indices) - 8
    max_idx = max(all_indices) + 8
    trace_min = min(all_indices) - 1
    trace_max = max(all_indices) + 1

    adj = {}
    for tr in machine.transitions.values():
        adj.setdefault(tr.current_state, []).append(tr.next_state)

    ordered_states = []
    fila = [machine.init_state]
    vistos = {machine.init_state}
    while fila:
        atual = fila.pop(0)
        ordered_states.append(atual)
        for nxt in adj.get(atual, []):
            if nxt not in vistos:
                vistos.add(nxt)
                fila.append(nxt)
    for st_name in machine.get_states():
        if st_name not in vistos:
            ordered_states.append(st_name)

    transitions_list = [
        {
            "from": tr.current_state,
            "to": tr.next_state,
            "read": tr.read_symbol,
            "write": tr.write_symbol,
            "dir": tr.direction,
        }
        for tr in machine.transitions.values()
    ]

    steps_payload = []
    for s in result.steps:
        rule_obj = None
        rule_text = "inicio"
        if s.applied_rule is not None:
            r = s.applied_rule
            rule_obj = {
                "from": r.current_state,
                "to": r.next_state,
                "read": r.read_symbol,
                "write": r.write_symbol,
                "dir": r.direction,
            }
            rule_text = f"{r.current_state}, {r.read_symbol} -> {r.next_state}, {r.write_symbol}, {r.direction}"
        steps_payload.append(
            {
                "step": s.step,
                "state": s.state,
                "head": s.head,
                "prevHead": s.prev_head,
                "tape": {str(k): v for k, v in s.tape.items()},
                "log": s.log_text,
                "rule": rule_text,
                "ruleObj": rule_obj,
            }
        )

    payload = {
        "blank": machine.blank,
        "initState": machine.init_state,
        "acceptStates": sorted(machine.accept_states),
        "states": ordered_states,
        "transitions": transitions_list,
        "status": result.status,
        "message": result.message,
        "minIdx": min_idx,
        "maxIdx": max_idx,
        "traceMin": trace_min,
        "traceMax": trace_max,
        "steps": steps_payload,
    }
    safe_json = json.dumps(payload, ensure_ascii=False).replace("</", "<\\/")

    return f"""<!DOCTYPE html>
<html lang="pt-BR" translate="no" class="notranslate">
<head>
<meta charset="utf-8" />
<meta name="google" content="notranslate" />
<script src="https://cdn.jsdelivr.net/npm/dagre@0.8.5/dist/dagre.min.js"></script>
<style>
  * {{ box-sizing: border-box; }}
  body {{
    margin: 0;
    padding: 2px;
    font-family: Consolas, "Courier New", monospace;
    color: #111111;
    background: #ffffff;
  }}
  .status-bar {{
    display: flex;
    flex-wrap: wrap;
    gap: 16px;
    align-items: center;
    padding: 9px 12px;
    background: #f5f5f5;
    border: 1px solid #cccccc;
    font-size: 13px;
    margin-bottom: 10px;
  }}
  .st-run {{ color: #333333; }}
  .st-acc {{ color: #116329; }}
  .st-rej {{ color: #a40e26; }}
  .st-lim {{ color: #9a6700; }}
  .phase-tag {{
    padding: 1px 6px;
    border: 1px solid #888888;
    background: #ffffff;
    font-size: 11px;
    color: #111111;
  }}

  .tape-stage {{
    position: relative;
    height: 108px;
    border: 1px solid #222222;
    background: #ffffff;
    overflow: hidden;
    margin-bottom: 10px;
    user-select: none;
    outline: none;
  }}
  .tape-stage:focus {{
    border-color: #000000;
    box-shadow: inset 0 0 0 1px #000000;
  }}
  .head-marker {{
    position: absolute;
    left: 50%;
    top: 6px;
    transform: translateX(-50%);
    width: 44px;
    display: flex;
    flex-direction: column;
    align-items: center;
    pointer-events: none;
    z-index: 3;
  }}
  .head-arrow {{
    width: 0;
    height: 0;
    border-left: 7px solid transparent;
    border-right: 7px solid transparent;
    border-top: 9px solid #111111;
    margin-bottom: 4px;
  }}
  .head-box {{
    width: 44px;
    height: 44px;
    border: 3px solid #111111;
  }}
  .tape-track {{
    position: absolute;
    left: 50%;
    top: 19px;
    display: flex;
    will-change: transform;
  }}
  .cell-wrap {{
    flex: 0 0 44px;
    width: 44px;
    display: flex;
    flex-direction: column;
    align-items: center;
  }}
  .cell {{
    width: 44px;
    height: 44px;
    border-top: 1px solid #555555;
    border-bottom: 1px solid #555555;
    border-right: 1px solid #555555;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 20px;
    font-weight: 700;
    background: #ffffff;
    color: #111111;
    transition: background-color 110ms ease, color 110ms ease;
  }}
  .cell-wrap:first-child .cell {{
    border-left: 1px solid #555555;
  }}
  .cell.active {{
    background: #111111;
    color: #ffffff;
  }}
  .cell.reading {{
    background: #333333;
    color: #ffffff;
  }}
  .cell.writing {{
    background: #d6d6d6;
    color: #000000;
  }}
  .cell-idx {{
    font-size: 10px;
    color: #777777;
    margin-top: 5px;
  }}

  .controls {{
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
    align-items: center;
    margin-bottom: 8px;
  }}
  .controls button, .controls select {{
    font-family: Consolas, "Courier New", monospace;
    font-size: 13px;
    padding: 5px 11px;
    border: 1px solid #888888;
    background: #ffffff;
    color: #111111;
    border-radius: 3px;
    cursor: pointer;
  }}
  .controls button:hover:not(:disabled) {{
    background: #f0f0f0;
    border-color: #222222;
  }}
  .controls button:disabled {{
    opacity: 0.4;
    cursor: default;
  }}
  .controls button.primary {{
    background: #111111;
    color: #ffffff;
    border-color: #111111;
  }}
  .controls button.primary:hover:not(:disabled) {{
    background: #333333;
  }}
  .hint {{
    font-size: 11px;
    color: #666666;
    margin-left: auto;
  }}

  .scrubber-row {{
    display: flex;
    align-items: center;
    gap: 10px;
    margin-bottom: 12px;
    font-size: 12px;
    color: #444444;
  }}
  .scrubber-row input[type="range"] {{
    flex: 1;
    accent-color: #111111;
    cursor: pointer;
  }}

  .split {{
    display: grid;
    grid-template-columns: 1.48fr 1fr;
    gap: 12px;
  }}
  @media (max-width: 860px) {{
    .split {{ grid-template-columns: 1fr; }}
  }}
  .box {{
    border: 1px solid #cccccc;
    background: #ffffff;
    display: flex;
    flex-direction: column;
  }}
  .box-header {{
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
    padding: 5px 9px;
    background: #f5f5f5;
    border-bottom: 1px solid #cccccc;
    font-size: 12px;
    font-weight: 700;
  }}
  .graph-tools {{
    display: flex;
    flex-wrap: wrap;
    gap: 4px;
    align-items: center;
  }}
  .gbtn {{
    font-family: Consolas, "Courier New", monospace;
    font-size: 11px;
    padding: 2px 7px;
    border: 1px solid #b5b5b5;
    background: #ffffff;
    color: #111111;
    border-radius: 2px;
    cursor: pointer;
  }}
  .gbtn:hover {{
    background: #eaeaea;
    border-color: #444444;
  }}
  .graph-wrap {{
    height: 345px;
    width: 100%;
    overflow: hidden;
    background: #ffffff;
    position: relative;
  }}
  svg {{
    width: 100%;
    height: 100%;
    display: block;
    user-select: none;
    cursor: grab;
  }}
  svg.panning {{
    cursor: grabbing;
  }}
  .graph-footer {{
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 4px 10px;
    border-top: 1px solid #e6e6e6;
    background: #fafafa;
    font-size: 11px;
    color: #555555;
  }}

  .mini-tabs {{
    display: flex;
    gap: 4px;
  }}
  .mini-tab {{
    font-family: Consolas, "Courier New", monospace;
    font-size: 12px;
    padding: 2px 8px;
    border: 1px solid transparent;
    background: transparent;
    color: #555555;
    cursor: pointer;
  }}
  .mini-tab.active {{
    background: #ffffff;
    color: #111111;
    font-weight: 700;
    border-color: #bbbbbb;
  }}
  .panel {{
    height: 369px;
    overflow: auto;
    padding: 8px 10px;
    background: #fafafa;
    font-size: 12px;
    line-height: 1.48;
  }}
  .hidden {{ display: none; }}

  .log-line {{ padding: 2px 4px; color: #444444; }}
  .log-line.curr {{ color: #000000; font-weight: 700; background: #ebebeb; }}
  .log-end {{
    margin-top: 8px;
    padding-top: 6px;
    border-top: 1px solid #cccccc;
    font-weight: 700;
    color: #111111;
  }}

  .trace-row {{
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 3px 4px;
    border-bottom: 1px solid #e6e6e6;
    background: #ffffff;
  }}
  .trace-row.curr {{ background: #ededed; }}
  .t-step {{ min-width: 30px; color: #666666; }}
  .t-state {{ min-width: 68px; font-weight: 700; }}
  .t-cells {{ display: inline-flex; }}
  .t-cell {{
    width: 20px;
    height: 20px;
    border: 1px solid #555555;
    margin-left: -1px;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    background: #ffffff;
    color: #111111;
    font-size: 11px;
  }}
  .t-cell.head {{
    background: #111111;
    color: #ffffff;
    font-weight: 700;
  }}
</style>
</head>
<body>
  <div class="status-bar" id="statusBar"></div>

  <div class="tape-stage" id="tapeStage" tabindex="0">
    <div class="head-marker">
      <div class="head-arrow"></div>
      <div class="head-box"></div>
    </div>
    <div class="tape-track" id="tapeTrack"></div>
  </div>

  <div class="controls">
    <button id="btnStart">Início</button>
    <button id="btnPrev">&lt; Passo</button>
    <button id="btnPlay" class="primary">Animar</button>
    <button id="btnNext">Passo &gt;</button>
    <button id="btnEnd">Final</button>
    <select id="speedSel" title="Velocidade da animação">
      <option value="950">Lento</option>
      <option value="520" selected>Normal</option>
      <option value="200">Rápido</option>
    </select>
    <span class="hint">teclado (clique na fita): &lt;- / -&gt; / espaco</span>
  </div>

  <div class="scrubber-row">
    <span>Progresso:</span>
    <input type="range" id="stepRange" min="0" value="0" />
    <span id="rangeLabel">0 / 0</span>
  </div>

  <div class="split">
    <div class="box">
      <div class="box-header">
        <span>Diagrama de estados</span>
        <div class="graph-tools">
          <button class="gbtn" id="gZoomIn" title="Aproximar">+</button>
          <button class="gbtn" id="gZoomOut" title="Afastar">-</button>
          <button class="gbtn" id="gFit" title="Ajustar à tela">Ajustar</button>
          <button class="gbtn" id="gRelayout" title="Reorganizar layout">Reorganizar</button>
          <button class="gbtn" id="gExport" title="Baixar diagrama vetorial (.svg)">SVG</button>
        </div>
      </div>
      <div class="graph-wrap" id="graphWrap">
        <svg id="graphSvg"></svg>
      </div>
      <div class="graph-footer">
        <span>notacao: le &rarr; escreve, L/R/S (&square; = branco)</span>
        <span>arraste estados ou fundo | scroll = zoom</span>
      </div>
    </div>

    <div class="box">
      <div class="box-header">
        <span>Execução</span>
        <div class="mini-tabs">
          <button class="mini-tab active" id="tabLogBtn">Log</button>
          <button class="mini-tab" id="tabTraceBtn">Rastro da fita</button>
        </div>
      </div>
      <div class="panel" id="logPanel"></div>
      <div class="panel hidden" id="tracePanel"></div>
    </div>
  </div>

<script>
  const DATA = {safe_json};
  const CELL_W = 44;
  const totalSteps = DATA.steps.length - 1;

  let currIdx = 0;
  let playing = false;
  let timers = [];

  const statusBar = document.getElementById("statusBar");
  const tapeTrack = document.getElementById("tapeTrack");
  const logPanel = document.getElementById("logPanel");
  const tracePanel = document.getElementById("tracePanel");
  const graphWrap = document.getElementById("graphWrap");
  const graphSvg = document.getElementById("graphSvg");
  const stepRange = document.getElementById("stepRange");
  const rangeLabel = document.getElementById("rangeLabel");
  const btnStart = document.getElementById("btnStart");
  const btnPrev = document.getElementById("btnPrev");
  const btnPlay = document.getElementById("btnPlay");
  const btnNext = document.getElementById("btnNext");
  const btnEnd = document.getElementById("btnEnd");
  const speedSel = document.getElementById("speedSel");
  const tabLogBtn = document.getElementById("tabLogBtn");
  const tabTraceBtn = document.getElementById("tabTraceBtn");
  const gZoomIn = document.getElementById("gZoomIn");
  const gZoomOut = document.getElementById("gZoomOut");
  const gFit = document.getElementById("gFit");
  const gRelayout = document.getElementById("gRelayout");
  const gExport = document.getElementById("gExport");

  stepRange.max = String(totalSteps);

  function esc(str) {{
    return String(str)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;");
  }}

  function fmtSym(s) {{
    return s === DATA.blank ? "\\u25A1" : s;
  }}

  function fmtDir(d) {{
    if (d === "<") return "L";
    if (d === ">") return "R";
    return "S";
  }}

  function clearTimers() {{
    while (timers.length) clearTimeout(timers.pop());
  }}

  function stopPlay() {{
    playing = false;
    clearTimers();
    btnPlay.textContent = "Animar";
  }}

  // ---------- FITA ----------
  const cellNodes = {{}};
  for (let i = DATA.minIdx; i <= DATA.maxIdx; i++) {{
    const wrap = document.createElement("div");
    wrap.className = "cell-wrap";
    const cell = document.createElement("div");
    cell.className = "cell";
    cell.textContent = DATA.blank;
    const label = document.createElement("div");
    label.className = "cell-idx";
    label.textContent = String(i);
    wrap.appendChild(cell);
    wrap.appendChild(label);
    tapeTrack.appendChild(wrap);
    cellNodes[i] = cell;
  }}

  // ---------- DIAGRAMA DE ESTADOS ----------
  const nodePos = {{}};
  const nodeDim = {{}};
  const acceptSet = new Set(DATA.acceptStates);

  DATA.states.forEach((st) => {{
    const isShort = st.length <= 3;
    const w = isShort ? 46 : Math.max(58, Math.round(st.length * 8.4 + 22));
    const h = isShort ? 46 : 38;
    const rx = Math.round(h / 2);
    nodeDim[st] = {{ w: w, h: h, rx: rx }};
  }});

  const edgeMap = {{}};
  DATA.transitions.forEach((tr) => {{
    const key = tr.from + "|||" + tr.to;
    if (!edgeMap[key]) {{
      edgeMap[key] = {{ from: tr.from, to: tr.to, rules: [] }};
    }}
    edgeMap[key].rules.push(tr);
  }});
  const edges = Object.values(edgeMap);

  function computeFallbackLayout() {{
    const levels = {{}};
    const queue = [DATA.initState];
    levels[DATA.initState] = 0;

    while (queue.length > 0) {{
      const cur = queue.shift();
      edges.forEach((e) => {{
        if (e.from === cur && e.to !== cur && levels[e.to] === undefined) {{
          levels[e.to] = levels[cur] + 1;
          queue.push(e.to);
        }}
      }});
    }}

    let maxLv = 0;
    Object.values(levels).forEach((lv) => {{ if (lv > maxLv) maxLv = lv; }});
    DATA.states.forEach((st) => {{
      if (levels[st] === undefined) {{
        maxLv += 1;
        levels[st] = maxLv;
      }}
    }});

    const cols = {{}};
    DATA.states.forEach((st) => {{
      const lv = levels[st];
      if (!cols[lv]) cols[lv] = [];
      cols[lv].push(st);
    }});

    Object.keys(cols).forEach((lvStr) => {{
      const lv = Number(lvStr);
      const list = cols[lv];
      const spacingY = 115;
      const totalH = (list.length - 1) * spacingY;
      list.forEach((st, idx) => {{
        nodePos[st] = {{
          x: 90 + lv * 165,
          y: 190 - totalH / 2 + idx * spacingY,
        }};
      }});
    }});
  }}

  function computeLayout() {{
    if (typeof window.dagre !== "undefined" && window.dagre.graphlib) {{
      try {{
        const g = new window.dagre.graphlib.Graph();
        g.setGraph({{
          rankdir: "LR",
          ranker: "network-simplex",
          nodesep: 86,
          ranksep: 120,
          marginx: 55,
          marginy: 60,
        }});
        g.setDefaultEdgeLabel(function() {{ return {{}}; }});

        DATA.states.forEach((st) => {{
          const d = nodeDim[st];
          g.setNode(st, {{ width: d.w, height: d.h }});
        }});

        edges.forEach((e) => {{
          if (e.from !== e.to) {{
            g.setEdge(e.from, e.to);
          }}
        }});

        window.dagre.layout(g);

        let ok = true;
        DATA.states.forEach((st) => {{
          const n = g.node(st);
          if (!n || isNaN(n.x) || isNaN(n.y)) {{
            ok = false;
          }} else {{
            nodePos[st] = {{ x: Math.round(n.x), y: Math.round(n.y) }};
          }}
        }});
        if (ok) return;
      }} catch (_err) {{
        // fallback em camadas
      }}
    }}
    computeFallbackLayout();
  }}

  function shapeBorderPoint(cx, cy, w, h, tx, ty, pad) {{
    const dx = tx - cx;
    const dy = ty - cy;
    if (dx === 0 && dy === 0) return {{ x: cx, y: cy }};

    if (w === h) {{
      const r = w / 2 + pad;
      const len = Math.hypot(dx, dy);
      return {{ x: cx + (dx / len) * r, y: cy + (dy / len) * r }};
    }}

    const hw = w / 2 + pad;
    const hh = h / 2 + pad;
    const r = hh;
    const flat = Math.max(0, hw - r);
    const len = Math.hypot(dx, dy);
    const ux = dx / len;
    const uy = dy / len;

    const sx = ux !== 0 ? hw / Math.abs(ux) : 1e9;
    const sy = uy !== 0 ? hh / Math.abs(uy) : 1e9;
    let t = Math.min(sx, sy);
    const px = ux * t;

    if (Math.abs(px) > flat) {{
      const ccx = px > 0 ? flat : -flat;
      const b = -2 * (ccx * ux);
      const c = ccx * ccx - r * r;
      const disc = b * b - 4 * c;
      if (disc >= 0) {{
        t = (-b + Math.sqrt(disc)) / 2;
      }}
    }}
    return {{ x: cx + ux * t, y: cy + uy * t }};
  }}

  function pointToSegmentDist(px, py, x1, y1, x2, y2) {{
    const vx = x2 - x1;
    const vy = y2 - y1;
    const lenSq = vx * vx + vy * vy;
    if (lenSq < 1) return {{ dist: Math.hypot(px - x1, py - y1), t: 0 }};
    const t = ((px - x1) * vx + (py - y1) * vy) / lenSq;
    const tc = Math.max(0, Math.min(1, t));
    const projX = x1 + tc * vx;
    const projY = y1 + tc * vy;
    return {{ dist: Math.hypot(px - projX, py - projY), t: t }};
  }}

  let vb = {{ x: 0, y: 0, w: 680, h: 350 }};
  let lastBounds = {{ minX: 0, minY: 0, maxX: 680, maxY: 350 }};

  function applyViewBox() {{
    graphSvg.setAttribute("viewBox", vb.x.toFixed(1) + " " + vb.y.toFixed(1) + " " + vb.w.toFixed(1) + " " + vb.h.toFixed(1));
  }}

  function fitToView() {{
    const rect = graphWrap.getBoundingClientRect();
    const aspect = (rect.width > 40 && rect.height > 40) ? (rect.width / rect.height) : (680 / 345);
    const pad = 28;
    const bw = Math.max(180, (lastBounds.maxX - lastBounds.minX) + pad * 2);
    const bh = Math.max(120, (lastBounds.maxY - lastBounds.minY) + pad * 2);
    const cx = (lastBounds.minX + lastBounds.maxX) / 2;
    const cy = (lastBounds.minY + lastBounds.maxY) / 2;

    let w = bw;
    let h = bh;
    if (w / h > aspect) {{
      h = w / aspect;
    }} else {{
      w = h * aspect;
    }}
    vb = {{ x: cx - w / 2, y: cy - h / 2, w: w, h: h }};
    applyViewBox();
  }}

  function zoomBy(factor, centerX, centerY) {{
    const newW = Math.max(140, Math.min(4000, vb.w * factor));
    const newH = vb.h * (newW / vb.w);
    const cx = centerX !== undefined ? centerX : (vb.x + vb.w / 2);
    const cy = centerY !== undefined ? centerY : (vb.y + vb.h / 2);
    const rx = (cx - vb.x) / vb.w;
    const ry = (cy - vb.y) / vb.h;
    vb.w = newW;
    vb.h = newH;
    vb.x = cx - rx * newW;
    vb.y = cy - ry * newH;
    applyViewBox();
  }}

  let currentGraphState = DATA.initState;
  let currentGraphRule = null;

  function drawGraph(activeState, activeRule) {{
    currentGraphState = activeState;
    currentGraphRule = activeRule;

    const palette = {{
      nodeFill: "#fff8c5",
      nodeActiveFill: "#111111",
      nodeText: "#111111",
      nodeActiveText: "#ffffff",
      edgeStroke: "#555555",
      edgeActiveStroke: "#000000",
      labelBg: "rgba(255,255,255,0.94)",
      labelActiveBg: "#fff1a8",
    }};

    let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
    function expandBounds(x, y) {{
      if (x < minX) minX = x;
      if (y < minY) minY = y;
      if (x > maxX) maxX = x;
      if (y > maxY) maxY = y;
    }}

    let globalMaxY = -Infinity;
    let globalMinY = Infinity;
    DATA.states.forEach((st) => {{
      const p = nodePos[st];
      const d = nodeDim[st];
      expandBounds(p.x - d.w / 2, p.y - d.h / 2);
      expandBounds(p.x + d.w / 2, p.y + d.h / 2);
      if (p.y + d.h / 2 > globalMaxY) globalMaxY = p.y + d.h / 2;
      if (p.y - d.h / 2 < globalMinY) globalMinY = p.y - d.h / 2;
    }});

    let svg =
      '<defs>' +
      '<marker id="arr" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="6.5" markerHeight="6.5" orient="auto">' +
      '<path d="M 0 1 L 10 5 L 0 9 z" fill="' + palette.edgeStroke + '" /></marker>' +
      '<marker id="arrAct" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7.5" markerHeight="7.5" orient="auto">' +
      '<path d="M 0 1 L 10 5 L 0 9 z" fill="' + palette.edgeActiveStroke + '" /></marker>' +
      '</defs>';

    let backEdgeCount = 0;

    edges.forEach((e) => {{
      const p1 = nodePos[e.from];
      const p2 = nodePos[e.to];
      const d1 = nodeDim[e.from];
      const d2 = nodeDim[e.to];
      if (!p1 || !p2) return;

      const isEdgeActive = activeRule && activeRule.from === e.from && activeRule.to === e.to;
      const stroke = isEdgeActive ? palette.edgeActiveStroke : palette.edgeStroke;
      const sw = isEdgeActive ? "2.4" : "1.35";
      const marker = isEdgeActive ? "url(#arrAct)" : "url(#arr)";

      const labels = e.rules.map((r) => {{
        const match = isEdgeActive && r.read === activeRule.read && r.write === activeRule.write && r.dir === activeRule.dir;
        return {{
          text: fmtSym(r.read) + " \\u2192 " + fmtSym(r.write) + ", " + fmtDir(r.dir),
          active: match,
        }};
      }});

      const lineH = 14;
      const boxH = labels.length * lineH + 6;
      const maxLen = Math.max(...labels.map((l) => l.text.length));
      const boxW = maxLen * 6.8 + 12;

      let pathD = "";
      let lx = 0;
      let ly = 0;

      if (e.from === e.to) {{
        const topY = p1.y - d1.h / 2;
        const spread = Math.min(16, d1.w * 0.28);
        const x1 = p1.x - spread;
        const x2 = p1.x + spread;
        const y1 = topY;
        const y2 = topY - 2;
        const loopH = 44;

        pathD =
          "M " + x1.toFixed(1) + " " + y1.toFixed(1) +
          " C " + (x1 - 24).toFixed(1) + " " + (topY - loopH).toFixed(1) + ", " +
          (x2 + 24).toFixed(1) + " " + (topY - loopH).toFixed(1) + ", " +
          x2.toFixed(1) + " " + y2.toFixed(1);

        lx = p1.x;
        ly = topY - loopH - boxH / 2 + 4;
        expandBounds(lx - boxW / 2, ly - boxH / 2);
        expandBounds(lx + boxW / 2, topY - loopH);
      }} else {{
        const hasReverse = Boolean(edgeMap[e.to + "|||" + e.from]);
        const dx = p2.x - p1.x;
        const dy = p2.y - p1.y;
        const dist = Math.hypot(dx, dy) || 1;
        const perpX = -dy / dist;
        const perpY = dx / dist;

        let blocked = false;
        DATA.states.forEach((other) => {{
          if (other === e.from || other === e.to) return;
          const po = nodePos[other];
          const doDim = nodeDim[other];
          const res = pointToSegmentDist(po.x, po.y, p1.x, p1.y, p2.x, p2.y);
          const margin = Math.max(doDim.w, doDim.h) / 2 + 22;
          if (res.t > 0.08 && res.t < 0.92 && res.dist < margin) {{
            blocked = true;
          }}
        }});

        const isBackEdge = dx < -30;
        let cpx = (p1.x + p2.x) / 2;
        let cpy = (p1.y + p2.y) / 2;

        if (isBackEdge && (blocked || Math.abs(dx) > 150)) {{
          backEdgeCount += 1;
          const archY = globalMaxY + 52 + (backEdgeCount - 1) * 34;
          cpy = archY + (archY - (p1.y + p2.y) / 2) * 0.45;
          cpx = (p1.x + p2.x) / 2;
        }} else if (hasReverse) {{
          const curve = 34;
          cpx += perpX * curve;
          cpy += perpY * curve;
        }} else if (blocked) {{
          const midY = (p1.y + p2.y) / 2;
          const goUp = midY <= (globalMinY + globalMaxY) / 2;
          cpy = goUp ? (globalMinY - 46) : (globalMaxY + 46);
        }}

        const startPt = shapeBorderPoint(p1.x, p1.y, d1.w, d1.h, cpx, cpy, 1);
        const endPt = shapeBorderPoint(p2.x, p2.y, d2.w, d2.h, cpx, cpy, 3);

        pathD =
          "M " + startPt.x.toFixed(1) + " " + startPt.y.toFixed(1) +
          " Q " + cpx.toFixed(1) + " " + cpy.toFixed(1) + " " +
          endPt.x.toFixed(1) + " " + endPt.y.toFixed(1);

        lx = 0.25 * startPt.x + 0.5 * cpx + 0.25 * endPt.x;
        ly = 0.25 * startPt.y + 0.5 * cpy + 0.25 * endPt.y;
        expandBounds(lx - boxW / 2, ly - boxH / 2);
        expandBounds(lx + boxW / 2, ly + boxH / 2);
      }}

      svg += '<path d="' + pathD + '" fill="none" stroke="' + stroke + '" stroke-width="' + sw + '" marker-end="' + marker + '" />';

      const rx = lx - boxW / 2;
      const ry = ly - boxH / 2;
      svg += '<rect x="' + rx.toFixed(1) + '" y="' + ry.toFixed(1) +
             '" width="' + boxW.toFixed(1) + '" height="' + boxH.toFixed(1) +
             '" fill="' + palette.labelBg + '" stroke="' + (isEdgeActive ? "#444444" : "#dddddd") +
             '" stroke-width="' + (isEdgeActive ? "1" : "0.7") + '" rx="3" />';

      labels.forEach((lbl, idx) => {{
        const lineTop = ry + 3 + idx * lineH;
        if (lbl.active) {{
          svg += '<rect x="' + (rx + 2).toFixed(1) + '" y="' + lineTop.toFixed(1) +
                 '" width="' + (boxW - 4).toFixed(1) + '" height="' + lineH +
                 '" fill="' + palette.labelActiveBg + '" rx="2" />';
        }}
        const ty = lineTop + 10.5;
        const fill = lbl.active ? "#000000" : "#333333";
        const weight = lbl.active ? "700" : "400";
        svg += '<text x="' + lx.toFixed(1) + '" y="' + ty.toFixed(1) +
               '" text-anchor="middle" font-size="11" font-weight="' + weight +
               '" fill="' + fill + '">' + esc(lbl.text) + '</text>';
      }});
    }});

    const initPos = nodePos[DATA.initState];
    const initDim = nodeDim[DATA.initState];
    if (initPos && initDim) {{
      const tipX = initPos.x - initDim.w / 2 - 2;
      const baseX = tipX - 20;
      const yMid = initPos.y;
      svg += '<polygon points="' +
             baseX.toFixed(1) + ',' + (yMid - 12).toFixed(1) + ' ' +
             baseX.toFixed(1) + ',' + (yMid + 12).toFixed(1) + ' ' +
             tipX.toFixed(1) + ',' + yMid.toFixed(1) +
             '" fill="#ffffff" stroke="#111111" stroke-width="1.6" />';
      expandBounds(baseX - 4, yMid - 14);
      expandBounds(baseX - 4, yMid + 14);
    }}

    DATA.states.forEach((st) => {{
      const p = nodePos[st];
      const d = nodeDim[st];
      const isAct = st === activeState;
      const isAcc = acceptSet.has(st);
      const fill = isAct ? palette.nodeActiveFill : palette.nodeFill;
      const txtFill = isAct ? palette.nodeActiveText : palette.nodeText;
      const x = p.x - d.w / 2;
      const y = p.y - d.h / 2;

      svg += '<g class="node" data-state="' + esc(st) + '" style="cursor:move;">';
      svg += '<rect x="' + x.toFixed(1) + '" y="' + y.toFixed(1) +
             '" width="' + d.w + '" height="' + d.h +
             '" rx="' + d.rx + '" ry="' + d.rx +
             '" fill="' + fill + '" stroke="#111111" stroke-width="' + (isAct ? "2.2" : "1.6") + '" />';
      if (isAcc) {{
        svg += '<rect x="' + (x + 4).toFixed(1) + '" y="' + (y + 4).toFixed(1) +
               '" width="' + (d.w - 8) + '" height="' + (d.h - 8) +
               '" rx="' + Math.max(4, d.rx - 4) + '" ry="' + Math.max(4, d.rx - 4) +
               '" fill="none" stroke="' + (isAct ? "#ffffff" : "#111111") + '" stroke-width="1.3" />';
      }}
      svg += '<text x="' + p.x.toFixed(1) + '" y="' + (p.y + 4).toFixed(1) +
             '" text-anchor="middle" font-size="12" font-weight="700" fill="' + txtFill + '">' + esc(st) + '</text>';
      svg += '</g>';
    }});

    lastBounds = {{ minX: minX, minY: minY, maxX: maxX, maxY: maxY }};
    graphSvg.innerHTML = svg;
  }}

  // ---------- INTERACAO DO DIAGRAMA ----------
  let dragState = null;
  let isPanning = false;
  let panStart = null;

  function clientToSvg(evt) {{
    const rect = graphSvg.getBoundingClientRect();
    return {{
      x: vb.x + ((evt.clientX - rect.left) / rect.width) * vb.w,
      y: vb.y + ((evt.clientY - rect.top) / rect.height) * vb.h,
    }};
  }}

  graphSvg.addEventListener("mousedown", (evt) => {{
    const g = evt.target.closest("g.node");
    if (g) {{
      dragState = g.getAttribute("data-state");
      evt.preventDefault();
    }} else {{
      isPanning = true;
      panStart = {{ clientX: evt.clientX, clientY: evt.clientY, vbX: vb.x, vbY: vb.y }};
      graphSvg.classList.add("panning");
      evt.preventDefault();
    }}
  }});

  window.addEventListener("mousemove", (evt) => {{
    if (dragState) {{
      const pt = clientToSvg(evt);
      nodePos[dragState] = {{ x: Math.round(pt.x), y: Math.round(pt.y) }};
      drawGraph(currentGraphState, currentGraphRule);
    }} else if (isPanning && panStart) {{
      const rect = graphSvg.getBoundingClientRect();
      const dx = (evt.clientX - panStart.clientX) * (vb.w / rect.width);
      const dy = (evt.clientY - panStart.clientY) * (vb.h / rect.height);
      vb.x = panStart.vbX - dx;
      vb.y = panStart.vbY - dy;
      applyViewBox();
    }}
  }});

  window.addEventListener("mouseup", () => {{
    dragState = null;
    isPanning = false;
    panStart = null;
    graphSvg.classList.remove("panning");
  }});

  graphSvg.addEventListener("wheel", (evt) => {{
    evt.preventDefault();
    const pt = clientToSvg(evt);
    const factor = evt.deltaY < 0 ? 0.88 : 1.14;
    zoomBy(factor, pt.x, pt.y);
  }}, {{ passive: false }});

  gZoomIn.addEventListener("click", () => zoomBy(0.82));
  gZoomOut.addEventListener("click", () => zoomBy(1.22));
  gFit.addEventListener("click", () => fitToView());
  gRelayout.addEventListener("click", () => {{
    computeLayout();
    drawGraph(currentGraphState, currentGraphRule);
    fitToView();
  }});
  gExport.addEventListener("click", () => {{
    const pad = 24;
    const x = lastBounds.minX - pad;
    const y = lastBounds.minY - pad;
    const w = Math.max(200, lastBounds.maxX - lastBounds.minX + pad * 2);
    const h = Math.max(140, lastBounds.maxY - lastBounds.minY + pad * 2);
    const svgFile =
      '<?xml version="1.0" encoding="UTF-8"?>\\n' +
      '<svg xmlns="http://www.w3.org/2000/svg" viewBox="' +
      x.toFixed(1) + ' ' + y.toFixed(1) + ' ' + w.toFixed(1) + ' ' + h.toFixed(1) +
      '" width="' + Math.round(w * 1.6) + '" height="' + Math.round(h * 1.6) +
      '" style="font-family: Consolas, Courier New, monospace; background: #ffffff;">' +
      '<rect x="' + x.toFixed(1) + '" y="' + y.toFixed(1) + '" width="' + w.toFixed(1) + '" height="' + h.toFixed(1) + '" fill="#ffffff" />' +
      graphSvg.innerHTML +
      '</svg>';
    const blob = new Blob([svgFile], {{ type: "image/svg+xml;charset=utf-8" }});
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = "diagrama_estados.svg";
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  }});

  // ---------- ANIMACAO DA FITA, LOG E CONTROLES ----------
  function updateStatusBar(idx, stateName, headPos, readSym, ruleTxt, phaseLabel) {{
    let stText = "EM EXECUÇÃO";
    let stCls = "st-run";
    if (idx === totalSteps && !phaseLabel) {{
      if (DATA.status === "ACCEPTED") {{ stText = "ACEITA"; stCls = "st-acc"; }}
      else if (DATA.status === "REJECTED") {{ stText = "REJEITADA"; stCls = "st-rej"; }}
      else {{ stText = "LIMITE ATINGIDO"; stCls = "st-lim"; }}
    }}
    let html =
      "<span><b>Estado:</b> " + esc(stateName) + "</span>" +
      "<span><b>Passo:</b> " + idx + " / " + totalSteps + "</span>" +
      "<span><b>Cabeçote:</b> " + headPos + "</span>" +
      "<span><b>Lendo:</b> '" + esc(readSym) + "'</span>" +
      "<span><b>Regra:</b> " + esc(ruleTxt) + "</span>" +
      '<span><b>Situação:</b> <b class="' + stCls + '">' + stText + "</b></span>";
    if (phaseLabel) {{
      html += '<span class="phase-tag">' + esc(phaseLabel) + "</span>";
    }}
    statusBar.innerHTML = html;
  }}

  function positionTape(headPos, durationMs) {{
    tapeTrack.style.transition = durationMs > 0
      ? "transform " + durationMs + "ms cubic-bezier(0.25, 0.8, 0.25, 1)"
      : "none";
    const offset = (headPos - DATA.minIdx) * CELL_W + CELL_W / 2;
    tapeTrack.style.transform = "translateX(" + (-offset) + "px)";
  }}

  function paintCells(tapeObj, activeHead, modeClass) {{
    for (let i = DATA.minIdx; i <= DATA.maxIdx; i++) {{
      const val = tapeObj[String(i)] !== undefined ? tapeObj[String(i)] : DATA.blank;
      const node = cellNodes[i];
      node.textContent = val;
      node.className = "cell";
      if (i === activeHead) node.classList.add(modeClass || "active");
    }}
  }}

  function updateLogAndTrace(idx) {{
    let logHtml = "";
    for (let k = 0; k <= idx; k++) {{
      const s = DATA.steps[k];
      const cls = k === idx ? "log-line curr" : "log-line";
      const mark = k === idx ? "&gt;" : "&nbsp;";
      logHtml += '<div class="' + cls + '">' + mark + " [" + String(s.step).padStart(2, "0") + "] " + esc(s.log) + "</div>";
    }}
    if (idx === totalSteps) {{
      logHtml += '<div class="log-end">FIM: ' + esc(DATA.message) + "</div>";
    }}
    logPanel.innerHTML = logHtml;
    logPanel.scrollTop = logPanel.scrollHeight;

    let traceHtml = "";
    for (let k = 0; k <= idx; k++) {{
      const s = DATA.steps[k];
      let cells = "";
      for (let c = DATA.traceMin; c <= DATA.traceMax; c++) {{
        const v = s.tape[String(c)] !== undefined ? s.tape[String(c)] : DATA.blank;
        const hCls = c === s.head ? "t-cell head" : "t-cell";
        cells += '<span class="' + hCls + '">' + esc(v) + "</span>";
      }}
      const rCls = k === idx ? "trace-row curr" : "trace-row";
      traceHtml +=
        '<div class="' + rCls + '">' +
        '<span class="t-step">[' + String(s.step).padStart(2, "0") + "]</span>" +
        '<span class="t-state">' + esc(s.state) + "</span>" +
        '<span class="t-cells">' + cells + "</span>" +
        "</div>";
    }}
    tracePanel.innerHTML = traceHtml;
    tracePanel.scrollTop = tracePanel.scrollHeight;
  }}

  function updateControls() {{
    btnStart.disabled = currIdx === 0;
    btnPrev.disabled = currIdx === 0;
    btnNext.disabled = currIdx === totalSteps;
    btnEnd.disabled = currIdx === totalSteps;
    btnPlay.disabled = totalSteps === 0;
    btnPlay.textContent = playing ? "Pausar" : "Animar";
    stepRange.value = String(currIdx);
    rangeLabel.textContent = currIdx + " / " + totalSteps;
  }}

  function renderInstant(idx, slideMs) {{
    const snap = DATA.steps[idx];
    const sym = snap.tape[String(snap.head)] !== undefined ? snap.tape[String(snap.head)] : DATA.blank;
    updateStatusBar(idx, snap.state, snap.head, sym, snap.rule, "");
    paintCells(snap.tape, snap.head, "active");
    positionTape(snap.head, slideMs || 0);
    drawGraph(snap.state, snap.ruleObj);
    updateLogAndTrace(idx);
    updateControls();
  }}

  function animateForward(fromIdx, toIdx, onDone) {{
    clearTimers();
    const prev = DATA.steps[fromIdx];
    const next = DATA.steps[toIdx];
    const r = next.ruleObj;
    const totalMs = Number(speedSel.value);

    if (!r || totalMs <= 210) {{
      currIdx = toIdx;
      renderInstant(toIdx, 140);
      timers.push(setTimeout(() => {{ if (onDone) onDone(); }}, totalMs));
      return;
    }}

    const tRead = Math.floor(totalMs * 0.28);
    const tWrite = Math.floor(totalMs * 0.32);
    const tMove = Math.max(120, totalMs - tRead - tWrite);

    updateStatusBar(fromIdx, prev.state, prev.head, r.read, next.rule, "1. lendo '" + r.read + "'");
    paintCells(prev.tape, prev.head, "reading");
    drawGraph(prev.state, null);

    timers.push(setTimeout(() => {{
      const midTape = Object.assign({{}}, prev.tape);
      midTape[String(prev.head)] = r.write;
      updateStatusBar(fromIdx, prev.state, prev.head, r.write, next.rule, "2. escrevendo '" + r.write + "'");
      paintCells(midTape, prev.head, "writing");
      drawGraph(prev.state, r);
    }}, tRead));

    timers.push(setTimeout(() => {{
      currIdx = toIdx;
      const newSym = next.tape[String(next.head)] !== undefined ? next.tape[String(next.head)] : DATA.blank;
      const dirLabel = r.dir === ">" ? "direita" : (r.dir === "<" ? "esquerda" : "parado");
      updateStatusBar(toIdx, next.state, next.head, newSym, next.rule, toIdx === totalSteps ? "" : "3. movendo (" + dirLabel + ")");
      paintCells(next.tape, next.head, "active");
      positionTape(next.head, Math.min(260, tMove));
      drawGraph(next.state, r);
      updateLogAndTrace(toIdx);
      updateControls();
    }}, tRead + tWrite));

    timers.push(setTimeout(() => {{
      const newSym = next.tape[String(next.head)] !== undefined ? next.tape[String(next.head)] : DATA.blank;
      updateStatusBar(toIdx, next.state, next.head, newSym, next.rule, "");
      if (onDone) onDone();
    }}, totalMs));
  }}

  function playLoop() {{
    if (!playing) return;
    if (currIdx < totalSteps) {{
      animateForward(currIdx, currIdx + 1, () => {{
        if (playing) playLoop();
      }});
    }} else {{
      stopPlay();
      updateControls();
    }}
  }}

  btnStart.addEventListener("click", () => {{ stopPlay(); currIdx = 0; renderInstant(0, 150); }});
  btnPrev.addEventListener("click", () => {{
    stopPlay();
    if (currIdx > 0) {{ currIdx--; renderInstant(currIdx, 150); }}
  }});
  btnNext.addEventListener("click", () => {{
    stopPlay();
    if (currIdx < totalSteps) {{ animateForward(currIdx, currIdx + 1, null); }}
  }});
  btnEnd.addEventListener("click", () => {{ stopPlay(); currIdx = totalSteps; renderInstant(currIdx, 150); }});

  btnPlay.addEventListener("click", () => {{
    if (playing) {{
      stopPlay();
      renderInstant(currIdx, 0);
    }} else {{
      if (currIdx === totalSteps) {{
        currIdx = 0;
        renderInstant(0, 0);
      }}
      playing = true;
      btnPlay.textContent = "Pausar";
      playLoop();
    }}
  }});

  stepRange.addEventListener("input", () => {{
    stopPlay();
    currIdx = Number(stepRange.value);
    renderInstant(currIdx, 0);
  }});

  tabLogBtn.addEventListener("click", () => {{
    tabLogBtn.classList.add("active");
    tabTraceBtn.classList.remove("active");
    logPanel.classList.remove("hidden");
    tracePanel.classList.add("hidden");
  }});

  tabTraceBtn.addEventListener("click", () => {{
    tabTraceBtn.classList.add("active");
    tabLogBtn.classList.remove("active");
    tracePanel.classList.remove("hidden");
    logPanel.classList.add("hidden");
    tracePanel.scrollTop = tracePanel.scrollHeight;
  }});

  window.addEventListener("keydown", (evt) => {{
    if (evt.key === "ArrowRight") {{
      evt.preventDefault();
      if (currIdx < totalSteps) {{
        stopPlay();
        animateForward(currIdx, currIdx + 1, null);
      }}
    }} else if (evt.key === "ArrowLeft") {{
      evt.preventDefault();
      if (currIdx > 0) {{
        stopPlay();
        currIdx--;
        renderInstant(currIdx, 140);
      }}
    }} else if (evt.code === "Space") {{
      evt.preventDefault();
      btnPlay.click();
    }}
  }});

  computeLayout();
  renderInstant(0, 0);
  fitToView();
</script>
</body>
</html>"""


init_state()

st.markdown("### Máquina de Turing")

c_in, c_btn = st.columns([4.4, 1.0])
with c_in:
    st.text_input(
        "Entrada na fita (vazio = cadeia vazia)",
        key="tape_input",
        on_change=execute_machine,
        placeholder="Digite a palavra de entrada...",
    )
with c_btn:
    st.markdown("<div style='height:28px;'></div>", unsafe_allow_html=True)
    st.button("Executar", on_click=execute_machine, use_container_width=True)

with st.expander("Importar ou baixar arquivo da máquina (.txt)"):
    u_col, d_col = st.columns([2.5, 1.2])
    with u_col:
        st.file_uploader(
            "Selecionar arquivo .txt",
            type=["txt"],
            key="tm_file_uploader",
            on_change=on_file_upload,
            label_visibility="collapsed",
        )
    with d_col:
        export_content = format_file_text(
            st.session_state.init_state,
            st.session_state.accept_str,
            st.session_state.blank_sym,
            st.session_state.rules_text,
        )
        st.download_button(
            "Baixar máquina atual (.txt)",
            data=export_content.encode("utf-8"),
            file_name="maquina_turing.txt",
            mime="text/plain",
            use_container_width=True,
        )

if st.session_state.parse_error:
    st.markdown(
        f"""
        <div style="padding:10px 12px; background:#fff5f5; border:1px solid #a40e26;
        color:#a40e26; font-family:Consolas, monospace; font-size:13px; margin:8px 0;">
            ERRO: {st.session_state.parse_error}
        </div>
        """,
        unsafe_allow_html=True,
    )
elif st.session_state.result is not None and st.session_state.machine is not None:
    view_html = build_machine_view(st.session_state.machine, st.session_state.result)
    components.html(view_html, height=670, scrolling=False)
else:
    st.markdown(
        """
        <div style="padding:22px 16px; border:1px dashed #bbbbbb; background:#fafafa;
        text-align:center; font-family:Consolas, monospace; font-size:13px; color:#555555; margin:8px 0;">
            Nenhuma regra carregada. Importe um arquivo .txt acima ou defina os estados e transições abaixo.
        </div>
        """,
        unsafe_allow_html=True,
    )

st.markdown("---")
st.markdown("#### Definição dos estados e regras")

s1, s2, s3 = st.columns([1.4, 2.2, 0.8])
with s1:
    st.text_input("Estado inicial (q0)", key="init_state", on_change=execute_machine)
with s2:
    st.text_input("Estados de aceitação (F, separados por vírgula)", key="accept_str", on_change=execute_machine)
with s3:
    st.text_input("Branco (B)", key="blank_sym", max_chars=1, on_change=execute_machine)

col_rules, col_formal = st.columns([1.25, 0.95], gap="medium")

with col_rules:
    tab_text, tab_table = st.tabs(["Regras (texto)", "Regras (tabela editável)"])

    with tab_text:
        st.text_area(
            "Regras",
            key="rules_text",
            height=290,
            label_visibility="collapsed",
            placeholder="Exemplo (1 linha por regra):\nq0, a -> q1, _, >\nq1, _ -> qAccept, _, -",
        )
        b_apply, b_lim = st.columns([1.2, 2.0])
        with b_apply:
            st.button("Aplicar regras", on_click=execute_machine, use_container_width=True)
        with b_lim:
            with st.expander("Limite de passos"):
                st.number_input(
                    "Máximo de passos",
                    min_value=10,
                    max_value=10000,
                    step=100,
                    key="max_steps",
                    on_change=execute_machine,
                )

    with tab_table:
        base_rows = []
        if st.session_state.machine is not None:
            base_rows = transitions_to_rows(st.session_state.machine.transitions)
        elif st.session_state.rules_text.strip() == "":
            base_rows = [{"estado": "q0", "le": "a", "proximo": "qAccept", "escreve": "a", "direcao": ">"}]

        edited_rows = st.data_editor(
            base_rows,
            num_rows="dynamic",
            use_container_width=True,
            height=290,
            hide_index=True,
            column_config={
                "estado": st.column_config.TextColumn("Estado"),
                "le": st.column_config.TextColumn("Lê", max_chars=1),
                "proximo": st.column_config.TextColumn("Próximo estado"),
                "escreve": st.column_config.TextColumn("Escreve", max_chars=1),
                "direcao": st.column_config.SelectboxColumn("Direção", options=["<", ">", "-"]),
            },
        )
        if st.button("Aplicar tabela"):
            st.session_state.rules_text = rows_to_rules_text(edited_rows)
            execute_machine()
            st.rerun()

with col_formal:
    tab_tuple, tab_help = st.tabs(["Definição formal (7-tupla)", "Sintaxe das regras"])

    with tab_tuple:
        if st.session_state.machine is not None:
            m: TuringMachine = st.session_state.machine
            states = m.get_states()
            tape_alpha = m.get_tape_alphabet()
            st.code(
                f"M = (Q, Sigma, Gamma, delta, q0, B, F)\n"
                f"Q     = {{{', '.join(states)}}}\n"
                f"Gamma = {{{', '.join(tape_alpha)}}}\n"
                f"q0    = {m.init_state}\n"
                f"B     = {m.blank}\n"
                f"F     = {{{', '.join(sorted(m.accept_states))}}}\n"
                f"Regras: {len(m.transitions)}",
                language=None,
            )
            rows = transitions_to_rows(m.transitions)
            st.dataframe(rows, hide_index=True, use_container_width=True, height=160)
        else:
            st.caption("Carregue ou aplique as regras para gerar a 7-tupla e a tabela.")

    with tab_help:
        st.code(
            "// Formato direto (1 linha por regra):\n"
            "estado, simbolo_lido -> novo_estado, escreve, direcao\n\n"
            "// Exemplo (voce pode nomear os estados como quiser):\n"
            "inicio, a -> busca_fim, _, >\n"
            "busca_fim, _ -> aceita, _, -\n\n"
            "// Direcoes: '<' (esquerda), '>' (direita), '-' (parado)\n"
            "// Comentarios iniciam com //\n"
            "// Arquivos .txt tambem aceitam:\n"
            "//   init: <estado_inicial>\n"
            "//   accept: <estado1, estado2>\n"
            "//   blank: _",
            language=None,
        )