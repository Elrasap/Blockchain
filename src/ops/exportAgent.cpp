#include "ops/exportAgent.hpp"
#include "ops/reliabilityGuard.hpp"
#include "analytics/trendAnalyzer.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>
#include <thread>

using json = nlohmann::json;

ExportAgent::ExportAgent(const std::string& endpoint, int intervalSec)
    : endpointUrl(endpoint), interval(intervalSec), running(false) {}

ExportAgent::~ExportAgent() { stop(); }

void ExportAgent::start() {
    if(running) return;
    running = true;
    worker = std::thread(&ExportAgent::run, this);
}

void ExportAgent::stop() {
    if(!running) return;
    running = false;
    if(worker.joinable()) worker.join();
}

void ExportAgent::run() {
    std::cout << "[ExportAgent] Started (interval " << interval << "s)\n";
    while(running) {
        std::string payload = collectMetricsJson();
        sendJson(payload);
        std::this_thread::sleep_for(std::chrono::seconds(interval));
    }
    std::cout << "[ExportAgent] Stopped.\n";
}

std::string ExportAgent::collectMetricsJson() {
    ReliabilityGuard guard("./reports", "./build/blockchain_node");
    auto status = guard.evaluate(8000.0,95.0,1);
    TrendAnalyzer t("./history.db");
    auto data = t.loadDaily();
    auto summary = t.computeSummary(data);

    json j;
    j["integrity"] = status.integrityOk;
    j["performance"] = status.perfOk;
    j["chaos"] = status.chaosOk;
    j["forecast"] = status.forecastOk;
    j["avg_rto_ms"] = status.avgRto;
    j["pass_rate"] = status.passRate;
    j["anomalies"] = status.anomalies;
    j["trend_mean_rto_ms"] = summary.meanRto;
    j["trend_slope"] = summary.slope;
    j["trend_regressions"] = summary.regressions;
    j["timestamp"] = std::time(nullptr);
    return j.dump();
}

void ExportAgent::sendJson(const std::string& payload) {
    if(endpointUrl.empty()) return;
    CURL* curl = curl_easy_init();
    if(!curl) return;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, endpointUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK)
        std::cerr << "[ExportAgent] CURL error: " << curl_easy_strerror(res) << "\n";
    else
        std::cout << "[ExportAgent] Metrics exported to " << endpointUrl << "\n";
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

