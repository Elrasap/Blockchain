// playerCrypto.js
class PlayerCrypto {

    static keyPair = null;

    static async generate() {
        this.keyPair = await crypto.subtle.generateKey(
            {
                name: "NODE-ED25519",
                namedCurve: "NODE-ED25519"
            },
            true,
            ["sign", "verify"]
        );
    }

    static async sign(msgUint8) {
        if (!this.keyPair) throw "Key not generated!";
        return new Uint8Array(
            await crypto.subtle.sign(
                "NODE-ED25519",
                this.keyPair.privateKey,
                msgUint8
            )
        );
    }

    static async exportPublicKeyRaw() {
        if (!this.keyPair) throw "Key not generated!";
        return new Uint8Array(
            await crypto.subtle.exportKey("raw", this.keyPair.publicKey)
        );
    }

    static async exportPrivateKeyRaw() {
        if (!this.keyPair) throw "Key not generated!";
        return new Uint8Array(
            await crypto.subtle.exportKey("pkcs8", this.keyPair.privateKey)
        );
    }

    static getPublicHex() {
        return Array.from(this.exportPublicKeyRawSync())
            .map(b => b.toString(16).padStart(2, "0"))
            .join("");
    }

    // kleine Sync-Hack nur fürs Anzeigen
    static exportPublicKeyRawSync() {
        if (!this.keyPair) return new Uint8Array();
        return new Uint8Array(
            crypto.subtle.exportKey("raw", this.keyPair.publicKey)
        );
    }
}

