#include "json_helpers.h"
#include "types.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <vector>
#include <yyjson.h>

void add_str(yyjson_mut_doc *doc, yyjson_mut_val *obj, const char *key, const std::string &val, const std::function<const char*(const std::string&)>& safe) {
    yyjson_mut_obj_add_str(doc, obj, key, safe(val));
}
void add_num(yyjson_mut_doc *doc, yyjson_mut_val *obj, const char *key, double val) {
    yyjson_mut_obj_add_real(doc, obj, key, val);
}

// Overload for compatibility with output.cpp usage
void add_str(yyjson_mut_doc *doc, yyjson_mut_val *obj, const char *key, const std::string &val) {
    auto safe = [](const std::string &s) -> const char * {
        return s.empty() ? "" : s.c_str();
    };
    yyjson_mut_obj_add_str(doc, obj, key, safe(val));
}

std::string serialize_to_json(const TestResults &res) {
  auto safe = [](const std::string &s) -> const char * {
    return s.empty() ? "" : s.c_str();
  };
  yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
  yyjson_mut_val *obj = yyjson_mut_obj(doc);
  yyjson_mut_doc_set_root(doc, obj);
  add_str(doc, obj, "sysinfo_date", res.sysinfo_date, safe);
  add_str(doc, obj, "server_city", res.city, safe);
  add_str(doc, obj, "colo", res.colo, safe);
  add_str(doc, obj, "ip", res.ip, safe);
  add_str(doc, obj, "loc", res.loc, safe);
  add_str(doc, obj, "host", res.host, safe);
  add_str(doc, obj, "arch", res.arch, safe);
  add_str(doc, obj, "kernel", res.kernel, safe);
  add_str(doc, obj, "cpu_model", res.cpu_model, safe);
  add_str(doc, obj, "mem_total", res.mem_total, safe);
  add_str(doc, obj, "version", res.version, safe);
  yyjson_mut_obj_add_int(doc, obj, "cpu_cores", res.cpu_cores);
  auto add_vec = [&](const char *key, const std::vector<double> &v) {
    yyjson_mut_val *arr = yyjson_mut_arr(doc);
    for (double d : v)
      yyjson_mut_arr_add_real(doc, arr, d);
    yyjson_mut_obj_add_val(doc, obj, key, arr);
  };
  add_vec("latency", res.latency);
  add_vec("download_100kB", res.download_100kB);
  add_vec("download_1MB", res.download_1MB);
  add_vec("download_10MB", res.download_10MB);
  add_vec("download_25MB", res.download_25MB);
  add_vec("download_100MB", res.download_100MB);
  add_vec("all_downloads", res.all_downloads);
  add_vec("upload_11kB", res.upload_11kB);
  add_vec("upload_100kB", res.upload_100kB);
  add_vec("upload_1MB", res.upload_1MB);
  add_vec("all_uploads", res.all_uploads);
  add_num(doc, obj, "total_time_ms", res.total_time_ms);
  add_num(doc, obj, "latency_avg", res.latency.size() > 2 ? res.latency[2] : 0.0);
  add_num(doc, obj, "jitter", res.latency.size() > 4 ? res.latency[4] : 0.0);
  add_num(doc, obj, "download_90pct", percentile(res.all_downloads, 0.9));
  add_num(doc, obj, "upload_90pct", percentile(res.all_uploads, 0.9));
  yyjson_mut_val *flags_arr = yyjson_mut_arr(doc);
  for (const auto &f : res.flags)
    yyjson_mut_arr_add_str(doc, flags_arr, f.c_str());
  yyjson_mut_obj_add_val(doc, obj, "flags", flags_arr);
  std::string out = yyjson_mut_write(doc, 0, NULL);
  yyjson_mut_doc_free(doc);
  return out;
}

double percentile(const std::vector<double> &v, double pct) {
  if (v.empty())
    return 0.0;
  std::vector<double> sorted = v;
  std::sort(sorted.begin(), sorted.end());
  size_t idx = static_cast<size_t>(std::ceil(pct * sorted.size())) - 1;
  if (idx >= sorted.size())
    idx = sorted.size() - 1;
  return sorted[idx];
}

bool is_valid_utf8(const std::string &str) {
  const unsigned char* bytes = reinterpret_cast<const unsigned char*>(str.data());
  size_t len = str.size();
  const int MAX_ASCII_CODE_POINT = 0x7F;
  const int CONT_BYTE_MASK = 0xC0;
  const int CONT_BYTE_PATTERN = 0x80;
  const int TWO_BYTE_MASK = 0xE0;
  const int TWO_BYTE_PATTERN = 0xC0;
  const int THREE_BYTE_MASK = 0xF0;
  const int THREE_BYTE_PATTERN = 0xE0;
  const int FOUR_BYTE_MASK = 0xF8;
  const int FOUR_BYTE_PATTERN = 0xF0;
  size_t i = 0;
  while (i < len) {
    if (bytes[i] <= MAX_ASCII_CODE_POINT) {
      i++;
      continue;
    }
    if ((bytes[i] & TWO_BYTE_MASK) == TWO_BYTE_PATTERN) {
      if (i + 1 >= len || (bytes[i + 1] & CONT_BYTE_MASK) != CONT_BYTE_PATTERN)
        return false;
      i += 2;
      continue;
    }
    if ((bytes[i] & THREE_BYTE_MASK) == THREE_BYTE_PATTERN) {
      if (i + 2 >= len || (bytes[i + 1] & CONT_BYTE_MASK) != CONT_BYTE_PATTERN ||
          (bytes[i + 2] & CONT_BYTE_MASK) != CONT_BYTE_PATTERN)
        return false;
      i += 3;
      continue;
    }
    if ((bytes[i] & FOUR_BYTE_MASK) == FOUR_BYTE_PATTERN) {
      if ((i + 3 >= len) || (bytes[i + 1] & CONT_BYTE_MASK) != CONT_BYTE_PATTERN ||
          (bytes[i + 2] & CONT_BYTE_MASK) != CONT_BYTE_PATTERN || (bytes[i + 3] & CONT_BYTE_MASK) != CONT_BYTE_PATTERN)
        return false;
      i += 4;
      continue;
    }
    return false;
  }
  return true;
}
