// teamApi.js
const teamApi = {
    async getDndState() {
        const res = await fetch("/dnd/state");
        return await res.json();
    },

    async getEncounter(encounterId) {
        const res = await fetch(`/dnd/encounter/${encodeURIComponent(encounterId)}`);
        return await res.json();
    }
};

