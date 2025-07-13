#pragma once
#include <vector>

namespace stats {
double average(const std::vector<double> &values);
double median(std::vector<double> values);
double quartile(std::vector<double> values, double percentile);
double jitter(const std::vector<double> &values);
} // namespace stats
