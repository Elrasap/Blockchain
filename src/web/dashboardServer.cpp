#include "web/dashboardServer.hpp"
#include "ops/reliabilityGuard.hpp"
#include "analytics/rtoRpoAnalyzer.hpp"
#include "analytics/trendAnalyzer.hpp"
#include <thirdparty/httplib.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>
#include "web/metricsEndpoint.hpp"


using json = nlohmann::json;
namespace fs = std::filesystem;

DashboardServer::DashboardServer(int p,const std::string& dir,const std::string& bin)
:port(p),reportsDir(dir),binaryPath(bin){}

void DashboardServer::start(){
    httplib::Server svr;

    // --- API: /api/status ---
    svr.Get("/api/status",[&](const httplib::Request&,httplib::Response&res){
        ReliabilityGuard guard(reportsDir,binaryPath);
        ReliabilityStatus s=guard.evaluate(8000.0,95.0,1);
        json j;
        j["integrity"]=s.integrityOk;
        j["performance"]=s.perfOk;
        j["chaos"]=s.chaosOk;
        j["forecast"]=s.forecastOk;
        j["avg_rto_ms"]=s.avgRto;
        j["pass_rate"]=s.passRate;
        j["anomalies"]=s.anomalies;
        res.set_content(j.dump(2),"application/json");
    });

    // --- API: /api/rto ---
    svr.Get("/api/rto",[&](const httplib::Request&,httplib::Response&res){
        RtoRpoAnalyzer a(reportsDir);
        auto runs=a.analyzeAll();
        json arr=json::array();
        for(auto&r:runs){
            json j;
            j["file"]=r.filename;
            j["rto_ms"]=r.rto_ms;
            j["restore_ms"]=r.restore_ms;
            j["passed"]=r.passed;
            arr.push_back(j);
        }
        res.set_content(arr.dump(2),"application/json");
    });

    // --- API: /api/alerts ---
    svr.Get("/api/alerts",[&](const httplib::Request&,httplib::Response&res){
        std::ifstream in("alerts.log");
        if(!in){res.status=404;return;}
        std::string line; json arr=json::array();
        while(std::getline(in,line)) arr.push_back(line);
        res.set_content(arr.dump(2),"application/json");
    });

    svr.Get("/api/trend",[&](const httplib::Request&,httplib::Response&res){
    TrendAnalyzer t("./history.db");
    auto data = t.loadDaily();
    auto summary = t.computeSummary(data);

    json j;
    j["mean_rto_ms"] = summary.meanRto;
    j["median_rto_ms"] = summary.medianRto;
    j["slope"] = summary.slope;
    j["regressions"] = summary.regressions;
    j["data"] = json::array();

    double regressionThreshold = summary.medianRto * 1.15;
    json regressionDays = json::array();

    for(auto& e : data){
        json d;
        d["date"] = e.date;
        d["avg_rto_ms"] = e.avgRto;
        d["pass_rate"] = e.passRate;
        d["anomalies"] = e.anomalies;
        bool regressed = e.avgRto > regressionThreshold;
        d["regression"] = regressed;
        if(regressed) regressionDays.push_back(e.date);
        j["data"].push_back(d);
    }

    j["regression_days"] = regressionDays;
    res.set_content(j.dump(2),"application/json");
    });
    // --- API: /metrics (Prometheus Pull) ---
    svr.Get("/metrics",[&](const httplib::Request&,httplib::Response&res){
    std::string metrics = MetricsEndpoint::collectMetrics();
    res.set_content(metrics,"text/plain; version=0.0.4");
    });


    // --- HTML Dashboard ---
    svr.Get("/",[&](const httplib::Request&,httplib::Response&res){
        std::string html = R"(
<!DOCTYPE html><html><head><meta charset="utf-8">
<title>Blockchain Trend Dashboard</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<style>
body{font-family:Arial;background:#0e1117;color:#eee;margin:2em;}
h1{color:#61dafb;}
.card{background:#1c1f26;padding:1em;border-radius:10px;margin-bottom:1em;}
.ok{color:#5cb85c;} .fail{color:#d9534f;}
canvas{background:#181b21;border-radius:8px;width:100%;max-width:800px;}
</style></head><body>
<h1>Blockchain Trend Dashboard</h1>
<div id="status" class="card">Loading...</div>
<div class="card"><canvas id="rtoChart" height="100"></canvas></div>
<div class="card"><canvas id="trendChart" height="100"></canvas></div>
<div id="alerts" class="card"></div>
<script>
let chart1, chart2;

async function load(){
  const s = await fetch('/api/status').then(r=>r.json());
  const r = await fetch('/api/rto').then(r=>r.json());
  const a = await fetch('/api/alerts').then(r=>r.json()).catch(()=>[]);
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
  const t = await fetch('/api/trend').then(r=>r.json()).catch(()=>({data:[]}));
  const ok = v=>v?"<span class='ok'>OK</span>":"<span class='fail'>FAIL</span>";
  document.getElementById('status').innerHTML =
    `<h2>Status</h2>
     Integrity: ${ok(s.integrity)} |
     Performance: ${ok(s.performance)} (${s.avg_rto_ms.toFixed(0)} ms) |
     Chaos: ${ok(s.chaos)} |
     Forecast: ${ok(s.forecast)} |
     Pass Rate: ${s.pass_rate.toFixed(1)} % |
     Anomalies: ${s.anomalies}`;
  updateRtoChart(r);
  updateTrendChart(t);
  const al=["<h2>Alerts</h2><ul>"];
  a.forEach(x=>al.push(`<li>${x}</li>`));al.push("</ul>");
  document.getElementById('alerts').innerHTML=al.join('');
}

function updateRtoChart(data){
  const ctx=document.getElementById('rtoChart').getContext('2d');
  const labels=data.map((_,i)=>i.toString());
  const rtos=data.map(e=>e.rto_ms);
  const pass=data.map(e=>e.passed?1:0);
  if(chart1) chart1.destroy();
  chart1=new Chart(ctx,{
    type:'line',
    data:{labels:labels,
      datasets:[
        {label:'RTO (ms)',data:rtos,borderColor:'#61dafb',tension:0.25},
        {label:'Pass',data:pass,borderColor:'#5cb85c',tension:0.25,yAxisID:'y2'}
      ]},
    options:{
      plugins:{legend:{labels:{color:'#ccc'}}},
      scales:{
        x:{ticks:{color:'#aaa'}},
        y:{ticks:{color:'#aaa'},title:{display:true,text:'RTO (ms)',color:'#aaa'}},
        y2:{position:'right',min:0,max:1,ticks:{color:'#aaa'}}
      }
    }
  });
}

function updateTrendChart(t){
  if(!t.data||t.data.length===0) return;
  const ctx=document.getElementById('trendChart').getContext('2d');
  const labels=t.data.map(d=>d.date);
  const rto=t.data.map(d=>d.avg_rto_ms);
  const pass=t.data.map(d=>d.pass_rate);
  const anomalies=t.regression_days||[];

  const annotations = anomalies.map(d=>({
    type:'point',
    xValue:d,
    yValue:t.data.find(e=>e.date===d)?.avg_rto_ms||0,
    backgroundColor:'red',
    radius:6,
    borderWidth:0
  }));

  if(chart2) chart2.destroy();
  chart2=new Chart(ctx,{
    type:'line',
    data:{labels:labels,
      datasets:[
        {label:'Avg RTO (ms)',data:rto,borderColor:'#f0ad4e',fill:false,tension:0.3},
        {label:'Pass Rate (%)',data:pass,borderColor:'#5cb85c',yAxisID:'y2',tension:0.3}
      ]},
    options:{
      plugins:{
        legend:{labels:{color:'#ccc'}},
        annotation:{annotations:annotations}
      },
      scales:{
        x:{ticks:{color:'#aaa'}},
        y:{ticks:{color:'#aaa'},title:{display:true,text:'RTO (ms)',color:'#aaa'}},
        y2:{position:'right',ticks:{color:'#aaa'},title:{display:true,text:'Pass %',color:'#aaa'}}
      }
    }
  });
}

load(); setInterval(load,20000);
</script></body></html>
)";
        res.set_content(html,"text/html");
    });

    std::cout<<"[Dashboard] running on http://localhost:"<<port<<"\n";
    svr.listen("0.0.0.0",port);
}

