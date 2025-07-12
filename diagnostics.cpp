#include "diagnostics.h"
#include <yyjson.h>
#include <fstream>
#include <iostream>
#include <regex>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>
#include "json_helpers.h"
#include "types.h"

void yyjson_minimal_test(bool diagnostics_mode, bool debug_mode) {
    if (!(diagnostics_mode || debug_mode)) return;
    fprintf(stderr, "[DIAG] Running minimal yyjson test with 8 objects...\n");
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *arr = yyjson_mut_arr(doc);
    for (int i = 0; i < 8; ++i) {
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
    if (json) {
        fprintf(stderr, "[DIAG] Minimal test serialization succeeded! Length: %zu\n", len);
        free(json);
    } else {
        fprintf(stderr, "[DIAG] Minimal test serialization FAILED!\n");
    }
    yyjson_mut_doc_free(doc);
}

bool validate_json_schema(const std::string& json_path, const std::string& schema_path, bool diagnostics_mode) {
    using nlohmann::json;
    using nlohmann::json_schema::json_validator;
    std::ifstream schema_stream(schema_path);
    std::ifstream json_stream(json_path);
    if (!schema_stream) {
        if (diagnostics_mode)
            fprintf(stderr, "[SCHEMA] Could not open schema file: %s\n", schema_path.c_str());
        return false;
    }
    if (!json_stream) {
        if (diagnostics_mode)
            fprintf(stderr, "[SCHEMA] Could not open JSON file: %s\n", json_path.c_str());
        return false;
    }
    json schema_json;
    json data_json;
    try {
        schema_stream >> schema_json;
        json_stream >> data_json;
    } catch (const std::exception& e) {
        if (diagnostics_mode)
            fprintf(stderr, "[SCHEMA] Failed to parse JSON or schema: %s\n", e.what());
        return false;
    }
    json_validator validator;
    try {
        validator.set_root_schema(schema_json);
    } catch (const std::exception& e) {
        if (diagnostics_mode)
            fprintf(stderr, "[SCHEMA] Invalid schema: %s\n", e.what());
        return false;
    }
    try {
        validator.validate(data_json);
        if (diagnostics_mode)
            fprintf(stderr, "[SCHEMA] JSON is valid according to schema.\n");
        return true;
    } catch (const std::exception& e) {
        if (diagnostics_mode)
            fprintf(stderr, "[SCHEMA] JSON validation failed: %s\n", e.what());
        return false;
    }
}

// ...existing code for write_summary_json and load_summary_results from main.cpp...
// (Move both functions here, unchanged except for includes)
