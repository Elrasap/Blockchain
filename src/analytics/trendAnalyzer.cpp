#include "analytics/trend_analyzer.hpp"
#include <sqlite3.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <cmath>

using json = nlohmann::json;

TrendAnalyzer::TrendAnalyzer(const std::string& dbPath) : path(dbPath) {}

std::vector<TrendEntry> TrendAnalyzer::loadDaily() {
    std::vector<TrendEntry> out;
    sqlite3* db=nullptr;
    if(sqlite3_open(path.c_str(),&db)!=SQLITE_OK) return out;

    const char* sql =
        "SELECT strftime('%Y-%m-%d', ts, 'unixepoch') AS day,"
        " AVG(avg_rto) AS avg_rto,"
        " AVG(pass_rate) AS pass_rate,"
        " AVG(anomalies) AS anomalies"
        " FROM reliability_history"
        " GROUP BY day ORDER BY day ASC;";
    sqlite3_stmt* stmt=nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    while(sqlite3_step(stmt)==SQLITE_ROW){
        TrendEntry e;
        e.date = reinterpret_cast<const char*>(sqlite3_column_text(stmt,0));
        e.avgRto = sqlite3_column_double(stmt,1);
        e.passRate = sqlite3_column_double(stmt,2);
        e.anomalies = (int)sqlite3_column_double(stmt,3);
        out.push_back(e);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return out;
}

TrendSummary TrendAnalyzer::computeSummary(const std::vector<TrendEntry>& data){
    TrendSummary s{};
    if(data.empty()) return s;
    std::vector<double> rt;
    for(auto&d:data) rt.push_back(d.avgRto);
    double sum=0; for(double v:rt) sum+=v;
    s.meanRto = sum / rt.size();
    std::sort(rt.begin(),rt.end());
    s.medianRto = rt[rt.size()/2];

    double sumx=0,sumy=0,sumxx=0,sumxy=0;
    int n=(int)rt.size();
    for(int i=0;i<n;++i){ sumx+=i; sumy+=rt[i]; sumxx+=i*i; sumxy+=i*rt[i]; }
    double denom=n*sumxx - sumx*sumx;
    s.slope = denom!=0 ? (n*sumxy - sumx*sumy)/denom : 0;

    double thresh = s.medianRto * 1.15;
    s.regressions = std::count_if(rt.begin(),rt.end(),[&](double v){return v>thresh;});
    return s;
}

bool TrendAnalyzer::writeJson(const std::string& path,const std::vector<TrendEntry>&data,const TrendSummary&s){
    json j;
    j["mean_rto_ms"]=s.meanRto;
    j["median_rto_ms"]=s.medianRto;
    j["slope"]=s.slope;
    j["regressions"]=s.regressions;
    j["data"]=json::array();
    for(auto&e:data){
        json d;
        d["date"]=e.date;
        d["avg_rto_ms"]=e.avgRto;
        d["pass_rate"]=e.passRate;
        d["anomalies"]=e.anomalies;
        j["data"].push_back(d);
    }
    std::ofstream out(path);
    if(!out)return false;
    out<<std::setw(2)<<j;
    return true;
}

bool TrendAnalyzer::writeCsv(const std::string& path,const std::vector<TrendEntry>&data){
    std::ofstream out(path);
    if(!out)return false;
    out<<"date,avg_rto_ms,pass_rate,anomalies\n";
    for(auto&e:data){
        out<<e.date<<","<<e.avgRto<<","<<e.passRate<<","<<e.anomalies<<"\n";
    }
    return true;
}

