// dndTxClient.js
class DndTxClient {

    static async sendEvent(evt) {
        if (!PlayerCrypto.keyPair)
            return { error: "Generate key first!" };

        const pub = await PlayerCrypto.exportPublicKeyRaw();

        const payloadObj = {
            ...evt,
            senderPubKey: Array.from(pub),
        };

        const payloadJson = JSON.stringify(payloadObj);
        const msgBytes = new TextEncoder().encode(payloadJson);

        const sigBytes = await PlayerCrypto.sign(msgBytes);
        payloadObj.signature = Array.from(sigBytes);

        // final JSON
        const jsonTx = JSON.stringify(payloadObj);

        const res = await fetch("/dnd/api/tx/propose", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: jsonTx
        });

        return await res.json();
    }
}

