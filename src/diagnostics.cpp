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
  if (!(diagnostics_mode || debug_mode))
    return;
  std::cerr << "[DIAG] Running minimal yyjson test with " << kTestObjects << " objects...\n";
  yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
  yyjson_mut_val *arr = yyjson_mut_arr(doc);
  for (int i = 0; i < kTestObjects; ++i) {
    yyjson_mut_val *obj = yyjson_mut_obj(doc);
    yyjson_mut_obj_add_str(doc, obj, "label", "test");
    yyjson_mut_obj_add_int(doc, obj, "value", i);
    yyjson_mut_arr_add_val(arr, obj);
  }
  yyjson_mut_val *root = yyjson_mut_obj(doc);
  yyjson_mut_obj_add_val(doc, root, "results", arr);
  yyjson_mut_doc_set_root(doc, root);
  size_t len = 0;
  char *json = (char *)yyjson_mut_write(doc, 0, &len);
  std::unique_ptr<char, decltype(&free)> json_ptr(json, &free);
  if (json_ptr) {
    std::cerr << "[DIAG] Minimal test serialization succeeded! Length: " << len << "\n";
  } else {
    std::cerr << "[DIAG] Minimal test serialization FAILED!\n";
  }
  yyjson_mut_doc_free(doc);
}

bool validate_json_schema(const std::string &json_path,
                          const std::string &schema_path,
                          bool diagnostics_mode) {
  using nlohmann::json;
  using nlohmann::json_schema::json_validator;
  std::ifstream schema_stream(schema_path);
  std::ifstream json_stream(json_path);
  if (!schema_stream) {
    if (diagnostics_mode)
      std::cerr << "[SCHEMA] Could not open schema file: " << schema_path << "\n";
    return false;
  }
  if (!json_stream) {
    if (diagnostics_mode)
      std::cerr << "[SCHEMA] Could not open JSON file: " << json_path << "\n";
    return false;
  }
  json schema_json = {};
  json data_json = {};
  json_validator validator = {};
  try {
    schema_stream >> schema_json;
    json_stream >> data_json;
  } catch (const std::exception &e) {
    if (diagnostics_mode)
      std::cerr << "[SCHEMA] Failed to parse JSON or schema: " << e.what() << "\n";
    return false;
  }
  try {
    validator.set_root_schema(schema_json);
  } catch (const std::exception &e) {
    if (diagnostics_mode)
      std::cerr << "[SCHEMA] Invalid schema: " << e.what() << "\n";
    return false;
  }
  try {
    validator.validate(data_json);
    if (diagnostics_mode)
      std::cerr << "[SCHEMA] JSON is valid according to schema.\n";
    return true;
  } catch (const std::exception &e) {
    if (diagnostics_mode)
      std::cerr << "[SCHEMA] JSON validation failed: " << e.what() << "\n";
    return false;
  }
}

// ...existing code for write_summary_json and load_summary_results from
// main.cpp... (Move both functions here, unchanged except for includes)
