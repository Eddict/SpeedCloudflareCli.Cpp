#include "diagnostics.h"
#include <yyjson.h>                   // for yyjson_mut_obj, yyjson_mut_arr
#include <cstdlib>                    // for free, size_t
#include <exception>                  // for exception
#include <fstream>                    // IWYU pragma: keep  // for ifstream
#include <iostream>                   // for operator<<, basic_ostream, cerr
#include <memory>                     // for unique_ptr
#include <nlohmann/json-schema.hpp>   // for json_validator
#include <nlohmann/json.hpp>          // for basic_json, basic_json<>::object_t
#include <nlohmann/json_fwd.hpp>      // for json
#include <set>                        // for operator!=, operator==
#include <string>                     // for char_traits, operator<<, string
#include "embedded_result_schema.h"   // for embedded_result_schema
#include "embedded_summary_schema.h"  // for embedded_summary_schema

constexpr int kTestObjects = 8;

void yyjson_minimal_test(bool is_diagnostics, bool is_debug)
{
  if (!(is_diagnostics || is_debug))
  {
    return;
  }
  std::cerr << "[DIAG] Running minimal yyjson test with " << kTestObjects << " objects...\n";
  yyjson_mut_doc* doc = yyjson_mut_doc_new(nullptr);
  yyjson_mut_val* array_val = yyjson_mut_arr(doc);
  for (int object_index = 0; object_index < kTestObjects; ++object_index)
  {
    yyjson_mut_val* object_val = yyjson_mut_obj(doc);
    yyjson_mut_obj_add_str(doc, object_val, "label", "test");
    yyjson_mut_obj_add_int(doc, object_val, "value", object_index);
    yyjson_mut_arr_add_val(array_val, object_val);
  }
  yyjson_mut_val* root_val = yyjson_mut_obj(doc);
  yyjson_mut_obj_add_val(doc, root_val, "results", array_val);
  yyjson_mut_doc_set_root(doc, root_val);
  size_t json_length = 0;
  char* json = static_cast<char*>(yyjson_mut_write(doc, 0, &json_length));
  std::unique_ptr<char, decltype(&free)> json_ptr(json, &free);
  if (json_ptr)
  {
    std::cerr << "[DIAG] Minimal test serialization succeeded! Length: " << json_length << "\n";
  }
  else
  {
    std::cerr << "[DIAG] Minimal test serialization FAILED!\n";
  }
  yyjson_mut_doc_free(doc);
}

auto validate_json_schema(const std::string& json_path, const std::string& schema_path,
                          bool is_diagnostics) -> bool
{
  using nlohmann::json;
  using nlohmann::json_schema::json_validator;
  // Use embedded schema
  const char* schema_cstr = nullptr;
  if (schema_path.find("result.schema.json") != std::string::npos) {
    schema_cstr = embedded_result_schema;
  } else if (schema_path.find("summary.schema.json") != std::string::npos) {
    schema_cstr = embedded_summary_schema;
  } else {
    if (is_diagnostics) {
      std::cerr << "[SCHEMA] Unknown schema requested: " << schema_path << "\n";
    }
    return false;
  }
  std::ifstream json_stream(json_path);
  if (!json_stream)
  {
    if (is_diagnostics)
    {
      std::cerr << "[SCHEMA] Could not open JSON file: " << json_path << "\n";
    }
    return false;
  }
  json schema_json = {};
  json data_json = {};
  json_validator validator = {};
  try
  {
    schema_json = json::parse(schema_cstr);
    json_stream >> data_json;
  }
  catch (const std::exception& exception)
  {
    if (is_diagnostics)
    {
      std::cerr << "[SCHEMA] Failed to parse JSON or schema: " << exception.what() << "\n";
    }
    return false;
  }
  try
  {
    validator.set_root_schema(schema_json);
  }
  catch (const std::exception& exception)
  {
    if (is_diagnostics)
    {
      std::cerr << "[SCHEMA] Invalid schema: " << exception.what() << "\n";
    }
    return false;
  }
  try
  {
    validator.validate(data_json);
    if (is_diagnostics)
    {
      std::cerr << "[SCHEMA] JSON is valid according to schema.\n";
    }
    return true;
  }
  catch (const std::exception& exception)
  {
    if (is_diagnostics)
    {
      std::cerr << "[SCHEMA] JSON validation failed: " << exception.what() << "\n";
    }
    return false;
  }
}
