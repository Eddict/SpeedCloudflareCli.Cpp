#pragma once
#include <string>
#include <vector>
#include <map>

// HTTP request helpers
std::string http_get(const std::string& hostname, const std::string& path);
std::string http_post(const std::string& hostname, const std::string& path, const std::string& data);

// JSON parsing helpers
std::vector<std::map<std::string, std::string>> parse_locations_json(const std::string& json);
std::map<std::string, std::string> parse_cdn_trace(const std::string& text);
