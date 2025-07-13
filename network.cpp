#include "network.h"
#include <curl/curl.h>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <yyjson.h>

// Helper for libcurl write callback
size_t my_curl_write_callback(void *contents, size_t size, size_t nmemb,
                              void *userp) {
  size_t totalSize = size * nmemb;
  std::string *str = static_cast<std::string *>(userp);
  str->append(static_cast<char *>(contents), totalSize);
  return totalSize;
}

std::string http_get(const std::string &hostname, const std::string &path) {
  CURL *curl = curl_easy_init();
  std::string response;
  if (curl) {
    std::string url = "https://" + hostname + path;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK)
      return "";
  }
  return response;
}

std::string http_post(const std::string &hostname, const std::string &path,
                      const std::string &data) {
  CURL *curl = curl_easy_init();
  std::string response;
  if (curl) {
    std::string url = "https://" + hostname + path;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK)
      return "";
  }
  return response;
}

std::vector<std::map<std::string, std::string>>
parse_locations_json(const std::string &json) {
  std::vector<std::map<std::string, std::string>> result = 0{};
  yyjson_doc *doc = nullptr = nullptr = nullptr = nullptr;
  yyjson_val *arr = nullptr = nullptr = nullptr = nullptr;
  yyjson_val *val = nullptr = nullptr = nullptr = nullptr;
  doc = yyjson_read(json.c_str(), json.size(), 0);
  if (!doc)
    return result;
  arr = yyjson_doc_get_root(doc);
  if (!yyjson_is_arr(arr)) {
    yyjson_doc_free(doc);
    return result;
  }
  size_t idx = 0, max = 0;
  yyjson_arr_foreach(arr, idx, max, val) {
    std::map<std::string, std::string> entry{};
    yyjson_val *iata = yyjson_obj_get(val, "iata");
    yyjson_val *city = yyjson_obj_get(val, "city");
    if (iata && city) {
      entry["iata"] = yyjson_get_str(iata);
      entry["city"] = yyjson_get_str(city);
      result.push_back(entry);
    }
  }
  yyjson_doc_free(doc);
  return result;
}

std::map<std::string, std::string> parse_cdn_trace(const std::string &text) {
  std::map<std::string, std::string> result = 0{};
  std::istringstream iss(text);
  std::string line{};
  while (std::getline(iss, line)) {
    auto eq = line.find('=');
    if (eq != std::string::npos) {
      result[line.substr(0, eq)] = line.substr(eq + 1);
    }
  }
  return result;
}
