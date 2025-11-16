#include "web/dndApi.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

using nlohmann::json;
using namespace std;

namespace web {

DndApi::DndApi(Blockchain& chain, Mempool& mem)
    : chain_(chain), mempool_(mem)
{
}

void DndApi::registerRoutes(httplib::Server& server)
{
    //
    // PLAYER submits a signed DnD Event TX
    //
    server.Post("/dnd/api/tx/propose", [&](const httplib::Request& req,
                                           httplib::Response& res)
    {
        try {
            if (!req.has_header("Content-Type") ||
                req.get_header_value("Content-Type") != "application/json")
            {
                res.status = 400;
                res.set_content("{\"error\":\"Content-Type must be application/json\"}",
                                "application/json");
                return;
            }

            json j = json::parse(req.body);

            // Decode DndEventTx
            dnd::DndEventTx evt;
            j.at("encounterId").get_to(evt.encounterId);
            j.at("actorId").get_to(evt.actorId);
            j.at("targetId").get_to(evt.targetId);
            j.at("actorType").get_to(evt.actorType);
            j.at("targetType").get_to(evt.targetType);
            j.at("roll").get_to(evt.roll);
            j.at("damage").get_to(evt.damage);
            j.at("hit").get_to(evt.hit);
            j.at("note").get_to(evt.note);
            j.at("timestamp").get_to(evt.timestamp);

            j.at("senderPubKey").get_to(evt.senderPubKey);
            j.at("signature").get_to(evt.signature);

            //
            // Convert DndEventTx → generic Transaction
            //
            Transaction tx = dnd::wrapDndTxIntoTransaction(evt);

            tx.signature = evt.signature;
            tx.senderPubkey = evt.senderPubKey;

            //
            // Validate TX (signatures + DnD rules)
            //
            string err;
            if (!chain_.validateTransaction(tx, err)) {
                json errJson = {{"error", err}};
                res.status = 400;
                res.set_content(errJson.dump(), "application/json");
                return;
            }

            //
            // Insert into mempool
            //
            if (!mempool_.addTransactionValidated(tx, err)) {
                json errJson = {{"error", err}};
                res.status = 400;
                res.set_content(errJson.dump(), "application/json");
                return;
            }

            // Success response
            json ok = {
                {"status", "ok"},
                {"accepted", true}
            };
            res.status = 200;
            res.set_content(ok.dump(), "application/json");
        }
        catch (std::exception& e) {
            json err = {{"error", e.what()}};
            res.status = 400;
            res.set_content(err.dump(), "application/json");
        }
        catch (...) {
            json err = {{"error", "unknown error"}};
            res.status = 400;
            res.set_content(err.dump(), "application/json");
        }
    });
}

} // namespace web

