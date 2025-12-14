#include "ops/notification.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <curl/curl.h>   // libcurl for webhook

Notifier::Notifier(const std::string& webhook, const std::string& logfile)
: webhookUrl(webhook), logFile(logfile) {}

void Notifier::sendAlert(const ReliabilityStatus& s, AlertLevel level) {
    std::ostringstream oss;
    std::time_t t = std::time(nullptr);
    oss << "[" << std::put_time(std::localtime(&t), "%F %T") << "] ";

    std::string lvl = (level == AlertLevel::INFO ? "INFO" :
                      (level == AlertLevel::WARNING ? "WARNING" : "CRITICAL"));
    oss << lvl << ": ";

    oss << "Integrity=" << (s.integrityOk ? "OK" : "FAIL")
        << " Perf=" << (s.perfOk ? "OK" : "FAIL")
        << " Chaos=" << (s.chaosOk ? "OK" : "FAIL")
        << " Forecast=" << (s.forecastOk ? "OK" : "FAIL")
        << " RTO=" << s.avgRto << "ms PassRate=" << s.passRate
        << "% Anomalies=" << s.anomalies;

    writeLog(oss.str());
    sendWebhook(oss.str());
}

void Notifier::writeLog(const std::string& msg) {
    std::ofstream out(logFile, std::ios::app);
    if(!out) return;
    out << msg << "\n";
}

void Notifier::sendWebhook(const std::string& msg) {
    if(webhookUrl.empty()) return;
    CURL* curl = curl_easy_init();
    if(!curl) return;
    std::string payload = "{\"text\":\"" + msg + "\"}";
    curl_easy_setopt(curl, CURLOPT_URL, webhookUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, nullptr);
    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK)
        std::cerr << "[Notifier] CURL error: " << curl_easy_strerror(res) << "\n";
    curl_easy_cleanup(curl);
}

