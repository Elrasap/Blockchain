#include "web/dashboardServer.hpp"
#include "web/chainApi.hpp"
#include "web/dndApi.hpp"

#include "thirdparty/httplib.h"

#include <iostream>
#include <filesystem>
#include <string>

// =============================================================
// DashboardServer
// =============================================================

DashboardServer::DashboardServer(int port,
                                 const std::string& reportsDir,
                                 const std::string& binaryPath)
    : port(port), reportsDir(reportsDir), binaryPath(binaryPath)
{
}

void DashboardServer::start() {
    std::cerr << "[DashboardServer] start() disabled. Using attach().\n";
}

// =============================================================
// Embedded UI (DM UI, Player UI, CSS, JS)
// Using SAFE RAW STRING DELIMITERS
// =============================================================
namespace {

std::string dm_html() {
    return R"DMHTML(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8"/>
<title>DM Dashboard</title>
<link rel="stylesheet" href="/ui/style.css"/>
<script defer src="/ui/app.js"></script>
</head>
<body>
<header>
    <h1>Dungeon Master Dashboard</h1>
    <nav>
        <a href="/ui/dm">DM</a>
        <a href="/ui/player?id=hero">Player (hero)</a>
    </nav>
</header>

<main>
    <section class="grid">
        <div class="card">
            <h2>Create Character</h2>
            <label>Name</label>
            <input id="cc_name"/>
            <label>Encounter</label>
            <input id="cc_enc" value="enc1"/>
            <button onclick="createCharacter()">Create</button>
        </div>

        <div class="card">
            <h2>Spawn Monster</h2>
            <label>Name</label>
            <input id="sm_name" value="goblin"/>
            <label>Encounter</label>
            <input id="sm_enc" value="enc1"/>
            <button onclick="spawnMonster()">Spawn</button>
        </div>

        <div class="card">
            <h2>Encounter Control</h2>
            <label>ID</label>
            <input id="enc_id" value="enc1"/>
            <button onclick="startEncounter()">Start Encounter</button>
            <button onclick="endEncounter()">End Encounter</button>
        </div>

        <div class="card">
            <h2>Quick Actions</h2>
            <label>Actor</label>
            <input id="qa_actor" value="hero"/>
            <label>Target</label>
            <input id="qa_target" value="goblin"/>
            <label>Damage</label>
            <input id="qa_damage" type="number" value="3"/>
            <button onclick="dmHit()">Hit</button>
            <button onclick="dmDamage()">Damage</button>
            <button onclick="dmSkillCheck()">Skill Check</button>
        </div>
    </section>

    <section class="split">
        <div class="card">
            <div class="card-header">
                <h2>State</h2>
                <button onclick="loadState()">Reload</button>
            </div>
            <pre id="state"></pre>
        </div>

        <div class="card">
            <div class="card-header">
                <h2>Event Log</h2>
                <button onclick="appendLog('--- mark ---')">Mark</button>
            </div>
            <pre id="log"></pre>
        </div>
    </section>
</main>
</body>
</html>
)DMHTML";
}

// =============================================================

std::string player_html() {
    return R"PLAYERHTML(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8"/>
<title>Player Panel</title>
<link rel="stylesheet" href="/ui/style.css"/>
<script defer src="/ui/app.js"></script>
</head>
<body>
<header>
    <h1>Player Panel</h1>
    <nav><a href="/ui/dm">DM</a></nav>
</header>

<main>

<section class="grid">
    <div class="card">
        <h2>My Character</h2>
        <p>ID: <span id="p_name"></span></p>
        <label>Encounter</label>
        <input id="p_enc" value="enc1"/>
    </div>

    <div class="card">
        <h2>Actions</h2>
        <button onclick="initiative()">Roll Initiative</button>
        <button onclick="hit()">Hit</button>
        <button onclick="damage()">Damage</button>
        <button onclick="skillCheck()">Skill Check</button>
        <label>Target</label>
        <input id="p_target" value="goblin"/>
        <label>Damage</label>
        <input id="p_damage" type="number" value="3"/>
    </div>

    <div class="card">
        <h2>HP</h2>
        <pre id="hp"></pre>
    </div>
</section>

<section>
    <div class="card">
        <h2>My Log</h2>
        <pre id="mylog"></pre>
    </div>
</section>

<script>
(function() {
    const params = new URLSearchParams(window.location.search);
    const actor = params.get("id") || "hero";
    document.getElementById("p_name").textContent = actor;
    window.PLAYER_ID = actor;
    loadState();
})();
</script>

</main>
</body>
</html>
)PLAYERHTML";
}

// =============================================================

