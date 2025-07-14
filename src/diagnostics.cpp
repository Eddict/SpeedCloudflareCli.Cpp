#include "diagnostics.h"
#include "json_helpers.h"
#include "types.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <regex>
#include <string>
#include <vector>
#include <yyjson.h>

constexpr int kTestObjects = 8;

void yyjson_minimal_test(bool diagnostics_mode, bool debug_mode) {
  if (!(diagnostics_mode || debug_mode)) {
    return;
  }
  std::cerr << "[DIAG] Running minimal yyjson test with " << kTestObjects << " objects...\n";
  yyjson_mut_doc *doc = yyjson_mut_doc_new(nullptr);
  yyjson_mut_val *array_val = yyjson_mut_arr(doc);
  for (int object_index = 0; object_index < kTestObjects; ++object_index) {
    yyjson_mut_val *object_val = yyjson_mut_obj(doc);
    yyjson_mut_obj_add_str(doc, object_val, "label", "test");
    yyjson_mut_obj_add_int(doc, object_val, "value", object_index);
    yyjson_mut_arr_add_val(array_val, object_val);
  }
  yyjson_mut_val *root_val = yyjson_mut_obj(doc);
  yyjson_mut_obj_add_val(doc, root_val, "results", array_val);
  yyjson_mut_doc_set_root(doc, root_val);
  size_t json_length = 0;
  char *json = static_cast<char *>(yyjson_mut_write(doc, 0, &json_length));
  std::unique_ptr<char, decltype(&free)> json_ptr(json, &free);
  if (json_ptr) {
    std::cerr << "[DIAG] Minimal test serialization succeeded! Length: " << json_length << "\n";
  } else {
    std::cerr << "[DIAG] Minimal test serialization FAILED!\n";
  }
  yyjson_mut_doc_free(doc);
}

auto validate_json_schema(const std::string &json_path,
                          const std::string &schema_path,
                          bool diagnostics_mode) -> bool {
  using nlohmann::json;
  using nlohmann::json_schema::json_validator;
  std::ifstream schema_stream(schema_path);
  std::ifstream json_stream(json_path);
  if (!schema_stream) {
    if (diagnostics_mode) {
      std::cerr << "[SCHEMA] Could not open schema file: " << schema_path << "\n";
    }
    return false;
  }
  if (!json_stream) {
    if (diagnostics_mode) {
      std::cerr << "[SCHEMA] Could not open JSON file: " << json_path << "\n";
    }
    return false;
  }
  json schema_json = {};
  json data_json = {};
  json_validator validator = {};
  try {
    schema_stream >> schema_json;
    json_stream >> data_json;
  } catch (const std::exception &exception) {
    if (diagnostics_mode) {
      std::cerr << "[SCHEMA] Failed to parse JSON or schema: " << exception.what() << "\n";
    }
    return false;
  }
  try {
    validator.set_root_schema(schema_json);
  } catch (const std::exception &exception) {
    if (diagnostics_mode) {
      std::cerr << "[SCHEMA] Invalid schema: " << exception.what() << "\n";
    }
    return false;
  }
  try {
    validator.validate(data_json);
    if (diagnostics_mode) {
      std::cerr << "[SCHEMA] JSON is valid according to schema.\n";
    }
    return true;
  } catch (const std::exception &exception) {
    if (diagnostics_mode) {
      std::cerr << "[SCHEMA] JSON validation failed: " << exception.what() << "\n";
    }
    return false;
  }
}

// Fix: use std::vector<SummaryResult> instead of SummaryResults
void write_summary_json(const std::string &path, const std::vector<SummaryResult> &results) {
  // ...existing code for write_summary_json...
}
