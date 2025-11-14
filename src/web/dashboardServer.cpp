 #include "web/dashboardServer.hpp"

#include "ops/reliabilityGuard.hpp"
#include "analytics/rtoRpoAnalyzer.hpp"
#include "analytics/trendAnalyzer.hpp"

#include "dnd/dndCharacterService.hpp"
#include "dnd/monster.hpp"
#include "dnd/encounter.hpp"

#include "core/mempool.hpp"
#include "core/transaction.hpp"

#include <thirdparty/httplib.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

// Global Mempool
static Mempool mempool;


DashboardServer::DashboardServer(int p, const std::string& dir, const std::string& bin)
    : port(p), reportsDir(dir), binaryPath(bin) {}


// ------------------------------------------------------------
// Helper: RAW transaction log
// ------------------------------------------------------------
void DashboardServer::logDnDTx(const std::string& j)
{
    try {
        std::ofstream out("dnd_tx_pool.log", std::ios::app);
        out << j << "\n";
    } catch(...) {}
}


// ------------------------------------------------------------
// START SERVER
// ------------------------------------------------------------
void DashboardServer::start() {
    httplib::Server svr;

    // ---- State Services ----
    dnd::DndCharacterService dndService("characters.db");
    dndService.load();

    dnd::MonsterService monsterService("monsters.json");
    monsterService.load();

    dnd::EncounterService encounterService;


    // ------------------------------------------------------------
    // STATUS
    // ------------------------------------------------------------
    svr.Get("/api/status", [&](const httplib::Request&, httplib::Response& res){
        ReliabilityGuard guard(reportsDir, binaryPath);
        auto s = guard.evaluate(8000.0, 95.0, 1);

        json j;
        j["integrity"]   = s.integrityOk;
        j["performance"] = s.perfOk;
        j["chaos"]       = s.chaosOk;
        j["forecast"]    = s.forecastOk;
        j["avg_rto_ms"]  = s.avgRto;
        j["pass_rate"]   = s.passRate;
        j["anomalies"]   = s.anomalies;

        res.set_content(j.dump(2), "application/json");
    });


    // ------------------------------------------------------------
    // RTO DATA
    // ------------------------------------------------------------
    svr.Get("/api/rto", [&](const httplib::Request&, httplib::Response& res){
        RtoRpoAnalyzer a(reportsDir);
        auto runs = a.analyzeAll();

        json arr = json::array();
        for (auto& r : runs) {
            json j;
            j["file"]       = r.filename;
            j["rto_ms"]     = r.rto_ms;
            j["restore_ms"] = r.restore_ms;
            j["passed"]     = r.passed;
            arr.push_back(j);
        }
        res.set_content(arr.dump(2), "application/json");
    });


    // ------------------------------------------------------------
    // TREND DATA
    // ------------------------------------------------------------
    svr.Get("/api/trend", [&](const httplib::Request&, httplib::Response& res){
        TrendAnalyzer t("history.db");
        auto data    = t.loadDaily();
        auto summary = t.computeSummary(data);

        json j;
        j["mean_rto_ms"]   = summary.meanRto;
        j["median_rto_ms"] = summary.medianRto;
        j["slope"]         = summary.slope;
        j["regressions"]   = summary.regressions;

        j["data"] = json::array();
        for (auto& e : data) {
            json d;
            d["date"]      = e.date;
            d["avg_rto_ms"]= e.avgRto;
            d["pass_rate"] = e.passRate;
            d["anomalies"] = e.anomalies;
            j["data"].push_back(d);
        }

        res.set_content(j.dump(2),"application/json");
    });


    // ------------------------------------------------------------
    // ALERT LOG
    // ------------------------------------------------------------
    svr.Get("/api/alerts", [&](const httplib::Request&, httplib::Response& res){
        std::ifstream in("alerts.log");
        if (!in) { res.status = 404; return; }

        json arr = json::array();
        std::string line;
        while (std::getline(in, line))
            arr.push_back(line);

        res.set_content(arr.dump(2), "application/json");
    });




    // ========================================================================
    // ██████╗ ███╗   ██╗██████╗     ██████╗███╗   ██╗██████╗
    // ██╔══██╗████╗  ██║██╔══██╗    ██╔════╝████╗  ██║██╔══██╗
    // ██████╔╝██╔██╗ ██║██║  ██║    ██║     ██╔██╗ ██║██║  ██║
    // ██╔═══╝ ██║╚██╗██║██║  ██║    ██║     ██║╚██╗██║██║  ██║
    // ██║     ██║ ╚████║██████╔╝    ╚██████╗██║ ╚████║██████╔╝
    // ========================================================================
    // DnD CHARACTER API
    // ========================================================================

    svr.Get("/dnd/characters", [&](const httplib::Request&, httplib::Response& res){
        json arr = json::array();
        auto list = dndService.listCharacters();
        for (auto& c : list) {
            json j = c;
            arr.push_back(j);
        }
        res.set_content(arr.dump(2),"application/json");
    });


    svr.Post("/dnd/create-character", [&](const httplib::Request& req, httplib::Response& res){
        try {
            json j = json::parse(req.body);

            logDnDTx(j.dump());

            // → echte Transaktion in Mempool
            Transaction tx;
            tx.payload = std::vector<uint8_t>(j.dump().begin(), j.dump().end());
            tx.nonce   = std::time(nullptr);
            mempool.addTransaction(tx);

            // → unmittelbare State-Anwendung
            dndService.applyCreateJson(j);
            dndService.save();

            res.set_content("{\"status\":\"ok\"}", "application/json");
        }
        catch (...) {
            res.status = 400;
            res.set_content("{\"status\":\"error\"}", "application/json");
        }
    });


    svr.Post("/dnd/update-character", [&](const httplib::Request& req, httplib::Response& res){
        try {
            json j = json::parse(req.body);

            logDnDTx(j.dump());

            Transaction tx;
            tx.payload = std::vector<uint8_t>(j.dump().begin(), j.dump().end());
            tx.nonce   = std::time(nullptr);
            mempool.addTransaction(tx);

            dndService.applyUpdateJson(j);
            dndService.save();

            res.set_content("{\"status\":\"ok\"}", "application/json");
        }
        catch (...) {
            res.status = 400;
            res.set_content("{\"status\":\"error\"}","application/json");
        }
    });




    // ========================================================================
    // ███╗   ███╗ ██████╗ ██████╗ ███████╗███████╗██████╗
    // ████╗ ████║██╔═══██╗██╔══██╗██╔════╝██╔════╝██╔══██╗
    // ██╔████╔██║██║   ██║██████╔╝█████╗  █████╗  ██║  ██║
    // ██║╚██╔╝██║██║   ██║██╔═══╝ ██╔══╝  ██╔══╝  ██║  ██║
    // ██║ ╚═╝ ██║╚██████╔╝██║     ███████╗███████╗██████╔╝
    // ========================================================================
    // DnD MONSTER API
    // ========================================================================

    svr.Get("/dnd/monsters", [&](const httplib::Request&, httplib::Response& res){
        json arr = json::array();
        for (auto& m : monsterService.list()) {
            json j = m;
            arr.push_back(j);
        }
        res.set_content(arr.dump(2),"application/json");
    });


    svr.Post("/dnd/monster", [&](const httplib::Request& req, httplib::Response& res){
        try {
            json j = json::parse(req.body);
            dnd::Monster m = j.get<dnd::Monster>();

            if (m.id.empty())
                m.id = "mob-" + std::to_string(std::time(nullptr));

            monsterService.upsert(m);
            monsterService.save();

            json out;
            out["status"] = "ok";
            out["id"] = m.id;
            res.set_content(out.dump(2),"application/json");
        }
        catch (...) {
            res.status = 400;
            res.set_content("{\"error\":\"invalid monster json\"}", "application/json");
        }
    });




    // ========================================================================
    // ███████╗███╗   ██╗ ██████╗███████╗███╗   ██╗ ██████╗████████╗███████╗
    // ██╔════╝████╗  ██║██╔════╝██╔════╝████╗  ██║██╔════╝╚══██╔══╝██╔════╝
    // █████╗  ██╔██╗ ██║██║     █████╗  ██╔██╗ ██║██║        ██║   ███████╗
    // ██╔══╝  ██║╚██╗██║██║     ██╔══╝  ██║╚██╗██║██║        ██║   ╚════██║
    // ███████╗██║ ╚████║╚██████╗███████╗██║ ╚████║╚██████╗   ██║   ███████║
    // ========================================================================
    // DnD ENCOUNTER + INITIATIVE API
    // ========================================================================

    svr.Post("/dnd/encounter/start",[&](const httplib::Request& req, httplib::Response& res){
        try {
            json j = json::parse(req.body);
            std::string name = j.value("name","Encounter");
            auto& enc = encounterService.startEncounter(name);

            json out;
            out["status"] = "ok";
            out["encounterId"] = enc.id;

            res.set_content(out.dump(2),"application/json");
        }
        catch (...) {
            res.status = 400;
            res.set_content("{\"error\":\"invalid json\"}","application/json");
        }
    });


    svr.Post("/dnd/encounter/add-character",[&](const httplib::Request& req, httplib::Response& res){
        try {
            json j = json::parse(req.body);
            bool ok = encounterService.addCharacter(
                j.value("encounterId",""),
                j.value("characterId",""),
                j.value("initiative",0)
            );
            if (!ok) { res.status = 404; }
            res.set_content("{\"status\":\"ok\"}","application/json");
        }
        catch (...) { res.status = 400; }
    });


    svr.Post("/dnd/encounter/add-monster",[&](const httplib::Request& req, httplib::Response& res){
        try {
            json j = json::parse(req.body);
            bool ok = encounterService.addMonster(
                j.value("encounterId",""),
                j.value("monsterId",""),
                j.value("initiative",0)
            );
            if (!ok) { res.status = 404; }
            res.set_content("{\"status\":\"ok\"}","application/json");
        }
        catch (...) { res.status = 400; }
    });


    svr.Post("/dnd/encounter/next",[&](const httplib::Request& req, httplib::Response& res){
        try {
            json j = json::parse(req.body);
            bool ok = encounterService.nextTurn(j.value("encounterId",""));
            if (!ok) res.status = 404;
            res.set_content("{\"status\":\"ok\"}","application/json");
        }
        catch (...) { res.status = 400; }
    });


    svr.Get("/dnd/encounter",[&](const httplib::Request& req, httplib::Response& res){
        if (!req.has_param("id")) {
            res.status = 400;
            res.set_content("{\"error\":\"missing id\"}","application/json");
            return;
        }

        std::string id = req.get_param_value("id");

        dnd::Encounter e;
        if (!encounterService.get(id, e)) {
            res.status = 404;
            res.set_content("{\"error\":\"not found\"}","application/json");
            return;
        }

        json j = e;
        res.set_content(j.dump(2),"application/json");
    });


    svr.Get("/dnd/encounters",[&](const httplib::Request&, httplib::Response& res){
        json arr = json::array();
        for (auto& e : encounterService.list()) {
            json j;
            j["id"] = e.id;
            j["name"] = e.name;
            j["round"] = e.round;
            j["active"] = e.active;
            arr.push_back(j);
        }
        res.set_content(arr.dump(2),"application/json");
    });
   // =========================
    // 4) HTML-UI
    // =========================

    svr.Get("/",[&](const httplib::Request&,httplib::Response&res){
        std::string html = R"(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Blockchain + DnD Dashboard</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<style>
body{font-family:Arial;background:#0e1117;color:#eee;margin:2em;}
h1{color:#61dafb;}
.card{background:#1c1f26;padding:1em;border-radius:12px;margin-bottom:1.2em;}
button{padding:6px 10px;border-radius:6px;border:0;cursor:pointer;margin-right:6px;}
input,select,textarea{padding:4px;border-radius:6px;border:1px solid #555;background:#222;color:#eee;}
.ok{color:#5cb85c;} .fail{color:#d9534f;}
.character-box{background:#181b21;padding:1em;margin-bottom:1em;border-radius:8px;}
.itemlist{margin-left:1em;}
textarea{width:100%;box-sizing:border-box;font-family:monospace;}
small{color:#999;}
.active-turn{border:2px solid #61dafb;}
.enc-table td, .enc-table th{padding:4px 8px;}
</style>
</head>

<body>
<h1>Blockchain Reliability + DnD Dashboard</h1>

<div id="status" class="card">Loading…</div>
<div class="card"><canvas id="rtoChart" height="100"></canvas></div>
<div id="alerts" class="card"></div>

<div class="card">
  <h2>DnD Characters</h2>
  <div id="dnd-list">Loading…</div>

  <h3>Create New Character</h3>
  <div>
    Name: <input id="newName" placeholder="Name">
    Level: <input id="newLevel" type="number" value="1" style="width:70px;">
    HP: <input id="newHp" type="number" value="10" style="width:70px;">
    <button onclick="createCharacterDirect()">Create</button>
  </div>
</div>

<div class="card">
  <h2>Encounter</h2>
  <div>
    Charakter-IDs (kommagetrennt): <input id="encChars" style="width:60%;">
    <button onclick="startEncounter()">Start Encounter</button>
    <button onclick="nextTurn()">Next Turn</button>
  </div>
  <div id="encounter-state">No encounter</div>

  <h3>Quick Attack</h3>
  <div>
    Attacker ID: <input id="atkAttackerId" style="width:140px;">
    Target ID: <input id="atkTargetId" style="width:140px;">
    Weapon: <input id="atkWeapon" value="Sword" style="width:120px;">
    Attack Bonus: <input id="atkBonus" type="number" value="5" style="width:70px;">
    Damage Expr: <input id="atkDamage" value="1d8+3" style="width:100px;">
    <button onclick="encounterAttack()">Attack</button>
  </div>
</div>

<div class="card">
  <h2>Combat Log</h2>
  <div id="combat-log">Loading…</div>
</div>

<script>
async function GET(url){ return fetch(url).then(r=>r.json()); }
async function POST(url,obj){
  return fetch(url,{
    method:"POST",
    headers:{"Content-Type":"application/json"},
    body:JSON.stringify(obj||{})
  }).then(r=>r.json().catch(()=>({})));
}

// ---------- Status / RTO / Alerts ----------
let chart;
async function loadStatus(){
  const s = await GET('/api/status').catch(()=>null);
  const r = await GET('/api/rto').catch(()=>[]);
  const a = await fetch('/api/alerts').then(r=>r.json()).catch(()=>[]);

  if(!s){
    document.getElementById('status').innerHTML = "<b>Status:</b> /api/status not reachable";
    return;
  }

  const ok = v=>v?"<span class='ok'>OK</span>":"<span class='fail'>FAIL</span>";

  document.getElementById('status').innerHTML = `
    <h2>Status</h2>
    Integrity: ${ok(s.integrity)} |
    Performance: ${ok(s.performance)} (${s.avg_rto_ms.toFixed(0)} ms) |
    Chaos: ${ok(s.chaos)} |
    Forecast: ${ok(s.forecast)} |
    Pass Rate: ${s.pass_rate.toFixed(1)}% |
    Anomalies: ${s.anomalies}
  `;

  loadRtoChart(r);

  let al = "<h2>Alerts</h2><ul>";
  a.forEach(x=>al += `<li>${x}</li>`);
  al+="</ul>";
  document.getElementById("alerts").innerHTML = al;
}

function loadRtoChart(data){
  const ctx=document.getElementById('rtoChart');
  if(!ctx) return;

  const labels=data.map((_,i)=>i);
  const rtos=data.map(e=>e.rto_ms);
  const pass=data.map(e=>e.passed?1:0);

  if(chart) chart.destroy();
  chart=new Chart(ctx,{
    type:'line',
    data:{
      labels:labels,
      datasets:[
        {label:'RTO (ms)',data:rtos,borderColor:'#61dafb',tension:0.25},
        {label:'Pass',data:pass,borderColor:'#5cb85c',tension:0.25,yAxisID:'y2'}
      ]
    },
    options:{
      scales:{
        x:{ticks:{color:'#aaa'}},
        y:{ticks:{color:'#aaa'},title:{display:true,text:'RTO',color:'#aaa'}},
        y2:{position:'right',min:0,max:1,ticks:{color:'#aaa'}}
      }
    }
  });
}

// ---------- DnD Characters ----------
async function loadDnD(){
  const list = await GET('/dnd/characters').catch(()=>[]);
  let html = "";

  if(list.length === 0){
    html = "<i>Keine Charaktere gefunden.</i>";
  } else {
    list.forEach(ch=>{
      html += `
        <div class="character-box">
          <b>${ch.id}</b><br>
          <h3>${ch.name} (Lv ${ch.level})</h3>
          <b>HP:</b> ${ch.hpCurrent}/${ch.hpMax} <br>
          <b>XP:</b> ${ch.xp}<br>
          Stats: STR ${ch.stats.str}, DEX ${ch.stats.dex}, CON ${ch.stats.con},
                 INT ${ch.stats.intl}, WIS ${ch.stats.wis}, CHA ${ch.stats.cha}<br>
          <b>Inventory:</b>
          <ul class="itemlist">
            ${ch.inventory.map(i=>`<li>${i}</li>`).join("")}
          </ul>

          <button onclick="heal('${ch.id}',5)">Heal +5</button>
          <button onclick="damage('${ch.id}',5)">Damage -5</button>
          <button onclick="addItem('${ch.id}')">Add Item</button>
        </div>
      `;
    });
  }

  document.getElementById('dnd-list').innerHTML = html;
}

async function createCharacterDirect(){
  const name=document.getElementById("newName").value;
  const level=parseInt(document.getElementById("newLevel").value||"1");
  const hp=parseInt(document.getElementById("newHp").value||"10");

  await POST('/dnd/create-character',{
      "id": crypto.randomUUID(),
      "name": name,
      "level": level,
      "hpCurrent": hp,
      "hpMax": hp,
      "xp": 0,
      "notes": "",
      "stats": { "str":10,"dex":10,"con":10,"int":10,"wis":10,"cha":10 },
      "inventory":[]
  });
  loadDnD();
}

async function heal(id,amount){
  await POST('/dnd/update-character',{
    "characterId": id,
    "patchData": { "hpDelta": amount }
  });
  loadDnD();
}

async function damage(id,amount){
  await POST('/dnd/update-character',{
    "characterId": id,
    "patchData": { "hpDelta": -amount }
  });
  loadDnD();
}

async function addItem(id){
  let item = prompt("Item name?");
  if(!item) return;
  await POST('/dnd/update-character',{
    "characterId": id,
    "patchData": { "addItem": item }
  });
  loadDnD();
}

// ---------- Encounter ----------
async function loadEncounter(){
  const div = document.getElementById("encounter-state");
  const data = await GET('/dnd/encounter').catch(()=>null);
  if(!data || !data.active){
    div.innerHTML = "<i>No active encounter</i>";
    return;
  }

  let html = `<b>Round ${data.round}</b><br><br>`;
  html += `<table class="enc-table"><tr><th>ID</th><th>Name</th><th>Init</th><th>HP</th><th>Alive</th></tr>`;
  (data.participants || []).forEach(p=>{
    const cls = p.isActive ? "active-turn" : "";
    html += `<tr class="${cls}">
      <td>${p.characterId}</td>
      <td>${p.name||""}</td>
      <td>${p.initiative}</td>
      <td>${p.hpCurrent}/${p.hpMax}</td>
      <td>${p.alive ? "yes" : "no"}</td>
    </tr>`;
  });
  html += "</table>";

  div.innerHTML = html;
}

async function startEncounter(){
  const raw = document.getElementById("encChars").value;
  const ids = raw.split(",").map(s=>s.trim()).filter(Boolean);
  if(ids.length === 0){
    alert("Bitte mindestens eine Charakter-ID eingeben.");
    return;
  }
  await POST('/dnd/encounter/start', {characterIds: ids});
  loadEncounter();
}

async function nextTurn(){
  await POST('/dnd/encounter/next-turn',{});
  loadEncounter();
}

async function encounterAttack(){
  const attackerId = document.getElementById("atkAttackerId").value.trim();
  const targetId   = document.getElementById("atkTargetId").value.trim();
  const weapon     = document.getElementById("atkWeapon").value.trim();
  const bonus      = parseInt(document.getElementById("atkBonus").value||"0");
  const dmg        = document.getElementById("atkDamage").value.trim();

  if(!attackerId || !targetId){
    alert("Attacker und Target ID müssen gesetzt sein.");
    return;
  }

  const payload = {
    attackerId: attackerId,
    targetId: targetId,
    weaponName: weapon,
    attackBonus: bonus,
    damageExpr: dmg,
    advantage: "normal"
  };

  const resp = await POST('/dnd/encounter/attack', payload);
  console.log("Attack result:", resp);
  loadDnD();
  loadEncounter();
  loadCombatLog();
}

// ---------- Combat Log ----------
async function loadCombatLog(){
  const data = await GET('/dnd/combat-log').catch(()=>null);
  const div = document.getElementById("combat-log");
  if(!data || !data.entries){
    div.innerHTML = "<i>No entries</i>";
    return;
  }

  let html = "<ul>";
  data.entries.forEach(e=>{
    if(e.type === "attack"){
      html += `<li>[Attack] ${e.attackerId} -> ${e.targetId}, hit=${e.hit}, dmg=${e.damageTotal}</li>`;
    } else if(e.type === "savingThrow"){
      html += `<li>[Save] ${e.actorId} ${e.saveName} vs DC ${e.dc}, success=${e.success}, dmg=${e.damageTotal}</li>`;
    } else if(e.type === "skillCheck"){
      html += `<li>[Skill] ${e.actorId} ${e.skillName}, total=${e.roll.total}</li>`;
    } else {
      html += `<li>${JSON.stringify(e)}</li>`;
    }
  });
  html += "</ul>";
  div.innerHTML = html;
}

// ---------- Init ----------
loadStatus();
loadDnD();
loadEncounter();
loadCombatLog();
setInterval(loadStatus,10000);
setInterval(loadDnD,8000);
setInterval(loadEncounter,8000);
setInterval(loadCombatLog,15000);
</script>
</body>
</html>
)";
        res.set_content(html,"text/html");
    });

    std::cout << "[DashboardServer] Listening on 0.0.0.0:" << port << "\n";
    svr.listen("0.0.0.0", port);
}

