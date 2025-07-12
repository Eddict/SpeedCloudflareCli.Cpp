#pragma once
#include <string>
#include <vector>
#include "types.h"
#include <set>
#include <regex>

// Diagnostics and debug helpers
void yyjson_minimal_test(bool diagnostics_mode = false, bool debug_mode = false);
bool validate_json_schema(const std::string& json_path, const std::string& schema_path, bool diagnostics_mode = false);
void write_summary_json(const std::vector<SummaryResult>& results, const std::string& filename, bool diagnostics_mode = false, bool debug_mode = false);
std::vector<SummaryResult> load_summary_results(const std::vector<std::string>& files, bool diagnostics_mode = false, bool debug_mode = false);
