#include "analytics/forecaster.hpp"
#include <cmath>
#include <numeric>

static double mean(const std::vector<double>& v){ if(v.empty()) return 0.0; double s=std::accumulate(v.begin(),v.end(),0.0); return s/v.size(); }
static double variance(const std::vector<double>& v, double m){ if(v.size()<2) return 0.0; double s=0.0; for(double x:v){ double d=x-m; s+=d*d; } return s/(v.size()); }

FitResult Forecaster::fitLinear(const std::vector<double>& y){
    FitResult r{}; r.valid=false; if(y.size()<2) return r;
    int n=(int)y.size();
    double sumx=0, sumy=0, sumxx=0, sumxy=0;
    for(int i=0;i<n;++i){ double x=i; double yi=y[i]; sumx+=x; sumy+=yi; sumxx+=x*x; sumxy+=x*yi; }
    double denom=n*sumxx - sumx*sumx;
    if(std::fabs(denom)<1e-12) return r;
    r.slope=(n*sumxy - sumx*sumy)/denom;
    r.intercept=(sumy - r.slope*sumx)/n;
    double ssTot=0, ssRes=0; double ym=sumy/n;
    for(int i=0;i<n;++i){ double x=i; double yi=y[i]; double yhat=r.slope*x + r.intercept; double d=yi-ym; ssTot+=d*d; double e=yi-yhat; ssRes+=e*e; }
    r.mse=ssRes/n;
    r.r2 = ssTot>0 ? 1.0 - ssRes/ssTot : 0.0;
    r.mean = mean(y);
    r.stddev = std::sqrt(variance(y, r.mean));
    r.valid=true;
    return r;
}

double Forecaster::predictAt(const FitResult& f, double x){
    return f.slope*x + f.intercept;
}

std::vector<double> Forecaster::predictNext(const FitResult& f, int n, int startIndex){
    std::vector<double> out; out.reserve(n);
    for(int i=0;i<n;++i){ out.push_back(predictAt(f, startIndex + i)); }
    return out;
}

std::vector<int> Forecaster::detectAnomalies(const std::vector<double>& y, const FitResult& f, double zThreshold, double pctThreshold){
    std::vector<int> idx;
    if(y.empty() || !f.valid) return idx;
    for(int i=0;i<(int)y.size();++i){
        double pred=predictAt(f, i);
        double val=y[i];
        double z = f.stddev>1e-9 ? std::fabs(val - f.mean)/f.stddev : 0.0;
        double pct = pred!=0.0 ? std::fabs(val - pred)/std::fabs(pred) : 0.0;
        if(z > zThreshold || pct > pctThreshold) idx.push_back(i);
    }
    return idx;
}

