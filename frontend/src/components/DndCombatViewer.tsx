import React, { useEffect, useState } from "react";
import { EncounterHistory, fetchEncounterHistory } from "../lib/api";

export const DndCombatViewer: React.FC = () => {
  const [encounterId, setEncounterId] = useState("enc1");
  const [data, setData] = useState<EncounterHistory | null>(null);
  const [error, setError] = useState<string | null>(null);

  const load = async () => {
    setError(null);
    try {
      const res = await fetchEncounterHistory(encounterId);
      setData(res);
    } catch (e: any) {
      setData(null);
      setError("Encounter not found or API error");
    }
  };

  useEffect(() => {
    load();
    const id = setInterval(load, 3000);
    return () => clearInterval(id);
  }, [encounterId]);

  return (
    <div className="flex flex-col gap-4">
      <div className="flex items-end justify-between gap-4">
        <div>
          <h2 className="text-lg font-semibold">DnD Combat Viewer</h2>
          <p className="text-xs text-slate-400">
            Live-Ansicht der Encounter-History aus der Blockchain.
          </p>
        </div>
        <div className="flex items-center gap-2">
          <input
            className="px-2 py-1 text-sm rounded bg-slate-900 border border-slate-700 font-mono"
            value={encounterId}
            onChange={(e) => setEncounterId(e.target.value)}
          />
          <button
            onClick={load}
            className="px-3 py-1 text-xs rounded bg-emerald-600 hover:bg-emerald-500"
          >
            Reload
          </button>
        </div>
      </div>

      {error && (
        <div className="px-3 py-2 rounded bg-red-900/40 border border-red-700 text-xs">
          {error}
        </div>
      )}

      {data && (
        <div className="grid grid-cols-3 gap-4">
          <div className="col-span-1 p-4 rounded-xl bg-slate-900 border border-slate-800">
            <div className="text-xs uppercase text-slate-400 mb-2">
              Encounter
            </div>
            <div className="text-sm">
              <div>ID: <span className="font-mono">{data.encounterId}</span></div>
              <div>Active: {data.active ? "Yes" : "No"}</div>
              <div>Round: {data.round}</div>
              <div>Turn Index: {data.turnIndex}</div>
            </div>
          </div>

          <div className="col-span-2 p-4 rounded-xl bg-slate-900 border border-slate-800">
            <div className="text-xs uppercase text-slate-400 mb-2">
              Event Log
            </div>
            <div className="max-h-80 overflow-y-auto text-xs font-mono space-y-1">
              {data.events.map((e, idx) => {
                let parsedNote: any = null;
                try {
                  parsedNote = JSON.parse(e.note);
                } catch {}

                return (
                  <div key={idx} className="border-b border-slate-800 pb-1">
                    <div className="flex justify-between">
                      <span>
                        {e.actorId} → {e.targetId} | roll={e.roll}{" "}
                        {e.hit ? "HIT" : "MISS"} dmg={e.damage}
                      </span>
                      <span className="text-slate-500">
                        t={e.timestamp}
                      </span>
                    </div>
                    {parsedNote && (
                      <div className="text-slate-400">
                        {parsedNote.type && (
                          <span className="mr-2">[{parsedNote.type}]</span>
                        )}
                        {parsedNote.spellName && (
                          <span>Spell: {parsedNote.spellName}</span>
                        )}
                        {parsedNote.ability && (
                          <span> Save: {parsedNote.ability} vs {parsedNote.dc}</span>
                        )}
                        {parsedNote.effect && (
                          <span> Status: {parsedNote.effect}</span>
                        )}
                        {parsedNote.itemName && (
                          <span> Loot: {parsedNote.itemName} x{parsedNote.quantity}</span>
                        )}
                      </div>
                    )}
                  </div>
                );
              })}
              {data.events.length === 0 && (
                <div className="text-slate-500">
                  No events yet.
                </div>
              )}
            </div>
          </div>
        </div>
      )}
    </div>
  );
};

