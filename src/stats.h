#pragma once
#include <vector>

namespace stats
{
// Modernized: trailing return types, descriptive parameter names
auto average(const std::vector<double>& values) -> double;
auto median(std::vector<double> values) -> double;
auto quartile(std::vector<double> values, double percentile) -> double;
auto jitter(const std::vector<double>& values) -> double;
} // namespace stats
