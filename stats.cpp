#include "stats.h"
#include <algorithm>
#include <cmath>

namespace stats {
    double average(const std::vector<double>& values) {
        double total = 0;
        for (double v : values) total += v;
        return total / values.size();
    }
    double median(std::vector<double> values) {
        size_t n = values.size();
        std::sort(values.begin(), values.end());
        if (n % 2) return values[n/2];
        return (values[n/2 - 1] + values[n/2]) / 2.0;
    }
    double quartile(std::vector<double> values, double percentile) {
        std::sort(values.begin(), values.end());
        double pos = (values.size() - 1) * percentile;
        size_t base = static_cast<size_t>(std::floor(pos));
        double rest = pos - base;
        if (base + 1 < values.size())
            return values[base] + rest * (values[base + 1] - values[base]);
        return values[base];
    }
    double jitter(const std::vector<double>& values) {
        std::vector<double> jitters;
        for (size_t i = 0; i + 1 < values.size(); ++i)
            jitters.push_back(std::abs(values[i] - values[i+1]));
        return average(jitters);
    }
}
