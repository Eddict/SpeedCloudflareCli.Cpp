#pragma once
#include <string>
#include <vector>
#include "types.h"

// JSON helpers
std::string serialize_to_json(const TestResults& res);
double percentile(const std::vector<double>& v, double pct);
bool is_valid_utf8(const std::string& str);
