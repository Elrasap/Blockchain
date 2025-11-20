import React, { useEffect, useState } from "react";

// ---------- Types matching your backend JSON ----------

type PingResponse = {
  pong?: boolean;
};

type CharacterSummary = {
  id: string;
  name: string;
  hp: number;
  maxHp: number;
  ac: number;
  level: number;
};

type MonsterSummary = {
  id: string;
  name: string;
  hp: number;
  maxHp: number;
  ac: number;
};

type EncounterSummary = {
  id: string;
  active: boolean;
  round: number;
  turnIndex: number;
};

type DndStateResponse = {
  ok: boolean;
  characters: Record<string, CharacterSummary>;
  monsters: Record<string, MonsterSummary>;
  encounters: Record<string, EncounterSummary>;
};

type EncounterEvent = {
  encounterId: string;
  actorId: string;
  actorType: number;
  targetId: string;
  targetType: number;
  roll: number;
  damage: number;
  hit: boolean;
  note: string;
  timestamp: number;
  eventType?: number;
};

// eventType mapping to backend enum
const EventType = {
  CreateCharacter: 1,
  SpawnMonster: 2,
  StartEncounter: 3,
  Initiative: 4,
  Hit: 5,
  Damage: 6,
  SkillCheck: 7,
  EndEncounter: 8
} as const;

type ApiActionState = {
  loading: boolean;
  error: string | null;
  success: string | null;
};

const apiBase = ""; // same origin; if needed: "http://localhost:8080"

// small helper to wrap fetch+JSON with error handling
async function apiPostJson<T>(
  path: string,
  body: unknown
): Promise<{ data: T | null; error: string | null }> {
  try {
    const res = await fetch(apiBase + path, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(body)
    });

    const txt = await res.text();
    let json: any = null;
    try {
      json = txt ? JSON.parse(txt) : null;
    } catch {
      // ignore parse error
    }

    if (!res.ok) {
      return {
        data: null,
        error:
          (json && json.error) ||
          `HTTP ${res.status} ${res.statusText || ""}`.trim()
      };
    }
    return { data: json as T, error: null };
  } catch (e: any) {
    return { data: null, error: e?.message || "Network error" };
  }
}

async function apiGetJson<T>(
  path: string
): Promise<{ data: T | null; error: string | null }> {
  try {
    const res = await fetch(apiBase + path);
    const txt = await res.text();
    const json = txt ? JSON.parse(txt) : null;

    if (!res.ok) {
      return {
        data: null,
        error: (json && json.error) || `HTTP ${res.status}`
      };
    }
    return { data: json as T, error: null };
  } catch (e: any) {
    return { data: null, error: e?.message || "Network error" };
  }
}

// --------------------------------------------------
// Main App
// --------------------------------------------------

