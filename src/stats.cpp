#include "stats.h"
#include <algorithm>
#include <cmath>

namespace stats
{
constexpr double kTwo = 2.0;
// Modernized: trailing return types, braces, descriptive variable names, auto, nullptr, one
// declaration per statement, no implicit conversions

auto average(const std::vector<double>& values) -> double
{
  if (values.empty())
  {
    return 0.0;
  }
  double total = 0.0;
  for (const double value : values)
  {
    total += value;
  }
  return total / static_cast<double>(values.size());
}

auto median(std::vector<double> values) -> double
{
  const size_t value_count = values.size();
  if (value_count == 0)
  {
    return 0.0;
  }
  std::sort(values.begin(), values.end());
  if (value_count % 2)
  {
    return values[value_count / 2];
  }
  return (values[value_count / 2 - 1] + values[value_count / 2]) / kTwo;
}

auto quartile(std::vector<double> values, double percentile) -> double
{
  if (values.empty())
  {
    return 0.0;
  }
  std::sort(values.begin(), values.end());
  const double position = static_cast<double>(values.size() - 1) * percentile;
  const size_t base_index = static_cast<size_t>(std::floor(position));
  const double remainder = position - static_cast<double>(base_index);
  if (base_index + 1 < values.size())
  {
    return values[base_index] + remainder * (values[base_index + 1] - values[base_index]);
  }
  return values[base_index];
}

auto jitter(const std::vector<double>& values) -> double
{
  if (values.size() < 2)
  {
    return 0.0;
  }
  std::vector<double> jitters{};
  for (size_t index = 0; index + 1 < values.size(); ++index)
  {
    jitters.push_back(std::abs(values[index] - values[index + 1]));
  }
  return average(jitters);
}
} // namespace stats
