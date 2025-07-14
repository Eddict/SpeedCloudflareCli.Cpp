#pragma once
#include "types.h"
#include <regex>
#include <set>
#include <string>
#include <vector>

// Diagnostics and debug helpers
// Modernized: trailing return types, descriptive parameter names
auto yyjson_minimal_test(bool diagnostics_mode = false,
                         bool debug_mode = false) -> void;
auto validate_json_schema(const std::string &json_path,
                          const std::string &schema_path,
                          bool diagnostics_mode = false) -> bool;
auto write_summary_json(const std::vector<SummaryResult> &summary_results,
                        const std::string &filename,
                        bool diagnostics_mode = false, bool debug_mode = false) -> void;
auto load_summary_results(const std::vector<std::string> &files,
                     bool diagnostics_mode = false, bool debug_mode = false) -> std::vector<SummaryResult>;
