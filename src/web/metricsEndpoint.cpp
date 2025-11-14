#include "web/metricsEndpoint.hpp"
#include "ops/reliabilityGuard.hpp"
#include "analytics/trendAnalyzer.hpp"
#include <sstream>
#include <iomanip>

std::string MetricsEndpoint::collectMetrics() {
    ReliabilityGuard guard("./reports", "./build/blockchain_node");
    auto s = guard.evaluate(8000.0,95.0,1);
    TrendAnalyzer t("./history.db");
    auto data = t.loadDaily();
    auto summary = t.computeSummary(data);

    std::ostringstream out;
    out << std::fixed << std::setprecision(2);
    out << "# HELP blockchain_integrity_ok Integrity check passed (1=true, 0=false)\n";
    out << "# TYPE blockchain_integrity_ok gauge\n";
    out << "blockchain_integrity_ok " << (s.integrityOk?1:0) << "\n";
    out << "# HELP blockchain_performance_ok Performance check passed (1=true, 0=false)\n";
    out << "# TYPE blockchain_performance_ok gauge\n";
    out << "blockchain_performance_ok " << (s.perfOk?1:0) << "\n";
    out << "# HELP blockchain_chaos_ok Chaos tests passed\n";
    out << "# TYPE blockchain_chaos_ok gauge\n";
    out << "blockchain_chaos_ok " << (s.chaosOk?1:0) << "\n";
    out << "# HELP blockchain_avg_rto_ms Average recovery time (ms)\n";
    out << "# TYPE blockchain_avg_rto_ms gauge\n";
    out << "blockchain_avg_rto_ms " << s.avgRto << "\n";
    out << "# HELP blockchain_pass_rate_percent Successful test pass rate\n";
    out << "# TYPE blockchain_pass_rate_percent gauge\n";
    out << "blockchain_pass_rate_percent " << s.passRate << "\n";
    out << "# HELP blockchain_anomalies_total Detected anomalies\n";
    out << "# TYPE blockchain_anomalies_total counter\n";
    out << "blockchain_anomalies_total " << s.anomalies << "\n";
    out << "# HELP blockchain_trend_mean_rto_ms Mean historical RTO (ms)\n";
    out << "# TYPE blockchain_trend_mean_rto_ms gauge\n";
    out << "blockchain_trend_mean_rto_ms " << summary.meanRto << "\n";
    out << "# HELP blockchain_trend_slope Recent performance slope\n";
    out << "# TYPE blockchain_trend_slope gauge\n";
    out << "blockchain_trend_slope " << summary.slope << "\n";
    out << "# HELP blockchain_trend_regressions_total Regression days\n";
    out << "# TYPE blockchain_trend_regressions_total counter\n";
    out << "blockchain_trend_regressions_total " << summary.regressions << "\n";
    return out.str();
}

