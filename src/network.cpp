#include "network.h"
#include <curl/curl.h>
#include <stddef.h>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <yyjson.h>

// Modernized: braces, descriptive variable names, trailing return types, auto, nullptr, one
// declaration per statement, no implicit conversions

auto my_curl_write_callback(void* contents, size_t size, size_t nmemb, void* userp) -> size_t
{
  const size_t total_size = size * nmemb;
  auto* vec_ptr = static_cast<std::vector<char>*>(userp);
  vec_ptr->insert(vec_ptr->end(), static_cast<char*>(contents), static_cast<char*>(contents) + total_size);
  return total_size;
}

auto http_get(const std::string& hostname, const std::string& path, size_t expected_size = 0) -> std::string
{
  CURL* curl = curl_easy_init();
  std::vector<char> response_vec;
  if (expected_size > 0) {
    response_vec.reserve(expected_size);
  }
  if (curl != nullptr)
  {
    const std::string url = "https://" + hostname + path;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_vec);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    const CURLcode result = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (result != CURLE_OK)
    {
      return "";
    }
  }
  return std::string(response_vec.begin(), response_vec.end());
}

auto http_get(const HttpRequest& req, size_t expected_size = 0) -> std::string {
    return http_get(req.hostname, req.path, expected_size);
}

auto http_post(const HttpRequest& req, const std::string& data, size_t expected_size = 0) -> std::string
{
  CURL* curl = curl_easy_init();
  std::vector<char> response_vec;
  if (expected_size > 0) {
    response_vec.reserve(expected_size);
  }
  if (curl != nullptr)
  {
    const std::string url = "https://" + req.hostname + req.path;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_vec);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    const CURLcode result = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (result != CURLE_OK)
    {
      return "";
    }
  }
  return std::string(response_vec.begin(), response_vec.end());
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
