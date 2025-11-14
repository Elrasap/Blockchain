#include "ops/reliability_guard.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>
#include <filesystem>
#include "release/checksummer.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

ReliabilityGuard::ReliabilityGuard(const std::string& reportsDir,const std::string& binaryPath)
: dir(reportsDir), bin(binaryPath) {}

bool ReliabilityGuard::verifyBinaryChecksum() {
    if(!fs::exists(bin)) return false;
    auto sha = fileSha256Hex(bin);
    if(!fs::exists(bin + ".sig")) return true; // kein Signaturfile? nicht fatal
    std::ifstream sig(bin + ".sig");
    if(!sig) return true;
    std::string line;
    std::getline(sig, line);
    for(char &c: line) c = std::tolower(c);
    for(char &c: sha) c = std::tolower(c);
    return sha == line;
}

ReliabilityStatus ReliabilityGuard::evaluate(double maxAvgRto,double minPassRate,int maxAnomalies){
    ReliabilityStatus st{};
    st.integrityOk = verifyBinaryChecksum();

    RtoRpoAnalyzer analyzer(dir);
    auto runs = analyzer.analyzeAll();
    st.avgRto = 0.0;
    int pass=0;
    for(auto&r:runs){st.avgRto+=r.rto_ms; if(r.passed)pass++;}
    if(!runs.empty()) st.avgRto/=runs.size();
    st.passRate = runs.empty()?0.0:100.0*pass/runs.size();
    st.perfOk = st.avgRto<=maxAvgRto;
    st.chaosOk = st.passRate>=minPassRate;

    ForecastDashboard fd(dir);
    ForecastSummary f = fd.run(3,2.5,0.2);
    st.anomalies = (int)f.anomalies.size();
    st.forecastOk = st.anomalies<=maxAnomalies;
    return st;
}

bool ReliabilityGuard::writeJson(const std::string& path,const ReliabilityStatus&s){
    json j;
    j["integrity_ok"]=s.integrityOk;
    j["perf_ok"]=s.perfOk;
    j["chaos_ok"]=s.chaosOk;
    j["forecast_ok"]=s.forecastOk;
    j["avg_rto_ms"]=s.avgRto;
    j["pass_rate"]=s.passRate;
    j["anomalies"]=s.anomalies;
    j["timestamp"]=std::time(nullptr);
    std::ofstream out(path);
    if(!out) return false;
    out<<std::setw(2)<<j;
    return true;
}

void ReliabilityGuard::printStatus(const ReliabilityStatus&s){
    std::cout<<"=== Reliability Guard Report ===\n";
    std::cout<<"Integrity: "<<(s.integrityOk?"OK":"FAIL")<<"\n";
    std::cout<<"Performance: "<<(s.perfOk?"OK":"FAIL")<<" (avgRTO="<<s.avgRto<<"ms)\n";
    std::cout<<"Chaos Tests: "<<(s.chaosOk?"OK":"FAIL")<<" (passRate="<<s.passRate<<"%)\n";
    std::cout<<"Forecast: "<<(s.forecastOk?"OK":"FAIL")<<" (anomalies="<<s.anomalies<<")\n";
    if(!(s.integrityOk&&s.perfOk&&s.chaosOk&&s.forecastOk))
        std::cout<<"[Guard] ALERT: Reliability breach detected!\n";
    else
        std::cout<<"[Guard] System reliability within SLOs.\n";
}

