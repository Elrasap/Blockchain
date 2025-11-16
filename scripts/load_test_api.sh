#!/usr/bin/env bash
set -euo pipefail

HOST="${1:-localhost}"
PORT="${2:-8080}"

BASE="http://$HOST:$PORT"

echo "[LOAD] Hitting $BASE ..."

# 1) Warmup
curl -s "$BASE/health" >/dev/null || true

# 2) Viele DnD-TX vorschlagen
for i in $(seq 1 200); do
  echo -n "."
  curl -s -X POST "$BASE/dnd/api/tx/propose" \
    -H "Content-Type: application/json" \
    -d "{
      \"encounterId\": \"loadtest-enc\",
      \"actorId\": \"hero-1\",
      \"targetId\": \"goblin-$i\",
      \"actorType\": 0,
      \"targetType\": 1,
      \"roll\": 15,
      \"damage\": 3,
      \"hit\": true,
      \"note\": \"loadtest\",
      \"timestamp\": $(date +%s),
      \"senderPubKeyHex\": \"00\",
      \"signatureHex\": \"00\"
    }" >/dev/null || true
done

echo
echo "[LOAD] Proposing blocks..."

# 3) DM mined ein paar Blöcke
for i in $(seq 1 20); do
  curl -s -X POST "$BASE/dnd/api/block/mine?dmToken=dev" >/dev/null || true
done

echo "[LOAD] Done."

