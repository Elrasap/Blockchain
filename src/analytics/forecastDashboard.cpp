#include "analytics/forecastDashboard.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>

using json = nlohmann::json;

ForecastDashboard::ForecastDashboard(const std::string& reportsDir) : dir(reportsDir) {}

ForecastSummary ForecastDashboard::run(int horizon, double zThresh, double pctThresh){
    RtoRpoAnalyzer analyzer(dir);
    auto runs = analyzer.analyzeAll();
    std::vector<double> y;
    y.reserve(runs.size());
    for(auto& r: runs) y.push_back(r.rto_ms);
    FitResult fit = Forecaster::fitLinear(y);
    std::vector<double> fc = fit.valid ? Forecaster::predictNext(fit, horizon, (int)y.size()) : std::vector<double>{};
    std::vector<int> an = fit.valid ? Forecaster::detectAnomalies(y, fit, zThresh, pctThresh) : std::vector<int>{};
    ForecastSummary s; s.fit=fit; s.forecast=fc; s.anomalies=an; s.n=(int)y.size();
    return s;
}

bool ForecastDashboard::writeJson(const std::string& path, const ForecastSummary& s, const std::vector<double>& y){
    json j;
    j["points"] = y;
    j["fit"]["slope"] = s.fit.slope;
    j["fit"]["intercept"] = s.fit.intercept;
    j["fit"]["r2"] = s.fit.r2;
    j["fit"]["mse"] = s.fit.mse;
    j["fit"]["mean"] = s.fit.mean;
    j["fit"]["stddev"] = s.fit.stddev;
    j["fit"]["valid"] = s.fit.valid;
    j["forecast"] = s.forecast;
    j["anomalies"] = s.anomalies;
    std::ofstream out(path);
    if(!out) return false;
    out << std::setw(2) << j;
    return true;
}

bool ForecastDashboard::writeCsv(const std::string& path, const ForecastSummary& s, const std::vector<double>& y){
    std::ofstream out(path);
    if(!out) return false;
    out << "index,rto_ms,pred_ms,anomaly\n";
    int n = s.n;
    for(int i=0;i<n;++i){
        double pred = s.fit.valid ? Forecaster::predictAt(s.fit, i) : 0.0;
        int an = 0; for(int k: s.anomalies) if(k==i) { an=1; break; }
        out << i << "," << y[i] << "," << pred << "," << an << "\n";
    }
    for(int h=0; h<(int)s.forecast.size(); ++h){
        out << (n+h) << ",," << s.forecast[h] << ",\n";
    }
    return true;
}

