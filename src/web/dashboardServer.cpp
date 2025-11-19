#include "web/dashboardServer.hpp"
#include "web/chainApi.hpp"
#include "web/dndApi.hpp"

#include <thirdparty/httplib.h>
#include <filesystem>
#include <iostream>

DashboardServer::DashboardServer(int port,
                                 const std::string& reportsDir,
                                 const std::string& binaryPath)
    : port(port), reportsDir(reportsDir), binaryPath(binaryPath)
{
}

void DashboardServer::start()
{
    namespace fs = std::filesystem;

    // Ordner erstellen falls er fehlt
    if (!fs::exists(reportsDir)) {
        std::cout << "[DashboardServer] Creating folder: " << reportsDir << "\n";
        fs::create_directories(reportsDir);
    }

    httplib::Server server;

    // ========== BASIC TEST ENDPOINTS ==========
    server.Get("/ping", [&](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"pong\":true}", "application/json");
    });

    server.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    // ========== GOSSIP: TX ==========
    server.Post("/gossip/tx", [&](const httplib::Request& req,
                                 httplib::Response& res)
    {
        std::cout << "[GOSSIP] Received TX payload: " << req.body << "\n";

        res.status = 200;
        res.set_content("{\"received\":true}", "application/json");
    });

    // ========== GOSSIP: BLOCK ==========
    server.Post("/gossip/block", [&](const httplib::Request& req,
                                    httplib::Response& res)
    {
        std::cout << "[GOSSIP] Received BLOCK payload: " << req.body << "\n";

        res.status = 200;
        res.set_content("{\"received\":true}", "application/json");
    });

    // ========== ROOT PAGE ==========
    server.Get("/", [&](const httplib::Request&, httplib::Response& res) {
        const char* html = R"HTML(
    <!DOCTYPE html>
    <html lang="de">
    <head>
      <meta charset="UTF-8" />
      <title>Blockchain + DnD Dashboard</title>
      <style>
        :root {
          color-scheme: dark;
          --bg: #111827;
          --bg-alt: #020617;
          --card: #111827;
          --card-alt: #020617;
          --accent: #22c55e;
          --accent-soft: rgba(34,197,94,0.15);
          --text: #e5e7eb;
          --muted: #9ca3af;
          --border: #1f2937;
          --danger: #ef4444;
          --danger-soft: rgba(239,68,68,0.1);
          --warn: #f59e0b;
          --warn-soft: rgba(245,158,11,0.1);
        }
        * { box-sizing: border-box; }
        body {
          margin: 0;
          font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
          background: radial-gradient(circle at top left, #1f2937, #020617 50%, #000 100%);
          color: var(--text);
          min-height: 100vh;
        }
        .layout {
          max-width: 1200px;
          margin: 0 auto;
          padding: 24px 16px 40px;
        }
        header {
          display: flex;
          align-items: center;
          justify-content: space-between;
          gap: 16px;
          margin-bottom: 24px;
        }
        .title {
          display: flex;
          flex-direction: column;
          gap: 4px;
        }
        h1 {
          margin: 0;
          font-size: 1.8rem;
          letter-spacing: 0.03em;
          display: flex;
          align-items: center;
          gap: 8px;
        }
        h1 span.logo-pill {
          font-size: 0.9rem;
          padding: 4px 10px;
          border-radius: 999px;
          background: linear-gradient(90deg, #22c55e, #0ea5e9);
          color: #020617;
          font-weight: 600;
        }
        .subtitle {
          margin: 0;
          font-size: 0.9rem;
          color: var(--muted);
        }
        .status-row {
          display: flex;
          flex-wrap: wrap;
          gap: 12px;
          margin-top: 8px;
          font-size: 0.85rem;
          color: var(--muted);
        }
        .badge {
          display: inline-flex;
          align-items: center;
          gap: 6px;
          padding: 4px 10px;
          border-radius: 999px;
          background: rgba(15,23,42,0.9);
          border: 1px solid rgba(51,65,85,0.8);
        }
        .badge-dot {
          width: 7px;
          height: 7px;
          border-radius: 999px;
          background: var(--accent);
          box-shadow: 0 0 0 4px var(--accent-soft);
        }
        .badge-dot.warn {
          background: var(--warn);
          box-shadow: 0 0 0 4px var(--warn-soft);
        }
        .badge-dot.danger {
          background: var(--danger);
          box-shadow: 0 0 0 4px var(--danger-soft);
        }
        .badge-label { opacity: 0.9; }

        .controls {
          display: flex;
          gap: 10px;
          flex-wrap: wrap;
          justify-content: flex-end;
        }
        button {
          border-radius: 999px;
          border: 1px solid rgba(75,85,99,0.8);
          background: linear-gradient(135deg, rgba(15,23,42,0.95), rgba(15,23,42,0.8));
          color: var(--text);
          padding: 6px 14px;
          font-size: 0.85rem;
          cursor: pointer;
          display: inline-flex;
          align-items: center;
          gap: 6px;
          transition: background 0.15s ease, transform 0.08s ease, box-shadow 0.15s ease;
        }
        button.primary {
          border-color: rgba(34,197,94,0.7);
          background: linear-gradient(135deg, #22c55e, #16a34a);
          color: #020617;
          font-weight: 600;
          box-shadow: 0 8px 18px rgba(34,197,94,0.25);
        }
        button:hover {
          transform: translateY(-1px);
          box-shadow: 0 6px 16px rgba(0,0,0,0.35);
        }
        button.primary:hover {
          box-shadow: 0 10px 22px rgba(34,197,94,0.35);
        }
        button:active {
          transform: translateY(0);
          box-shadow: none;
        }

        main {
          display: grid;
          grid-template-columns: 1.3fr 1fr;
          gap: 16px;
        }
        @media (max-width: 900px) {
          main {
            grid-template-columns: 1fr;
          }
        }

        .card {
          border-radius: 14px;
          padding: 14px 14px 10px;
          background: radial-gradient(circle at top left, rgba(15,23,42,0.95), rgba(15,23,42,0.98));
          border: 1px solid rgba(30,64,175,0.6);
          box-shadow:
            0 18px 35px rgba(0,0,0,0.55),
            inset 0 0 0 1px rgba(15,23,42,0.7);
          backdrop-filter: blur(14px);
        }
        .card-header {
          display: flex;
          justify-content: space-between;
          align-items: baseline;
          margin-bottom: 8px;
        }
        .card-title {
          font-size: 0.95rem;
          text-transform: uppercase;
          letter-spacing: 0.16em;
          color: var(--muted);
        }
        .card-meta {
          font-size: 0.8rem;
          color: var(--muted);
          opacity: 0.9;
        }
        .pill-bar {
          display: flex;
          gap: 8px;
          flex-wrap: wrap;
          margin-bottom: 6px;
        }
        .pill {
          font-size: 0.78rem;
          padding: 3px 8px;
          border-radius: 999px;
          background: rgba(15,23,42,0.9);
          border: 1px solid rgba(51,65,85,0.9);
          color: var(--muted);
        }
        .pill strong { color: var(--text); }

        .divider {
          height: 1px;
          border-radius: 999px;
          background: linear-gradient(90deg, transparent, rgba(148,163,184,0.35), transparent);
          margin: 6px 0 10px;
        }

        table {
          width: 100%;
          border-collapse: collapse;
          font-size: 0.8rem;
        }
        thead {
          text-transform: uppercase;
          font-size: 0.7rem;
          letter-spacing: 0.15em;
          color: var(--muted);
        }
        th, td {
          padding: 4px 6px;
          text-align: left;
          white-space: nowrap;
          overflow: hidden;
          text-overflow: ellipsis;
        }
        tbody tr {
          border-radius: 8px;
          background: rgba(15,23,42,0.75);
        }
        tbody tr:nth-child(2n) {
          background: rgba(15,23,42,0.9);
        }
        tbody tr:hover {
          background: rgba(30,64,175,0.65);
        }
        tbody td.hash {
          font-family: "JetBrains Mono", ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace;
          font-size: 0.75rem;
          color: var(--muted);
        }
        .tag {
          display: inline-flex;
          align-items: center;
          padding: 2px 6px;
          border-radius: 999px;
          font-size: 0.7rem;
          text-transform: uppercase;
          letter-spacing: 0.08em;
          border: 1px solid rgba(55,65,81,0.8);
          color: var(--muted);
        }
        .tag.dnd {
          border-color: rgba(34,197,94,0.7);
          color: #bbf7d0;
          background: rgba(34,197,94,0.05);
        }
        .tag.sys {
          border-color: rgba(59,130,246,0.7);
          color: #bfdbfe;
          background: rgba(37,99,235,0.08);
        }

        .state-section {
          margin-bottom: 8px;
        }
        .state-title {
          font-size: 0.8rem;
          text-transform: uppercase;
          letter-spacing: 0.15em;
          color: var(--muted);
          margin-bottom: 4px;
        }
        .state-list {
          max-height: 180px;
          overflow: auto;
          border-radius: 10px;
          border: 1px solid rgba(30,64,175,0.6);
          background: rgba(15,23,42,0.8);
          padding: 4px;
        }
        .state-row {
          display: flex;
          justify-content: space-between;
          align-items: center;
          padding: 3px 6px;
          border-radius: 6px;
          font-size: 0.8rem;
        }
        .state-row:nth-child(2n) {
          background: rgba(15,23,42,0.95);
        }
        .state-row span.id {
          font-weight: 500;
        }
        .state-row span.meta {
          font-size: 0.75rem;
          color: var(--muted);
        }
        .hp-bar {
          position: relative;
          height: 6px;
          border-radius: 999px;
          background: rgba(15,23,42,0.95);
          overflow: hidden;
          width: 80px;
        }
        .hp-bar-inner {
          position: absolute;
          top: 0;
          left: 0;
          bottom: 0;
          border-radius: 999px;
          background: linear-gradient(90deg, #22c55e, #16a34a);
          transition: width 0.2s ease-out;
        }
        .hp-bar-inner.low {
          background: linear-gradient(90deg, #f97316, #ef4444);
        }
        .hp-label {
          font-size: 0.72rem;
          color: var(--muted);
          margin-left: 6px;
        }

        .enc-row {
          display: flex;
          flex-direction: column;
          gap: 2px;
          padding: 4px 6px;
          border-radius: 6px;
          margin-bottom: 2px;
          background: rgba(15,23,42,0.98);
        }
        .enc-header-line {
          display: flex;
          justify-content: space-between;
          align-items: baseline;
          font-size: 0.78rem;
        }
        .enc-header-line span.id {
          font-weight: 500;
        }
        .enc-header-line span.meta {
          font-size: 0.7rem;
          color: var(--muted);
        }
        .enc-actors {
          font-size: 0.7rem;
          color: var(--muted);
          display: flex;
          flex-wrap: wrap;
          gap: 6px;
          margin-top: 2px;
        }
        .enc-pill {
          padding: 1px 6px;
          border-radius: 999px;
          background: rgba(15,23,42,0.9);
          border: 1px solid rgba(55,65,81,0.9);
        }
        .enc-pill.active {
          border-color: rgba(34,197,94,0.9);
          color: #bbf7d0;
        }
        .small-muted {
          font-size: 0.75rem;
          color: var(--muted);
          margin-top: 4px;
        }
        .last-updated {
          font-size: 0.7rem;
          color: var(--muted);
          text-align: right;
          margin-top: 6px;
        }
      </style>
    </head>
    <body>
      <div class="layout">
        <header>
          <div class="title">
            <h1>
              <span>On-Chain DnD</span>
              <span class="logo-pill">PoA Blockchain Node</span>
            </h1>
            <p class="subtitle">Live-Ansicht: Blockchain + DnD State, direkt aus deinem Node.</p>
            <div class="status-row">
              <div class="badge">
                <span class="badge-dot" id="dot-node"></span>
                <span class="badge-label" id="node-status">Node: offline?</span>
              </div>
              <div class="badge">
                <span class="badge-dot warn" id="dot-peers"></span>
                <span class="badge-label" id="peers-status">Peers: unbekannt</span>
              </div>
              <div class="badge">
                <span class="badge-dot" id="dot-dnd"></span>
                <span class="badge-label" id="dnd-status">DnD-State: initialisiere…</span>
              </div>
            </div>
          </div>
          <div class="controls">
            <button id="btn-refresh">
              ⟳ Manuell aktualisieren
            </button>
            <button class="primary" id="btn-toggle-auto">
              ● Auto-Refresh: AN
            </button>
          </div>
        </header>

        <main>
          <!-- LEFT: Chain & Blocks -->
          <section class="card">
            <div class="card-header">
              <div>
                <div class="card-title">Blockchain</div>
                <div class="pill-bar">
                  <div class="pill">
                    Höhe: <strong id="height-value">–</strong>
                  </div>
                  <div class="pill">
                    Letzter Block: <strong id="last-block-ts">–</strong>
                  </div>
                </div>
              </div>
              <div class="card-meta" id="chain-meta">Lade Chain…</div>
            </div>

            <div class="divider"></div>

            <table>
              <thead>
                <tr>
                  <th>Höhe</th>
                  <th>Hash</th>
                  <th>TXs</th>
                  <th>Typ</th>
                  <th>Zeit</th>
                </tr>
              </thead>
              <tbody id="blocks-tbody">
                <tr>
                  <td colspan="5" class="small-muted">Keine Blöcke geladen.</td>
                </tr>
              </tbody>
            </table>
          </section>

          <!-- RIGHT: DnD State -->
          <section class="card">
            <div class="card-header">
              <div>
                <div class="card-title">DnD State</div>
                <div class="pill-bar">
                  <div class="pill">
                    Characters: <strong id="count-chars">–</strong>
                  </div>
                  <div class="pill">
                    Monster: <strong id="count-mons">–</strong>
                  </div>
                  <div class="pill">
                    Encounters: <strong id="count-encs">–</strong>
                  </div>
                </div>
              </div>
              <div class="card-meta" id="dnd-meta">Warte auf Daten…</div>
            </div>

            <div class="divider"></div>

            <div class="state-section">
              <div class="state-title">Monsters (HP)</div>
              <div class="state-list" id="monsters-list">
                <div class="small-muted">Noch keine Monster im State.</div>
              </div>
            </div>

            <div class="state-section">
              <div class="state-title">Encounters</div>
              <div class="state-list" id="encounters-list">
                <div class="small-muted">Noch keine Encounters gestartet.</div>
              </div>
            </div>

            <div class="state-section">
              <div class="state-title">Characters</div>
              <div class="state-list" id="characters-list">
                <div class="small-muted">Noch keine Characters auf der Chain.</div>
              </div>
            </div>

            <div class="last-updated" id="last-updated">Letzte Aktualisierung: –</div>
          </section>
        </main>
      </div>

      <script>
        let autoRefresh = true;
        let autoTimer = null;

        const heightValue = document.getElementById('height-value');
        const lastBlockTs = document.getElementById('last-block-ts');
        const chainMeta   = document.getElementById('chain-meta');
        const blocksTbody = document.getElementById('blocks-tbody');

        const countChars = document.getElementById('count-chars');
        const countMons  = document.getElementById('count-mons');
        const countEncs  = document.getElementById('count-encs');
        const monstersList   = document.getElementById('monsters-list');
        const encountersList = document.getElementById('encounters-list');
        const charactersList = document.getElementById('characters-list');
        const dndMeta        = document.getElementById('dnd-meta');
        const lastUpdated    = document.getElementById('last-updated');

        const nodeDot  = document.getElementById('dot-node');
        const nodeText = document.getElementById('node-status');
        const dndDot   = document.getElementById('dot-dnd');
        const dndText  = document.getElementById('dnd-status');
        const peersDot = document.getElementById('dot-peers');
        const peersText= document.getElementById('peers-status');

        function setDot(el, cls) {
          el.classList.remove('warn','danger');
          if (cls) el.classList.add(cls);
        }

        function formatTs(sec) {
          if (!sec) return "–";
          const d = new Date(sec * 1000);
          return d.toLocaleString();
        }

        async function loadChain() {
          try {
            const hRes = await fetch('/chain/height');
            if (!hRes.ok) throw new Error('height failed');
            const hJson = await hRes.json();
            const height = hJson.height ?? hJson.H || 0;

            heightValue.textContent = height;
            nodeText.textContent = 'Node: online';
            setDot(nodeDot, null);

            if (height === 0) {
              chainMeta.textContent = 'Nur Genesis-Block vorhanden.';
            } else {
              chainMeta.textContent = 'Letzte ' + Math.min(10, height+1) + ' Blöcke';
            }

            const maxBlocks = 10;
            const from = Math.max(0, height - (maxBlocks - 1));

            const blockPromises = [];
            for (let i = height; i >= from; --i) {
              blockPromises.push(fetch('/chain/block/' + i).then(r => r.json()));
            }
            const blocks = await Promise.all(blockPromises);

            blocksTbody.innerHTML = '';
            blocks.forEach(b => {
              const tr = document.createElement('tr');

              const txCount = (b.transactions && b.transactions.length) || b.txCount || 0;
              const hash    = (b.hashHex || (b.header && b.header.hashHex)) || '–';
              const ts      = (b.header && b.header.timestamp) || b.timestamp || 0;

              const hasDnd  = (b.transactions || []).some(t => t.isDnd || t.dnd === true || t.payloadType === 'dnd');

              tr.innerHTML = `
                <td>${b.header ? b.header.height : (b.height ?? '?')}</td>
                <td class="hash">${hash.substring(0,16)}…</td>
                <td>${txCount}</td>
                <td><span class="tag ${hasDnd ? 'dnd' : 'sys'}">${hasDnd ? 'DnD' : 'SYS'}</span></td>
                <td>${formatTs(ts)}</td>
              `;
              blocksTbody.appendChild(tr);
            });

            if (blocks.length > 0) {
              const ts = (blocks[0].header && blocks[0].header.timestamp) || blocks[0].timestamp || 0;
              lastBlockTs.textContent = formatTs(ts);
            } else {
              lastBlockTs.textContent = "–";
            }

          } catch (e) {
            nodeText.textContent = 'Node: offline oder /chain-API nicht erreichbar';
            setDot(nodeDot, 'danger');
          }
        }

        async function loadDnd() {
          try {
            const res = await fetch('/dnd/state');
            if (!res.ok) throw new Error('state failed');
            const j = await res.json();

            dndText.textContent = 'DnD-State: OK';
            setDot(dndDot, null);

            const chars = j.characters || {};
            const mons  = j.monsters || {};
            const encs  = j.encounters || {};

            const cList = Object.values(chars);
            const mList = Object.values(mons);
            const eList = Object.values(encs);

            countChars.textContent = cList.length;
            countMons.textContent  = mList.length;
            countEncs.textContent  = eList.length;

            dndMeta.textContent = cList.length + ' Chars · ' +
                                  mList.length + ' Monsters · ' +
                                  eList.length + ' Encounters';

            // Monsters
            monstersList.innerHTML = '';
            if (mList.length === 0) {
              monstersList.innerHTML = '<div class="small-muted">Keine Monster on-chain.</div>';
            } else {
              mList.forEach(m => {
                const row = document.createElement('div');
                row.className = 'state-row';

                const hp = m.hp ?? 0;
                const max = m.maxHp || m.hpMax || 1;
                const perc = Math.max(0, Math.min(100, Math.round(hp / max * 100)));
                const low = perc <= 30 ? 'low' : '';

                row.innerHTML = `
                  <div>
                    <span class="id">${m.id || 'monster'}</span>
                    <span class="meta">(${hp}/${max} HP)</span>
                  </div>
                  <div style="display:flex;align-items:center;">
                    <div class="hp-bar">
                      <div class="hp-bar-inner ${low}" style="width:${perc}%;"></div>
                    </div>
                    <span class="hp-label">${perc}%</span>
                  </div>
                `;
                monstersList.appendChild(row);
              });
            }

            // Characters
            charactersList.innerHTML = '';
            if (cList.length === 0) {
              charactersList.innerHTML = '<div class="small-muted">Keine Characters on-chain.</div>';
            } else {
              cList.forEach(c => {
                const row = document.createElement('div');
                row.className = 'state-row';
                const name = c.name || c.id || 'char';
                const hp   = c.hpCurrent ?? c.hp ?? 0;
                const max  = c.hpMax || c.maxHp || 1;

                row.innerHTML = `
                  <div>
                    <span class="id">${name}</span>
                    <span class="meta">(${hp}/${max} HP · Lvl ${c.level ?? '?'})</span>
                  </div>
                  <span class="meta">${c.playerAddress || ''}</span>
                `;
                charactersList.appendChild(row);
              });
            }

            // Encounters
            encountersList.innerHTML = '';
            if (eList.length === 0) {
              encountersList.innerHTML = '<div class="small-muted">Keine Encounters im State.</div>';
            } else {
              eList.forEach(e => {
                const row = document.createElement('div');
                row.className = 'enc-row';

                const actors = (e.actors || []).map(a => {
                  const id  = a.id || '?';
                  const kind= a.kind === 1 || a.kind === 'Monster' ? 'M' : 'C';
                  const activeIdx = e.turnIndex ?? 0;
                  return { id, kind, active: a === (e.actors || [])[activeIdx] };
                });

                const actorsHtml = actors.map(a =>
                  `<span class="enc-pill ${a.active ? 'active' : ''}">
                     ${a.kind}:${a.id}
                   </span>`
                ).join('');

                row.innerHTML = `
                  <div class="enc-header-line">
                    <span class="id">${e.id || 'encounter'}</span>
                    <span class="meta">
                      Runde ${e.round ?? 1},
                      Turn ${e.turnIndex ?? 0},
                      ${e.active === false ? 'beendet' : 'aktiv'}
                    </span>
                  </div>
                  <div class="enc-actors">
                    ${actorsHtml || '<span class="small-muted">Keine Actors in diesem Encounter.</span>'}
                  </div>
                `;
                encountersList.appendChild(row);
              });
            }

            const now = new Date();
            lastUpdated.textContent = 'Letzte Aktualisierung: ' + now.toLocaleTimeString();

          } catch (e) {
            dndText.textContent = 'DnD-State: Fehler oder /dnd/state nicht erreichbar';
            setDot(dndDot, 'danger');
          }
        }

        async function loadPeers() {
          try {
            const res = await fetch('/chain/peers');
            if (!res.ok) throw new Error('peers failed');
            const j = await res.json();
            const count = (j.peers && j.peers.length) || 0;

            peersText.textContent = 'Peers: ' + count;
            setDot(peersDot, count > 0 ? null : 'warn');
          } catch {
            peersText.textContent = 'Peers: unbekannt';
            setDot(peersDot, 'warn');
          }
        }

        async function refreshAll() {
          await Promise.all([
            loadChain(),
            loadDnd(),
            loadPeers()
          ]);
        }

        document.getElementById('btn-refresh').addEventListener('click', () => {
          refreshAll();
        });

        const btnAuto = document.getElementById('btn-toggle-auto');
        btnAuto.addEventListener('click', () => {
          autoRefresh = !autoRefresh;
          btnAuto.textContent = '● Auto-Refresh: ' + (autoRefresh ? 'AN' : 'AUS');
          if (autoRefresh) {
            autoTimer = setInterval(refreshAll, 3000);
          } else if (autoTimer) {
            clearInterval(autoTimer);
            autoTimer = null;
          }
        });

        // initial load
        refreshAll();
        autoTimer = setInterval(refreshAll, 3000);
      </script>
    </body>
    </html>
    )HTML";

        res.set_content(html, "text/html");
    });


    // ========== START SERVER ==========
    std::cout << "[DashboardServer] Listening on 0.0.0.0:" << port << "\n";
    server.listen("0.0.0.0", port);
}

