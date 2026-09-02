# DnD Blockchain Node

A distributed, cryptographically verifiable D&D combat tracker written in
C++20. DM and player actions form an append-only event log. Every node verifies
the same signed transactions and deterministically rebuilds the same combat
state from the chain.

The consensus model is intentionally small and explicit:

- the DM is the trusted Proof-of-Authority block producer;
- players are untrusted transaction producers with Ed25519 keys;
- the network and persisted mempool are untrusted inputs;
- replica nodes independently verify blocks, transactions, permissions and
  state transitions.

This is not a general-purpose cryptocurrency or a Byzantine-fault-tolerant
consensus implementation.

## Core guarantees

- One transaction admission pipeline is used by HTTP, TCP gossip, mempool
  restore, block sync and local mining.
- Transactions and blocks use versioned canonical binary formats. A block
  carries every complete transaction, including signatures.
- TCP messages use a magic value, protocol version, message type and bounded
  payload length. Fragmented and coalesced TCP reads are handled correctly.
- `DndState` is the runtime source of truth. It is a deterministic projection
  of validated blockchain events, not a separately writable database.
- DM-only actions and character ownership are verified against signer public
  keys. The DM can delegate a new character to a player key.
- The DM private key is stored in a dedicated mode-`0600` key file and is never
  accepted from `config.json`.
- Missing-parent blocks are buffered. A reconnecting node requests ancestors
  by hash and applies them in order; conflicting DM-signed blocks are reported
  as equivocation.

## Validation flow

```text
HTTP / TCP / persisted mempool / block sync
                    |
              bounded decode
                    |
        canonical transaction checks
                    |
          Ed25519 signature check
                    |
       D&D semantics and permissions
                    |
         deterministic trial transition
                    |
             mempool or block
                    |
               DndState
```

No input path inserts a transaction directly into the mempool or mutates D&D
state directly. A received block is committed only after all of its
transactions can be applied to a temporary state copy; a failure leaves the
live state unchanged.

## D&D permissions

| Action | Required signer |
|---|---|
| Create/assign character | DM |
| Spawn monster | DM |
| Start/end encounter | DM |
| Act as a monster | DM |
| Act as a character | Character owner or DM |

`CreateCharacter.ownerPubKey` is part of the signed D&D payload. If it is
omitted, the character remains DM-controlled. Remote requests must provide a
complete client signature. Automatic DM signing is restricted to loopback
HTTP requests so the local DM interface does not become a remote signing
oracle. Do not put these unsigned action routes behind a local reverse proxy;
the proxy connection itself would be loopback. Network clients should always
submit signed transactions.

Supported events are character creation, monster spawning, encounter start,
initiative, hit, damage, skill check and encounter end. Invalid events do not
create placeholder entities. Damage is clamped at zero and cannot revive a
defeated target.

## Wire formats

All integers are little-endian and all variable fields are length-prefixed.

```text
Transaction
  magic + version + chain id
  sender public key (32 bytes)
  nonce + fee
  payload length + payload
  signature length + Ed25519 signature (64 bytes)

Block
  magic + version
  signed header length + signed header
  transaction count
  repeated transaction length + complete transaction

TCP message
  magic + protocol version + message type + payload length
  payload
```

Transaction payloads are limited to 64 KiB, blocks/messages to 2 MiB and
blocks to 1,024 transactions. Decoders reject unsupported versions, oversized
lengths, truncation and trailing bytes.

## Build and test

Dependencies: CMake 3.16+, a C++20 compiler, OpenSSL, libsodium, SQLite3,
libcurl and nlohmann-json.

On Ubuntu/Debian:

```bash
sudo apt-get install cmake g++ libssl-dev libsodium-dev \
  libsqlite3-dev libcurl4-openssl-dev nlohmann-json3-dev
```

Build and run the tests:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Address/undefined-behaviour sanitizers:

```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON -DENABLE_SANITIZERS=ON \
  -DSANITIZERS=address,undefined
cmake --build build-asan --parallel
ctest --test-dir build-asan --output-on-failure
```

Use `-DSANITIZERS=thread` for ThreadSanitizer. CI builds the node, runs the
test suite, exercises address/undefined/thread sanitizers and uploads the
verified binary.

The tests cover canonical transaction/block round trips and event signing,
malformed and random decoder inputs, fragmented/coalesced framing, DM/player
authorization, atomic state transitions, key-file permissions, out-of-order
replica catch-up and state reconstruction after restart.

## Configuration and startup

`config.json` is optional. Defaults are suitable for a local node. Copy
[`config.example.json`](config.example.json) when custom ports or peers are
needed.

```json
{
  "port": 8080,
  "gossipPort": 8090,
  "peerPort": 9000,
  "blockDb": "blocks",
  "mempoolFile": "mempool.json",
  "snapshotFile": "state_snapshot.json",
  "dmKeyFile": "keys/dm.key",
  "peers": [
    {"host": "127.0.0.1", "port": 9001}
  ]
}
```

Start the node from the repository root:

```bash
./build/blockchain_node
```

On first start, the node creates `keys/dm.key` with private file permissions.
If an existing key file is malformed, startup fails instead of silently
rotating the authority key and invalidating the stored chain.

Useful endpoints:

- `GET /health`
- `GET /chain/latest`
- `GET /chain/block/<height>`
- `GET /dnd/state`
- `GET /dnd/history/<encounter-id>`
- `POST /gossip/tx` for a complete signed transaction
- `POST /gossip/block` for a canonical binary block

The embedded UI under `/ui/dm` is a local DM demo. Networked player clients
should submit their own canonical Ed25519-signed transactions.

## Repository layout

```text
include/core       transaction, block, validation and chain interfaces
include/dnd        event schema, policy and projected state
include/network    framing, peers and missing-parent synchronization
src                implementations and node startup
src/tests          unit, parser-hardening and convergence tests
ui / www           local demonstration clients
```
