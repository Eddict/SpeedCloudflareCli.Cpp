#pragma once
#include "types.h"
#include <string>
#include <vector>
#include <yyjson.h>

// JSON helpers
// Modernized: trailing return types, descriptive parameter names
auto serialize_to_json(const TestResults& results) -> std::string;
auto percentile(const std::vector<double>& values, double percentile_value) -> double;
auto is_valid_utf8(const std::string& input) -> bool;
void add_str(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, const std::string& value);
void add_num(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, double value);
