import React, { useEffect, useState } from "react";
import { BlockSummary, fetchHeight, fetchBlockRange, fetchLatestBlock } from "../lib/api";

export const BlockExplorer: React.FC = () => {
  const [height, setHeight] = useState<number>(0);
  const [blocks, setBlocks] = useState<BlockSummary[]>([]);
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    const load = async () => {
      setLoading(true);
      try {
        const h = await fetchHeight();
        setHeight(h);

        const from = h > 20 ? h - 20 : 0;
        const list = await fetchBlockRange(from, h);
        setBlocks(list.reverse());
      } finally {
        setLoading(false);
      }
    };

    load();
    const id = setInterval(load, 5000);
    return () => clearInterval(id);
  }, []);

  const [latest, setLatest] = useState<BlockSummary | null>(null);

  useEffect(() => {
    const poll = async () => {
      try {
        const b = await fetchLatestBlock();
        setLatest(b);
      } catch {}
    };
    poll();
    const id = setInterval(poll, 3000);
    return () => clearInterval(id);
  }, []);

  return (
    <div className="flex flex-col gap-4">
      <div className="flex items-center justify-between">
        <h2 className="text-lg font-semibold">Block Explorer</h2>
        <div className="text-sm text-slate-400">
          Chain height: <span className="font-mono">{height}</span>
        </div>
      </div>

      {latest && (
        <div className="p-4 rounded-xl bg-slate-900 border border-slate-800">
          <div className="text-xs uppercase text-slate-400 mb-1">
            Latest Block
          </div>
          <div className="flex flex-col gap-1 text-sm">
            <div>Height: <span className="font-mono">{latest.height}</span></div>
            <div>Hash: <span className="font-mono text-xs break-all">{latest.hash}</span></div>
            <div>TXs: {latest.txCount}</div>
          </div>
        </div>
      )}

      <div className="p-4 rounded-xl bg-slate-900 border border-slate-800">
        <div className="flex items-center justify-between mb-2">
          <div className="text-xs uppercase text-slate-400">
            Recent Blocks
          </div>
          {loading && <div className="text-xs text-slate-500">Loading…</div>}
        </div>

        <div className="max-h-96 overflow-y-auto text-sm">
          {blocks.map((b) => (
            <div
              key={b.hash}
              className="py-2 border-b border-slate-800 last:border-b-0"
            >
              <div className="flex items-center justify-between">
                <div>
                  <div className="font-mono text-xs text-slate-400">
                    #{b.height}
                  </div>
                  <div className="font-mono text-xs break-all">
                    {b.hash}
                  </div>
                </div>
                <div className="text-right text-xs text-slate-400">
                  TXs: {b.txCount}
                </div>
              </div>
            </div>
          ))}
          {blocks.length === 0 && (
            <div className="text-xs text-slate-500">
              No blocks yet.
            </div>
          )}
        </div>
      </div>
    </div>
  );
};

