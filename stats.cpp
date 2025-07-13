#include "stats.h"
#include <algorithm>
#include <cmath>

namespace stats {
constexpr double kTwo = 2.0;
double average(const std::vector<double> &values) {
  if (values.empty()) return 0.0;
  double total = 0.0;
  for (double v : values)
    total += v;
  return total / static_cast<double>(values.size());
}
double median(std::vector<double> values) {
  size_t n = values.size();
  if (n == 0) return 0.0;
  std::sort(values.begin(), values.end());
  if (n % 2)
    return values[n / 2];
  return (values[n / 2 - 1] + values[n / 2]) / kTwo;
}
double quartile(std::vector<double> values, double percentile) {
  if (values.empty()) return 0.0;
  std::sort(values.begin(), values.end());
  double pos = static_cast<double>(values.size() - 1) * percentile;
  size_t base = static_cast<size_t>(std::floor(pos));
  double rest = pos - static_cast<double>(base);
  if (base + 1 < values.size())
    return values[base] + rest * (values[base + 1] - values[base]);
  return values[base];
}
double jitter(const std::vector<double> &values) {
  if (values.size() < 2) return 0.0;
  std::vector<double> jitters{};
  for (size_t i = 0; i + 1 < values.size(); ++i)
    jitters.push_back(std::abs(values[i] - values[i + 1]));
  return average(jitters);
}
} // namespace stats
