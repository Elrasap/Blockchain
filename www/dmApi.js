// dmApi.js
const dmApi = {
    async getMempool() {
        const res = await fetch("/dnd/api/mempool");
        return await res.json();
    },

    async mineBlock() {
        const res = await fetch("/dnd/api/block/mine", {
            method: "POST"
        });
        return await res.json();
    },

    async getChain() {
        const res = await fetch("/chain/list");
        return await res.json();
    },

    async getDndState() {
        const res = await fetch("/dnd/state");
        return await res.json();
    }
};

