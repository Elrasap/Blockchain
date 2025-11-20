async function apiPost(endpoint, data) {
    const res = await fetch(endpoint, {
        method: "POST",
        headers: {"Content-Type": "application/json"},
        body: JSON.stringify(data)
    });
    return await res.json();
}

async function apiGet(endpoint) {
    const res = await fetch(endpoint);
    return await res.json();
}

// -------------------
// DM Functions
// -------------------
async function dmSend(type) {
    let enc = document.getElementById("encounterId").value;
    let actor = document.getElementById("actorId").value;

    const payload = {
        encounterId: enc,
        actorId: actor
    };

    let url = "/dnd/" + type;

    const out = await apiPost(url, payload);
    document.getElementById("output").textContent = JSON.stringify(out, null, 2);
}

async function loadState() {
    const st = await apiGet("/dnd/state");
    document.getElementById("state").textContent = JSON.stringify(st, null, 2);
}

// -------------------
// PLAYER FUNCTIONS
// -------------------
async function playerAction(type) {
    const urlParams = new URLSearchParams(window.location.search);
    const actor = urlParams.get("id");

    const payload = {
        encounterId: document.getElementById("enc-player").value,
        actorId: actor
    };

    const out = await apiPost("/dnd/" + type, payload);
    document.getElementById("p-out").textContent = JSON.stringify(out, null, 2);
}

