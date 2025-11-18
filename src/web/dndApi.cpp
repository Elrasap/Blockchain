#include "web/dndApi.hpp"
#include <nlohmann/json.hpp>

using nlohmann::json;

namespace web {

DndApi::DndApi(Blockchain& chain, Mempool& mempool)
    : chain_(chain),
      mempool_(mempool)
{
}

void DndApi::registerRoutes(httplib::Server& server)
{
    // Simple test endpoint
    server.Get("/dnd/ping", [&](const httplib::Request&, httplib::Response& res) {
        json j = {
            {"status", "ok"},
            {"height", chain_.getHeight()}
        };

        res.status = 200;
        res.set_content(j.dump(2), "application/json");
    });
}

} // namespace web

