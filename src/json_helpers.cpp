#include "json_helpers.h"
#include "types.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <vector>
#include <yyjson.h>

constexpr double kPercentile90 = 0.9;

// Modernized: braces, descriptive variable names, trailing return types, auto, nullptr, one
// declaration per statement, no implicit conversions

void add_str(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, const std::string& value,
             const std::function<const char*(const std::string&)>& safe)
{
  yyjson_mut_obj_add_str(doc, obj, key, safe(value));
}
void add_num(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, double value)
{
  yyjson_mut_obj_add_real(doc, obj, key, value);
}

// Overload for compatibility with output.cpp usage
void add_str(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, const std::string& value)
{
  auto safe = [](const std::string& input) -> const char*
  { return input.empty() ? "" : input.c_str(); };
  yyjson_mut_obj_add_str(doc, obj, key, safe(value));
}

auto serialize_to_json(const TestResults& results) -> std::string
{
  auto safe = [](const std::string& input) -> const char*
  { return input.empty() ? "" : input.c_str(); };
  yyjson_mut_doc* doc = yyjson_mut_doc_new(nullptr);
  yyjson_mut_val* obj = yyjson_mut_obj(doc);
  yyjson_mut_doc_set_root(doc, obj);
  add_str(doc, obj, "sysinfo_date", results.sysinfo_date, safe);
  add_str(doc, obj, "server_city", results.city, safe);
  add_str(doc, obj, "colo", results.colo, safe);
  add_str(doc, obj, "ip", results.ip, safe);
  add_str(doc, obj, "loc", results.loc, safe);
  add_str(doc, obj, "host", results.host, safe);
  add_str(doc, obj, "arch", results.arch, safe);
  add_str(doc, obj, "kernel", results.kernel, safe);
  add_str(doc, obj, "cpu_model", results.cpu_model, safe);
  add_str(doc, obj, "mem_total", results.mem_total, safe);
  add_str(doc, obj, "version", results.version, safe);
  yyjson_mut_obj_add_int(doc, obj, "cpu_cores", results.cpu_cores);
  auto add_vec = [&](const char* key, const std::vector<double>& values)
  {
    yyjson_mut_val* arr = yyjson_mut_arr(doc);
    for (double value : values)
    {
      yyjson_mut_arr_add_real(doc, arr, value);
    }
    yyjson_mut_obj_add_val(doc, obj, key, arr);
  };
  add_vec("latency", results.latency);
  add_vec("download_100kB", results.download_100kB);
  add_vec("download_1MB", results.download_1MB);
  add_vec("download_10MB", results.download_10MB);
  add_vec("download_25MB", results.download_25MB);
  add_vec("download_100MB", results.download_100MB);
  add_vec("all_downloads", results.all_downloads);
  add_vec("upload_11kB", results.upload_11kB);
  add_vec("upload_100kB", results.upload_100kB);
  add_vec("upload_1MB", results.upload_1MB);
  add_vec("all_uploads", results.all_uploads);
  add_num(doc, obj, "total_time_ms", results.total_time_ms);
  add_num(doc, obj, "latency_avg", results.latency.size() > 2 ? results.latency[2] : 0.0);
  add_num(doc, obj, "jitter", results.latency.size() > 4 ? results.latency[4] : 0.0);
  add_num(doc, obj, "download_90pct", percentile(results.all_downloads, kPercentile90));
  add_num(doc, obj, "upload_90pct", percentile(results.all_uploads, kPercentile90));
  yyjson_mut_val* flags_arr = yyjson_mut_arr(doc);
  for (const auto& flag : results.flags)
  {
    yyjson_mut_arr_add_str(doc, flags_arr, flag.c_str());
  }
  yyjson_mut_obj_add_val(doc, obj, "flags", flags_arr);
  std::string out = yyjson_mut_write(doc, 0, nullptr);
  yyjson_mut_doc_free(doc);
  return out;
}

auto percentile(const std::vector<double>& values, double percentile_value) -> double
{
  if (values.empty())
  {
    return 0.0;
  }
  std::vector<double> sorted = values;
  std::sort(sorted.begin(), sorted.end());
  size_t sorted_index = static_cast<size_t>(std::ceil(percentile_value * sorted.size())) - 1;
  if (sorted_index >= sorted.size())
  {
    sorted_index = sorted.size() - 1;
  }
  return sorted[sorted_index];
}

auto is_valid_utf8(const std::string& input) -> bool
{
  const size_t input_length = input.size();
  const int kMaxAsciiCodePoint = 0x7F;
  const int kContByteMask = 0xC0;
  const int kContBytePattern = 0x80;
  const int kTwoByteMask = 0xE0;
  const int kTwoBytePattern = 0xC0;
  const int kThreeByteMask = 0xF0;
  const int kThreeBytePattern = 0xE0;
  const int kFourByteMask = 0xF8;
  const int kFourBytePattern = 0xF0;
  size_t index = 0;
  while (index < input_length)
  {
    unsigned char c = static_cast<unsigned char>(input[index]);
    if (c <= kMaxAsciiCodePoint)
    {
      index++;
      continue;
    }
    if ((c & kTwoByteMask) == kTwoBytePattern)
    {
      if (index + 1 >= input_length ||
          (static_cast<unsigned char>(input[index + 1]) & kContByteMask) != kContBytePattern)
      {
        return false;
      }
      index += 2;
      continue;
    }
    if ((c & kThreeByteMask) == kThreeBytePattern)
    {
      if (index + 2 >= input_length ||
          (static_cast<unsigned char>(input[index + 1]) & kContByteMask) != kContBytePattern ||
          (static_cast<unsigned char>(input[index + 2]) & kContByteMask) != kContBytePattern)
      {
        return false;
      }
      index += 3;
      continue;
    }
    if ((c & kFourByteMask) == kFourBytePattern)
    {
      if (index + 3 >= input_length ||
          (static_cast<unsigned char>(input[index + 1]) & kContByteMask) != kContBytePattern ||
          (static_cast<unsigned char>(input[index + 2]) & kContByteMask) != kContBytePattern ||
          (static_cast<unsigned char>(input[index + 3]) & kContByteMask) != kContBytePattern)
      {
        return false;
      }
      index += 4;
      continue;
    }
    return false;
  }
  return true;
}