std::string style_css() {
    return R"CSS(
body { background:#111; color:#eee; font-family:sans-serif; margin:0; }
header { padding:15px; background:#222; display:flex; justify-content:space-between; }
header a { color:#4af; margin-left:15px; text-decoration:none; }
.grid { display:flex; gap:15px; flex-wrap:wrap; }
.card { background:#222; padding:15px; border-radius:8px; min-width:260px; flex:1; }
button { margin-top:6px; padding:8px; border:none; background:#3c3; border-radius:6px; }
input { width:100%; padding:6px; margin-bottom:8px; }
.split { display:flex; flex-wrap:wrap; gap:15px; margin-top:20px; }
pre { background:#000; padding:10px; border-radius:6px; }
)CSS";
}

// =============================================================
// 🎉 JavaScript
// =============================================================

std::string app_js() {
    return R"JS(
// Utility
async function api(ep, body) {
    const res = await fetch(ep, {
        method:"POST",
        headers:{ "Content-Type":"application/json" },
        body:JSON.stringify(body)
    });
    return res.json();
}

function appendLog(msg){
    const el = document.getElementById("log");
    if(el) el.textContent += msg + "\n";
}

// DM actions
async function createCharacter(){
    const name = cc_name.value;
    const enc  = cc_enc.value;
    appendLog(JSON.stringify(await api("/dnd/createCharacter",{encounterId:enc, actorId:name})));
    loadState();
}

async function spawnMonster(){
    const name=sm_name.value;
    const enc=sm_enc.value;
    appendLog(JSON.stringify(await api("/dnd/spawnMonster",{encounterId:enc, actorId:name, actorType:1})));
    loadState();
}

async function startEncounter(){
    appendLog(JSON.stringify(await api("/dnd/startEncounter",{encounterId:enc_id.value, actorId:"dm"})));
    loadState();
}

async function endEncounter(){
    appendLog(JSON.stringify(await api("/dnd/endEncounter",{encounterId:enc_id.value, actorId:"dm"})));
    loadState();
}

async function dmHit(){
    appendLog(JSON.stringify(await api("/dnd/hit",{
        encounterId:enc_id.value, actorId:qa_actor.value,
        targetId:qa_target.value, hit:true,
        damage:parseInt(qa_damage.value)
    })));
    loadState();
}

async function dmDamage(){
    appendLog(JSON.stringify(await api("/dnd/damage",{
        encounterId:enc_id.value, actorId:qa_actor.value,
        targetId:qa_target.value,
        damage:parseInt(qa_damage.value)
    })));
    loadState();
}

async function dmSkillCheck(){
    appendLog(JSON.stringify(await api("/dnd/skillCheck",{
        encounterId:enc_id.value, actorId:qa_actor.value,
        roll:Math.floor(Math.random()*20)+1
    })));
    loadState();
}

// Player actions
function playerId(){ return window.PLAYER_ID || "hero"; }

async function initiative(){
    await api("/dnd/initiative",{encounterId:p_enc.value, actorId:playerId(), roll:Math.floor(Math.random()*20)+1});
    loadState();
}

async function hit(){
    await api("/dnd/hit",{ encounterId:p_enc.value, actorId:playerId(),
        targetId:p_target.value, hit:true, damage:parseInt(p_damage.value)});
    loadState();
}

async function damage(){
    await api("/dnd/damage",{ encounterId:p_enc.value, actorId:playerId(),
        targetId:p_target.value, damage:parseInt(p_damage.value)});
    loadState();
}

async function skillCheck(){
    await api("/dnd/skillCheck",{ encounterId:p_enc.value, actorId:playerId(),
        roll:Math.floor(Math.random()*20)+1});
    loadState();
}

// State
async function loadState(){
    const st = await (await fetch("/dnd/state")).json();
    const el = document.getElementById("state");
    if(el) el.textContent = JSON.stringify(st,null,2);

    const pid = playerId();
    if(st.characters && st.characters[pid] && hp){
        hp.textContent = "HP: " + st.characters[pid].hp + "/" + st.characters[pid].maxHp;
    }
}
)JS";
}

} // namespace

// =============================================================
// Attach everything to httplib server
// =============================================================

void DashboardServer::attach(httplib::Server& http)
{
    http.Get("/", [&](auto&, auto& res){
        res.set_content("<h1>Blockchain UI</h1><p><a href='/ui/dm'>DM UI</a></p><p><a href='/ui/player?id=hero'>Player UI</a></p>","text/html");
    });

    http.Get("/ui/dm", [&](auto&, auto&res){res.set_content(dm_html(),"text/html");});
    http.Get("/ui/player", [&](auto&, auto&res){res.set_content(player_html(),"text/html");});
    http.Get("/ui/style.css", [&](auto&, auto&res){res.set_content(style_css(),"text/css");});
    http.Get("/ui/app.js", [&](auto&, auto&res){res.set_content(app_js(),"application/javascript");});
}

