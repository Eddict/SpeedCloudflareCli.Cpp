#include "network.h"
#include <stddef.h>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <yyjson.h>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

// Modernized: braces, descriptive variable names, trailing return types, auto, nullptr, one
// declaration per statement, no implicit conversions

// Refactored HTTP GET using Boost.Beast
// Only accept HttpRequest struct to avoid swappable parameters
auto http_get(const HttpRequest& req) -> std::string
{
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace net = boost::asio;
    using tcp = net::ip::tcp;

    net::io_context ioc;
    net::ssl::context ctx(net::ssl::context::sslv23_client);
    tcp::resolver resolver(ioc);
    beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

    auto const results = resolver.resolve(req.hostname, "443");
    beast::get_lowest_layer(stream).connect(results);
    stream.handshake(net::ssl::stream_base::client);

    http::request<http::string_body> request{http::verb::get, req.path, 11};
    request.set(http::field::host, req.hostname);
    request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    http::write(stream, request);

    beast::flat_buffer buffer;
    http::response<http::string_body> response;
    http::read(stream, buffer, response);

    beast::error_code ec;
    stream.shutdown(ec);

    if (response.result() == http::status::ok) {
        return response.body();
    }
    return "";
}

// Refactored HTTP POST using Boost.Beast
auto http_post(const HttpRequest& req, const std::string& data) -> std::string
{
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace net = boost::asio;
    using tcp = net::ip::tcp;

    net::io_context ioc;
    net::ssl::context ctx(net::ssl::context::sslv23_client);
    tcp::resolver resolver(ioc);
    beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

    auto const results = resolver.resolve(req.hostname, "443");
    beast::get_lowest_layer(stream).connect(results);
    stream.handshake(net::ssl::stream_base::client);

    http::request<http::string_body> request{http::verb::post, req.path, 11};
    request.set(http::field::host, req.hostname);
    request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    request.set(http::field::content_type, "application/json");
    request.body() = data;
    request.prepare_payload();

    http::write(stream, request);

    beast::flat_buffer buffer;
    http::response<http::string_body> response;
    http::read(stream, buffer, response);

    beast::error_code ec;
    stream.shutdown(ec);

    std::string debug_info;
    debug_info += "[UPLOAD] HTTP response code: " + std::to_string(response.result_int()) + "\n";
    debug_info += "[UPLOAD] Response body size: " + std::to_string(response.body().size()) + "\n";
    debug_info += response.body();
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