export const App: React.FC = () => {
  const [pingOk, setPingOk] = useState<boolean | null>(null);
  const [pingError, setPingError] = useState<string | null>(null);

  const [state, setState] = useState<DndStateResponse | null>(null);
  const [stateError, setStateError] = useState<string | null>(null);
  const [stateLoading, setStateLoading] = useState(false);

  const [selectedEncounterId, setSelectedEncounterId] = useState<string | null>(
    null
  );
  const [encounterEvents, setEncounterEvents] = useState<EncounterEvent[]>([]);
  const [encEventsError, setEncEventsError] = useState<string | null>(null);
  const [encEventsLoading, setEncEventsLoading] = useState(false);

  const [actionState, setActionState] = useState<ApiActionState>({
    loading: false,
    error: null,
    success: null
  });

  // forms
  const [newCharacterId, setNewCharacterId] = useState("");
  const [newMonsterId, setNewMonsterId] = useState("");
  const [newEncounterId, setNewEncounterId] = useState("enc1");

  const [initiativeActorId, setInitiativeActorId] = useState("");
  const [initiativeEncounterId, setInitiativeEncounterId] = useState("enc1");
  const [initiativeRoll, setInitiativeRoll] = useState(10);

  const [hitEncounterId, setHitEncounterId] = useState("enc1");
  const [hitActorId, setHitActorId] = useState("");
  const [hitTargetId, setHitTargetId] = useState("");
  const [hitActorType, setHitActorType] = useState<0 | 1>(0);
  const [hitTargetType, setHitTargetType] = useState<0 | 1>(1);
  const [hitRoll, setHitRoll] = useState(12);
  const [hitIsHit, setHitIsHit] = useState(true);

  const [damageEncounterId, setDamageEncounterId] = useState("enc1");
  const [damageTargetId, setDamageTargetId] = useState("");
  const [damageTargetType, setDamageTargetType] = useState<0 | 1>(1);
  const [damageValue, setDamageValue] = useState(5);

  // --------- initial node ping + state load ----------
  useEffect(() => {
    (async () => {
      const { data, error } = await apiGetJson<PingResponse>("/ping");
      if (error || !data || !data.pong) {
        setPingOk(false);
        setPingError(error || "pong=false");
      } else {
        setPingOk(true);
        setPingError(null);
      }
    })();
  }, []);

  const refreshState = async () => {
    setStateLoading(true);
    setStateError(null);
    const { data, error } = await apiGetJson<DndStateResponse>("/dnd/state");
    setStateLoading(false);
    if (error || !data) {
      setStateError(error || "no data");
      setState(null);
      return;
    }
    setState(data);

    // choose a default encounter if none selected
    const ids = Object.keys(data.encounters);
    if (!selectedEncounterId && ids.length > 0) {
      setSelectedEncounterId(ids[0]);
    }
  };

  useEffect(() => {
    refreshState();
    const id = setInterval(refreshState, 4000); // poll every 4s
    return () => clearInterval(id);
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  // --------- load events for selected encounter ----------
  useEffect(() => {
    if (!selectedEncounterId) {
      setEncounterEvents([]);
      setEncEventsError(null);
      return;
    }
    (async () => {
      setEncEventsLoading(true);
      setEncEventsError(null);
      const { data, error } = await apiGetJson<{
        ok: boolean;
        events: EncounterEvent[];
      }>(`/dnd/history/${encodeURIComponent(selectedEncounterId)}`);
      setEncEventsLoading(false);
      if (error || !data || !data.ok) {
        setEncEventsError(error || "failed to load events");
        setEncounterEvents([]);
        return;
      }
      setEncounterEvents(data.events);
    })();
  }, [selectedEncounterId]);

  // --------------------------------------------------
  // helpers
  // --------------------------------------------------

  const setActionLoading = () =>
    setActionState({ loading: true, error: null, success: null });
  const setActionError = (msg: string) =>
    setActionState({ loading: false, error: msg, success: null });
  const setActionSuccess = (msg: string) =>
    setActionState({ loading: false, error: null, success: msg });

  // ----- Actions -----

  const handleCreateCharacter = async () => {
    if (!newCharacterId.trim()) {
      setActionError("Character id required");
      return;
    }
    setActionLoading();
    const body = {
      encounterId: newEncounterId || "enc-out-of-combat",
      actorId: newCharacterId.trim(),
      actorType: 0,
      eventType: EventType.CreateCharacter,
      targetType: 0,
      targetId: "",
      roll: 0,
      damage: 0,
      hit: false,
      note: "",
      timestamp: Math.floor(Date.now() / 1000)
    };
    const { error } = await apiPostJson("/dnd/createCharacter", body);
    if (error) {
      setActionError(error);
    } else {
      setActionSuccess(`Character '${newCharacterId}' created`);
      setNewCharacterId("");
      refreshState();
    }
  };

  const handleSpawnMonster = async () => {
    if (!newMonsterId.trim()) {
      setActionError("Monster id required");
      return;
    }
    setActionLoading();
    const body = {
      encounterId: newEncounterId || "enc1",
      actorId: newMonsterId.trim(),
      actorType: 1,
      eventType: EventType.SpawnMonster,
      targetType: 0,
      targetId: "",
      roll: 0,
      damage: 0,
      hit: false,
      note: "",
      timestamp: Math.floor(Date.now() / 1000)
    };
    const { error } = await apiPostJson("/dnd/spawnMonster", body);
    if (error) {
      setActionError(error);
    } else {
      setActionSuccess(`Monster '${newMonsterId}' spawned`);
      setNewMonsterId("");
      refreshState();
    }
  };

  const handleStartEncounter = async () => {
    if (!newEncounterId.trim()) {
      setActionError("Encounter id required");
      return;
    }
    setActionLoading();
    const body = {
      encounterId: newEncounterId.trim(),
      actorId: "",
      actorType: 0,
      eventType: EventType.StartEncounter,
      targetId: "",
      targetType: 0,
      roll: 0,
      damage: 0,
      hit: false,
      note: "",
      timestamp: Math.floor(Date.now() / 1000)
    };
    const { error } = await apiPostJson("/dnd/startEncounter", body);
    if (error) {
      setActionError(error);
    } else {
      setActionSuccess(`Encounter '${newEncounterId}' started`);
      setSelectedEncounterId(newEncounterId.trim());
      refreshState();
    }
  };

  const handleInitiative = async () => {
    if (!initiativeActorId.trim()) {
      setActionError("Actor id required for initiative");
      return;
    }
    setActionLoading();
    const body = {
      encounterId: initiativeEncounterId || "enc1",
      actorId: initiativeActorId.trim(),
      actorType: 0,
      eventType: EventType.Initiative,
      targetId: "",
      targetType: 0,
      roll: initiativeRoll,
      damage: 0,
      hit: false,
      note: "initiative",
      timestamp: Math.floor(Date.now() / 1000)
    };
    const { error } = await apiPostJson("/dnd/initiative", body);
    if (error) {
      setActionError(error);
    } else {
      setActionSuccess(
        `Initiative for '${initiativeActorId}' = ${initiativeRoll}`
      );
      setInitiativeActorId("");
      refreshState();
    }
  };

  const handleHit = async () => {
    if (!hitActorId.trim() || !hitTargetId.trim()) {
      setActionError("actorId and targetId required");
      return;
    }
    setActionLoading();
    const body = {
      encounterId: hitEncounterId || "enc1",
      actorId: hitActorId.trim(),
      actorType: hitActorType,
      targetId: hitTargetId.trim(),
      targetType: hitTargetType,
      eventType: EventType.Hit,
      roll: hitRoll,
      damage: 0,
      hit: hitIsHit,
      note: "attack roll",
      timestamp: Math.floor(Date.now() / 1000)
    };
    const { error } = await apiPostJson("/dnd/hit", body);
    if (error) {
      setActionError(error);
    } else {
      setActionSuccess(
        `Hit event from '${hitActorId}' to '${hitTargetId}' (roll ${hitRoll})`
      );
      refreshState();
    }
  };

  const handleDamage = async () => {
    if (!damageTargetId.trim()) {
      setActionError("targetId required for damage");
      return;
    }
    setActionLoading();
    const body = {
      encounterId: damageEncounterId || "enc1",
      actorId: "", // could be filled, but not required für unsere Regeln
      actorType: 0,
      targetId: damageTargetId.trim(),
      targetType: damageTargetType,
      eventType: EventType.Damage,
      roll: 0,
      damage: damageValue,
      hit: true,
      note: "damage",
      timestamp: Math.floor(Date.now() / 1000)
    };
    const { error } = await apiPostJson("/dnd/damage", body);
    if (error) {
      setActionError(error);
    } else {
      setActionSuccess(
        `Damage ${damageValue} on '${damageTargetId}' (${damageTargetType === 0 ? "char" : "monster"})`
      );
      refreshState();
    }
  };

  const handleEndEncounter = async () => {
    const id = selectedEncounterId || newEncounterId || "enc1";
    setActionLoading();
    const body = {
      encounterId: id,
      actorId: "",
      actorType: 0,
      eventType: EventType.EndEncounter,
      targetId: "",
      targetType: 0,
      roll: 0,
      damage: 0,
      hit: false,
      note: "end encounter",
      timestamp: Math.floor(Date.now() / 1000)
    };
    const { error } = await apiPostJson("/dnd/endEncounter", body);
    if (error) {
      setActionError(error);
    } else {
      setActionSuccess(`Encounter '${id}' ended`);
      refreshState();
    }
  };

  // --------------------------------------------------
  // Render helpers
  // --------------------------------------------------

  const renderNodeStatus = () => (
    <div className="card">
      <div className="card-header">
        <h2>Node</h2>
      </div>
      <div className="card-body">
        <div className="badge-row">
          <span
            className={
              "status-dot " +
              (pingOk === null
                ? "status-gray"
                : pingOk
                ? "status-green"
                : "status-red")
            }
          />
          <span>
            {pingOk === null
              ? "Checking /ping..."
              : pingOk
              ? "Online (pong=true)"
              : `Offline: ${pingError || "ping failed"}`}
          </span>
        </div>
        <button className="btn secondary" onClick={refreshState}>
          Refresh DnD State
        </button>
        {stateLoading && <div className="hint">Loading state...</div>}
        {stateError && <div className="error-text">{stateError}</div>}
      </div>
    </div>
  );

  const renderCharacters = () => {
    const chars = state ? Object.values(state.characters) : [];
    return (
      <div className="card">
        <div className="card-header">
          <h2>Characters</h2>
          <span className="pill">{chars.length}</span>
        </div>
        <div className="card-body list">
          {chars.length === 0 && <div className="hint">No characters yet.</div>}
          {chars.map((c) => (
            <div key={c.id} className="list-row">
              <div className="list-main">
                <div className="list-title">{c.name}</div>
                <div className="list-sub">
                  HP {c.hp}/{c.maxHp} • AC {c.ac} • Lvl {c.level}
                </div>
              </div>
              <div className="list-tag">char</div>
            </div>
          ))}
        </div>
      </div>
    );
  };

  const renderMonsters = () => {
    const mons = state ? Object.values(state.monsters) : [];
    return (
      <div className="card">
        <div className="card-header">
          <h2>Monsters</h2>
          <span className="pill">{mons.length}</span>
        </div>
        <div className="card-body list">
          {mons.length === 0 && <div className="hint">No monsters yet.</div>}
          {mons.map((m) => (
            <div key={m.id} className="list-row">
              <div className="list-main">
                <div className="list-title">{m.name}</div>
                <div className="list-sub">
                  HP {m.hp}/{m.maxHp} • AC {m.ac}
                </div>
              </div>
              <div className="list-tag danger">mob</div>
            </div>
          ))}
        </div>
      </div>
    );
  };

  const renderEncounters = () => {
    const encs = state ? Object.values(state.encounters) : [];
    return (
      <div className="card">
        <div className="card-header">
          <h2>Encounters</h2>
          <span className="pill">{encs.length}</span>
        </div>
        <div className="card-body list">
          {encs.length === 0 && (
            <div className="hint">No encounters. Start one below.</div>
          )}
          {encs.map((e) => {
            const isSel = selectedEncounterId === e.id;
            return (
              <div
                key={e.id}
                className={
                  "list-row clickable " + (isSel ? "list-row-selected" : "")
                }
                onClick={() => setSelectedEncounterId(e.id)}
              >
                <div className="list-main">
                  <div className="list-title">{e.id}</div>
                  <div className="list-sub">
                    {e.active ? "active" : "ended"} • round {e.round} • turn{" "}
                    {e.turnIndex}
                  </div>
                </div>
                <div className={"list-tag " + (e.active ? "success" : "muted")}>
                  {e.active ? "active" : "done"}
                </div>
              </div>
            );
          })}
        </div>
      </div>
    );
  };

  const renderEncounterEvents = () => (
    <div className="card">
      <div className="card-header">
        <h2>
          Events{" "}
          {selectedEncounterId ? `(${selectedEncounterId})` : "(no encounter)"}
        </h2>
      </div>
      <div className="card-body list events">
        {encEventsLoading && <div className="hint">Loading events…</div>}
        {encEventsError && <div className="error-text">{encEventsError}</div>}
        {!encEventsLoading &&
          !encEventsError &&
          encounterEvents.length === 0 && (
            <div className="hint">No events yet.</div>
          )}
        {encounterEvents
          .slice()
          .reverse()
          .map((e, idx) => (
            <div key={idx} className="event-row">
              <div className="event-main">
                <div className="event-title">
                  {e.actorId || "DM"} → {e.targetId || "—"}
                </div>
                <div className="event-sub">
                  roll {e.roll} • dmg {e.damage} •{" "}
                  {e.hit ? "hit" : "no hit"} • {e.note || "no note"}
                </div>
              </div>
              <div className="event-meta">
                <span className="event-time">
                  {new Date(e.timestamp * 1000).toLocaleTimeString()}
                </span>
              </div>
            </div>
          ))}
      </div>
    </div>
  );

  const renderActionsLeft = () => (
    <div className="card">
      <div className="card-header">
        <h2>Setup</h2>
      </div>
      <div className="card-body form-grid">
        <div className="field">
          <label>Encounter Id</label>
          <input
            value={newEncounterId}
            onChange={(e) => setNewEncounterId(e.target.value)}
            placeholder="enc1"
          />
          <button
            className="btn"
            disabled={actionState.loading}
            onClick={handleStartEncounter}
          >
            Start Encounter
          </button>
        </div>

        <div className="field">
          <label>New Character Id</label>
          <input
            value={newCharacterId}
            onChange={(e) => setNewCharacterId(e.target.value)}
            placeholder="hero"
          />
          <button
            className="btn"
            disabled={actionState.loading}
            onClick={handleCreateCharacter}
          >
            Create Character
          </button>
        </div>

        <div className="field">
          <label>New Monster Id</label>
          <input
            value={newMonsterId}
            onChange={(e) => setNewMonsterId(e.target.value)}
            placeholder="goblin-1"
          />
          <button
            className="btn danger"
            disabled={actionState.loading}
            onClick={handleSpawnMonster}
          >
            Spawn Monster
          </button>
        </div>
      </div>
    </div>
  );

  const renderActionsRight = () => (
    <div className="card">
      <div className="card-header">
        <h2>Combat</h2>
      </div>
      <div className="card-body form-grid">
        <div className="field">
          <label>Initiative</label>
          <div className="field-row">
            <input
              value={initiativeEncounterId}
              onChange={(e) => setInitiativeEncounterId(e.target.value)}
              placeholder="enc1"
            />
            <input
              value={initiativeActorId}
              onChange={(e) => setInitiativeActorId(e.target.value)}
              placeholder="actor id"
            />
            <input
              type="number"
              value={initiativeRoll}
              onChange={(e) => setInitiativeRoll(Number(e.target.value))}
            />
          </div>
          <button
            className="btn secondary"
            disabled={actionState.loading}
            onClick={handleInitiative}
          >
            Add Initiative
          </button>
        </div>

        <div className="field">
          <label>Hit</label>
          <div className="field-row">
            <input
              value={hitEncounterId}
              onChange={(e) => setHitEncounterId(e.target.value)}
              placeholder="enc1"
            />
            <input
              value={hitActorId}
              onChange={(e) => setHitActorId(e.target.value)}
              placeholder="attacker id"
            />
            <input
              value={hitTargetId}
              onChange={(e) => setHitTargetId(e.target.value)}
              placeholder="target id"
            />
          </div>
          <div className="field-row">
            <select
              value={hitActorType}
              onChange={(e) =>
                setHitActorType(Number(e.target.value) as 0 | 1)
              }
            >
              <option value={0}>Actor: Character</option>
              <option value={1}>Actor: Monster</option>
            </select>
            <select
              value={hitTargetType}
              onChange={(e) =>
                setHitTargetType(Number(e.target.value) as 0 | 1)
              }
            >
              <option value={0}>Target: Character</option>
              <option value={1}>Target: Monster</option>
            </select>
            <input
              type="number"
              value={hitRoll}
              onChange={(e) => setHitRoll(Number(e.target.value))}
            />
            <label className="checkbox">
              <input
                type="checkbox"
                checked={hitIsHit}
                onChange={(e) => setHitIsHit(e.target.checked)}
              />
              hit
            </label>
          </div>
          <button
            className="btn"
            disabled={actionState.loading}
            onClick={handleHit}
          >
            Add Hit Event
          </button>
        </div>

        <div className="field">
          <label>Damage</label>
          <div className="field-row">
            <input
              value={damageEncounterId}
              onChange={(e) => setDamageEncounterId(e.target.value)}
              placeholder="enc1"
            />
            <input
              value={damageTargetId}
              onChange={(e) => setDamageTargetId(e.target.value)}
              placeholder="target id"
            />
            <input
              type="number"
              value={damageValue}
              onChange={(e) => setDamageValue(Number(e.target.value))}
            />
          </div>
          <div className="field-row">
            <select
              value={damageTargetType}
              onChange={(e) =>
                setDamageTargetType(Number(e.target.value) as 0 | 1)
              }
            >
              <option value={0}>Character</option>
              <option value={1}>Monster</option>
            </select>
          </div>
          <button
            className="btn danger"
            disabled={actionState.loading}
            onClick={handleDamage}
          >
            Apply Damage
          </button>
        </div>

        <div className="field">
          <label>Encounter</label>
          <button
            className="btn ghost"
            disabled={actionState.loading}
            onClick={handleEndEncounter}
          >
            End Current Encounter
          </button>
        </div>

        {actionState.error && (
          <div className="error-text">{actionState.error}</div>
        )}
        {actionState.success && (
          <div className="success-text">{actionState.success}</div>
        )}
      </div>
    </div>
  );

  return (
    <div className="app-root">
      <header className="topbar">
        <div>
          <h1>DnD Chain Dashboard</h1>
          <p className="subtitle">
            On-chain encounter tracker – assists your real game, doesn&apos;t
            replace it.
          </p>
        </div>
        <div className="topbar-right">
          <span className="tag">Local Node</span>
          <span className="tag">http://localhost:8080</span>
        </div>
      </header>

      <main className="layout">
        <section className="column">
          {renderNodeStatus()}
          {renderCharacters()}
          {renderMonsters()}
        </section>

        <section className="column">
          {renderEncounters()}
          {renderEncounterEvents()}
        </section>

        <section className="column">
          {renderActionsLeft()}
          {renderActionsRight()}
        </section>
      </main>

      {/* Inline CSS to keep everything in one file */}
      <style>{`
        .app-root {
          min-height: 100vh;
          padding: 16px 24px 32px;
          background: radial-gradient(circle at top, #111827, #020617 60%);
          color: #e5e7eb;
        }

        .topbar {
          display: flex;
          justify-content: space-between;
          align-items: flex-end;
          margin-bottom: 16px;
        }
        .topbar h1 {
          margin: 0;
          font-size: 24px;
        }
        .subtitle {
          margin: 2px 0 0;
          color: #9ca3af;
          font-size: 13px;
        }
        .topbar-right {
          display: flex;
          gap: 8px;
        }
        .tag {
          padding: 4px 8px;
          border-radius: 999px;
          background: #111827;
          border: 1px solid #1f2937;
          font-size: 11px;
          color: #9ca3af;
        }

        .layout {
          display: grid;
          grid-template-columns: 1fr 1.1fr 1.2fr;
          gap: 16px;
        }
        @media (max-width: 1000px) {
          .layout {
            grid-template-columns: 1fr;
          }
        }

        .column {
          display: flex;
          flex-direction: column;
          gap: 12px;
        }

        .card {
          background: #020617;
          border-radius: 12px;
          border: 1px solid #1f2937;
          box-shadow: 0 12px 30px rgba(0,0,0,0.4);
          overflow: hidden;
        }
        .card-header {
          padding: 10px 12px;
          border-bottom: 1px solid #1f2937;
          display: flex;
          align-items: center;
          justify-content: space-between;
        }
        .card-header h2 {
          margin: 0;
          font-size: 15px;
          letter-spacing: 0.02em;
        }
        .card-body {
          padding: 10px 12px 12px;
          font-size: 13px;
        }

        .badge-row {
          display: flex;
          align-items: center;
          gap: 8px;
          margin-bottom: 8px;
        }
        .status-dot {
          width: 10px;
          height: 10px;
          border-radius: 999px;
        }
        .status-green { background: #22c55e; }
        .status-red { background: #ef4444; }
        .status-gray { background: #6b7280; }

        .btn {
          border: none;
          border-radius: 999px;
          padding: 6px 12px;
          background: linear-gradient(to right, #3b82f6, #6366f1);
          color: white;
          font-size: 12px;
          cursor: pointer;
          margin-top: 4px;
        }
        .btn.secondary {
          background: #111827;
          border: 1px solid #374151;
        }
        .btn.danger {
          background: linear-gradient(to right, #ef4444, #f97316);
        }
        .btn.ghost {
          background: transparent;
          border: 1px solid #4b5563;
        }
        .btn:disabled {
          opacity: 0.5;
          cursor: default;
        }

        .pill {
          padding: 2px 8px;
          border-radius: 999px;
          background: #0f172a;
          border: 1px solid #1f2937;
          font-size: 11px;
          color: #9ca3af;
        }

        .list {
          max-height: 280px;
          overflow: auto;
        }
        .list-row {
          display: flex;
          align-items: center;
          justify-content: space-between;
          padding: 6px 4px;
          border-radius: 8px;
        }
        .list-row + .list-row {
          margin-top: 2px;
        }
        .list-row-selected {
          background: rgba(59,130,246,0.16);
        }
        .list-row.clickable {
          cursor: pointer;
        }
        .list-row.clickable:hover {
          background: rgba(55,65,81,0.5);
        }
        .list-main {
          display: flex;
          flex-direction: column;
        }
        .list-title {
          font-size: 13px;
        }
        .list-sub {
          font-size: 11px;
          color: #9ca3af;
        }
        .list-tag {
          font-size: 10px;
          padding: 2px 6px;
          border-radius: 999px;
          border: 1px solid #374151;
          color: #9ca3af;
        }
        .list-tag.danger {
          border-color: #b91c1c;
          color: #fca5a5;
        }
        .list-tag.success {
          border-color: #16a34a;
          color: #4ade80;
        }
        .list-tag.muted {
          color: #6b7280;
        }

        .events {
          max-height: 320px;
        }
        .event-row {
          display: flex;
          justify-content: space-between;
          padding: 6px 4px;
          border-radius: 8px;
        }
        .event-row + .event-row {
          margin-top: 2px;
        }
        .event-row:hover {
          background: rgba(31,41,55,0.7);
        }
        .event-title {
          font-size: 13px;
        }
        .event-sub {
          font-size: 11px;
          color: #9ca3af;
        }
        .event-meta {
          font-size: 11px;
          color: #6b7280;
          margin-left: 8px;
        }

        .form-grid {
          display: flex;
          flex-direction: column;
          gap: 10px;
        }
        .field {
          display: flex;
          flex-direction: column;
          gap: 4px;
        }
        .field label {
          font-size: 11px;
          color: #9ca3af;
        }
        .field-row {
          display: flex;
          gap: 6px;
          flex-wrap: wrap;
        }
        input, select {
          background: #020617;
          border-radius: 8px;
          border: 1px solid #1f2937;
          padding: 5px 8px;
          font-size: 12px;
          color: #e5e7eb;
          min-width: 0;
        }
        input[type="number"] {
          width: 70px;
        }
        .checkbox {
          display: flex;
          align-items: center;
          gap: 4px;
          font-size: 11px;
        }

        .hint {
          font-size: 12px;
          color: #9ca3af;
          margin-top: 4px;
        }
        .error-text {
          margin-top: 4px;
          font-size: 12px;
          color: #fecaca;
        }
        .success-text {
          margin-top: 4px;
          font-size: 12px;
          color: #bbf7d0;
        }
      `}</style>
    </div>
  );
};

