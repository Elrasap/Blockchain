import axios from "axios";

const api = axios.create({
  baseURL: "/api"
});

export interface BlockSummary {
  height: number;
  timestamp: number;
  hash: string;
  prevHash: string;
  txCount: number;
}

export interface EncounterHistory {
  encounterId: string;
  active: boolean;
  round: number;
  turnIndex: number;
  events: {
    encounterId: string;
    actorId: string;
    targetId: string;
    actorType: number;
    targetType: number;
    roll: number;
    damage: number;
    hit: boolean;
    note: string;
    timestamp: number;
  }[];
}

export async function fetchHeight(): Promise<number> {
  const res = await api.get("/chain/height");
  return res.data.height;
}

export async function fetchLatestBlock(): Promise<BlockSummary> {
  const res = await api.get("/chain/latest");
  return res.data;
}

export async function fetchBlockRange(from: number, to: number): Promise<BlockSummary[]> {
  const res = await api.get("/chain/range", { params: { from, to } });
  return res.data;
}

export async function fetchEncounterHistory(id: string): Promise<EncounterHistory> {
  const res = await api.get(`/dnd/history/${id}`);
  return res.data;
}

export async function fetchPeers(): Promise<any[]> {
  const res = await api.get("/peers");
  return res.data.peers ?? [];
}

