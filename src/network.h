#pragma once
#include <map>
#include <string>
#include <vector>
#include <stddef.h>       // for size_t

// HTTP request helpers
struct HttpRequest {
    std::string hostname;
    std::string path;
};
auto http_get(const HttpRequest& req) -> std::string;
auto http_post(const HttpRequest& req, const std::string& data) -> std::string;

// JSON parsing helpers
auto parse_locations_json(const std::string& json)
    -> std::vector<std::map<std::string, std::string>>;
auto parse_cdn_trace(const std::string& text) -> std::map<std::string, std::string>;
