import React, { useEffect, useState } from "react";
import { fetchPeers } from "../lib/api";

export const PeerMonitor: React.FC = () => {
  const [peers, setPeers] = useState<any[]>([]);

  useEffect(() => {
    const load = async () => {
      try {
        const p = await fetchPeers();
        setPeers(p);
      } catch {
        setPeers([]);
      }
    };
    load();
    const id = setInterval(load, 5000);
    return () => clearInterval(id);
  }, []);

  return (
    <div className="flex flex-col gap-4">
      <h2 className="text-lg font-semibold">Peer Monitor</h2>
      <div className="p-4 rounded-xl bg-slate-900 border border-slate-800 text-sm">
        <div className="flex justify-between mb-2 text-xs text-slate-400">
          <span>Connected Peers</span>
          <span>{peers.length}</span>
        </div>
        <div className="space-y-1">
          {peers.map((p, i) => (
            <div
              key={i}
              className="flex justify-between border-b border-slate-800 pb-1 last:border-b-0"
            >
              <span className="font-mono">{p.address}</span>
              <span className="text-xs text-slate-400">
                height={p.height} lastSeen={p.lastSeen}
              </span>
            </div>
          ))}
          {peers.length === 0 && (
            <div className="text-xs text-slate-500">
              No peers connected.
            </div>
          )}
        </div>
      </div>
    </div>
  );
};

