#pragma once
#include "types.h"
#include <string>
#include <vector>
#include <yyjson.h>

// JSON helpers
std::string serialize_to_json(const TestResults &res);
double percentile(const std::vector<double> &v, double pct);
bool is_valid_utf8(const std::string &str);
void add_str(yyjson_mut_doc *doc, yyjson_mut_val *obj, const char *key, const std::string &val);
void add_num(yyjson_mut_doc *doc, yyjson_mut_val *obj, const char *key, double val);
