#pragma once
#include <vector>
#include <string>

struct FitResult {
    double slope;
    double intercept;
    double r2;
    double mse;
    double mean;
    double stddev;
    bool valid;
};

class Forecaster {
public:
    static FitResult fitLinear(const std::vector<double>& y);
    static double predictAt(const FitResult& f, double x);
    static std::vector<double> predictNext(const FitResult& f, int n, int startIndex);
    static std::vector<int> detectAnomalies(const std::vector<double>& y, const FitResult& f, double zThreshold, double pctThreshold);
};

