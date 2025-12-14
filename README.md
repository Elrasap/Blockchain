# 🧙‍♂️ DnD Blockchain Node

A blockchain-powered combat tracker for **Dungeons & Dragons** featuring:

* A lightweight blockchain
* DM & Player Web UI
* Real-time combat state
* Secure Ed25519-signed DnD actions
* Script-free setup & easy testing

---

## 📌 Features

### 🔥 Blockchain Core

* Custom PoA blockchain implementation
* Persistent block store (SQLite or FS depending on config)
* Mempool with validation & transaction propagation
* Ed25519 cryptography (libsodium)
* Automatic block mining on transaction arrival

### 🛡️ DnD Module

* Characters & monsters
* Encounters (start, end, initiative, hits, damage, skill checks)
* Persistent state across node restarts
* DM-only actions using private key
* Conflict-safe event application
* Fully encoded DnD transactions (`0xD1` magic format)

### 🌐 Web Interface

Served directly from the node — nothing else required.

#### **DM UI**

[http://localhost:8080/ui/dm](http://localhost:8080/ui/dm)
You can:

* Create characters
* Spawn monsters
* Start/end encounters
* Apply hits, damage, skill checks
* View a real-time global state
* View full event logs

#### **Player UI**

[http://localhost:8080/ui/player?id=](http://localhost:8080/ui/player?id=)<characterName>
Players can:

* Roll initiative
* Attack
* Deal damage
* Perform skill checks
* View their HP and logs

---

## 📦 Requirements

* **C++20 compiler** (GCC, Clang)
* **CMake ≥ 3.16**
* **OpenSSL**
* **libsodium**
* **SQLite3**
* **libcurl**

Install on Arch Linux:

```bash
sudo pacman -S cmake gcc make openssl sqlite libsodium curl
```

---

## 🚀 Build Instructions

```bash
mkdir build
cd build
cmake ..
make -j
```

You will get:

```
./blockchain_node
```

---

## 🔧 Configuration

Your `config.json` should contain DM keys:

```json
{
  "port": 8080,
  "gossipPort": 8090,
  "peerPort": 9000,

  "dmPrivKey": [ ... 64 bytes ... ],
  "dmPubKey":  [ ... 32 bytes ... ]
}
```

Generate keys with libsodium:

```cpp
unsigned char pk[32], sk[64];
crypto_sign_keypair(pk, sk);
```

---

## ▶️ Running the Node

```bash
./blockchain_node
```

Output:

```
=== DND BLOCKCHAIN NODE STARTING ===
[HTTP] Starting server on port 8080...
```

Now open your browser:

### DM UI

👉 [http://localhost:8080/ui/dm](http://localhost:8080/ui/dm)

### Player UI

👉 [http://localhost:8080/ui/player?id=hero](http://localhost:8080/ui/player?id=hero)

---

## 🧪 Testing the API (curl)

### Ping

```bash
curl http://localhost:8080/ping
```

### Create character

```bash
curl -X POST http://localhost:8080/dnd/createCharacter \
     -H "Content-Type: application/json" \
     -d '{"encounterId":"t1","actorId":"hero"}'
```

### Spawn monster

```bash
curl -X POST http://localhost:8080/dnd/spawnMonster \
     -H "Content-Type: application/json" \
     -d '{"encounterId":"t1","actorId":"goblin","actorType":1}'
```

### Hit

```bash
curl -X POST http://localhost:8080/dnd/hit \
     -H "Content-Type: application/json" \
     -d '{"encounterId":"t1","actorId":"hero","targetId":"goblin","hit":true,"damage":5}'
```

### Get full state

```bash
curl http://localhost:8080/dnd/state | jq
```

### Encounter history

```bash
curl http://localhost:8080/dnd/history/t1 | jq
```

---

## 🧩 Folder Structure

```
include/
    core/        Blockchain, mempool, crypto
    dnd/         DnD event structs, codec, state
    web/         HTTP APIs, UI
    thirdparty/  httplib, nlohmann-json

src/
    core/        Block, transaction, miner
    dnd/         Combat logic & state machine
    web/         DnD API + Dashboard UI server
    network/     Gossip & peer manager
    main.cpp     Node startup

ui/              (Not required — UI is embedded in C++)
build/           (Generated)
```

---

## 🧠 Architecture Overview

### 🏛️ Blockchain Layer

* Blocks contain transactions
* Each DnD action is encoded as a custom binary payload
* Miner automatically seals blocks
* State rebuilds from genesis at every boot

### 🎲 DnD Layer

State machine:

```
Encounter
    > Actors (PCs + monsters)
    > Events (initiative, hit, damage, skill check)
```

### 🌐 UI Layer

* No external files — fully embedded HTML/CSS/JS compiled into C++
* httplib serves the UI over `/ui/...`

---

## 🔮 Future Enhancements

Planned features (optional):

* Live WebSocket updates
* Visual encounter map with tokens
* Player authentication
* Equipment & inventory
* Spell automation
* Exportable battle reports

---

## ❤️ Credits

* `cpp-httplib` (Yhirose)
* `nlohmann-json`
* `libsodium` for cryptography

