#pragma once
#include <string>  // for string

// Diagnostics and debug helpers
// Modernized: trailing return types, descriptive parameter names
auto yyjson_minimal_test(bool is_diagnostics = false, bool is_debug = false) -> void;
auto validate_json_schema(const std::string& json_path, const std::string& schema_path,
                          bool is_diagnostics = false) -> bool;
