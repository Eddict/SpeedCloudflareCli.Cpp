#include "network.h"
#include <httplib.h>
#include <stddef.h>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <yyjson.h>

// Modernized: braces, descriptive variable names, trailing return types, auto, nullptr, one
// declaration per statement, no implicit conversions

auto http_get(const std::string& hostname, const std::string& path) -> std::string
{
    httplib::SSLClient cli(hostname.c_str());
    auto res = cli.Get(path.c_str());
    if (res && res->status == 200) {
        return res->body;
    }
    return "";
}

auto http_get(const HttpRequest& req) -> std::string {
    return http_get(req.hostname, req.path);
}

auto http_post(const HttpRequest& req, const std::string& data) -> std::string
{
    httplib::SSLClient cli(req.hostname.c_str());
    std::string debug_info;
    auto res = cli.Post(req.path.c_str(), data, "application/json");
    if (res) {
        debug_info += "[UPLOAD] HTTP response code: " + std::to_string(res->status) + "\n";
        debug_info += "[UPLOAD] Response body size: " + std::to_string(res->body.size()) + "\n";
        debug_info += res->body;
    } else {
        debug_info += "[UPLOAD] HTTP request failed\n";
    }
    return debug_info;
}

auto parse_locations_json(const std::string& json)
    -> std::vector<std::map<std::string, std::string>>
{
  std::vector<std::map<std::string, std::string>> locations_result = {};
  yyjson_doc* json_doc = nullptr;
  yyjson_val* json_array = nullptr;
  yyjson_val* json_value = nullptr;
  json_doc = yyjson_read(json.c_str(), json.size(), 0);
  if (json_doc == nullptr)
  {
    return locations_result;
  }
  json_array = yyjson_doc_get_root(json_doc);
  if (!yyjson_is_arr(json_array))
  {
    yyjson_doc_free(json_doc);
    return locations_result;
  }
  size_t array_index = 0;
  size_t array_max = 0;
  yyjson_arr_foreach(json_array, array_index, array_max, json_value)
  {
    std::map<std::string, std::string> entry{};
    yyjson_val* iata_val = yyjson_obj_get(json_value, "iata");
    yyjson_val* city_val = yyjson_obj_get(json_value, "city");
    if (iata_val != nullptr && city_val != nullptr)
    {
      entry["iata"] = yyjson_get_str(iata_val);
      entry["city"] = yyjson_get_str(city_val);
      locations_result.push_back(entry);
    }
  }
  yyjson_doc_free(json_doc);
  return locations_result;
}

auto parse_cdn_trace(const std::string& text) -> std::map<std::string, std::string>
{
  std::map<std::string, std::string> trace_result = {};
  std::istringstream input_stream(text);
  std::string line{};
  while (std::getline(input_stream, line))
  {
    const auto eq_pos = line.find('=');
    if (eq_pos != std::string::npos)
    {
      trace_result[line.substr(0, eq_pos)] = line.substr(eq_pos + 1);
    }
  }
  return trace_result;
}
