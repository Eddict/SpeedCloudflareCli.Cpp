#pragma once
#include <map>
#include <string>
#include <vector>

// HTTP request helpers
// Modernized: trailing return types, descriptive parameter names
auto http_get(const std::string& hostname, const std::string& path) -> std::string;
auto http_post(const std::string& hostname, const std::string& path, const std::string& data)
    -> std::string;

// JSON parsing helpers
auto parse_locations_json(const std::string& json)
    -> std::vector<std::map<std::string, std::string>>;
auto parse_cdn_trace(const std::string& text) -> std::map<std::string, std::string>;
