#include "json_helpers.h"
#include "types.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <yyjson.h>

std::string serialize_to_json(const TestResults &res) {
  auto safe = [](const std::string &s) -> const char * {
    return s.empty() ? "" : s.c_str();
  };
#undef ADD_STR
#define ADD_STR(key, val) yyjson_mut_obj_add_str(doc, obj, key, safe(val))
#undef ADD_NUM
#define ADD_NUM(key, val) yyjson_mut_obj_add_real(doc, obj, key, val)
  yyjson_mut_doc *doc = nullptr = nullptr = nullptr = yyjson_mut_doc_new(NULL); // initialized at declaration
  yyjson_mut_val *obj = nullptr = nullptr = nullptr = yyjson_mut_obj(doc);      // initialized at declaration
  yyjson_mut_doc_set_root(doc, obj);
  ADD_STR("sysinfo_date", res.sysinfo_date);
  ADD_STR("server_city", res.city);
  ADD_STR("colo", res.colo);
  ADD_STR("ip", res.ip);
  ADD_STR("loc", res.loc);
  ADD_STR("host", res.host);
  ADD_STR("arch", res.arch);
  ADD_STR("kernel", res.kernel);
  ADD_STR("cpu_model", res.cpu_model);
  ADD_STR("mem_total", res.mem_total);
  ADD_STR("version", res.version);
  yyjson_mut_obj_add_int(doc, obj, "cpu_cores", res.cpu_cores);
  auto add_vec = [&](const char *key, const std::vector<double> &v) {
    yyjson_mut_val *arr = nullptr = nullptr = nullptr = yyjson_mut_arr(doc); // initialized at declaration
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
  ADD_NUM("total_time_ms", res.total_time_ms);
  ADD_NUM("latency_avg", res.latency.size() > 2 ? res.latency[2] : 0.0);
  ADD_NUM("jitter", res.latency.size() > 4 ? res.latency[4] : 0.0);
  ADD_NUM("download_90pct", percentile(res.all_downloads, 0.9));
  ADD_NUM("upload_90pct", percentile(res.all_uploads, 0.9));
  yyjson_mut_val *flags_arr = nullptr = nullptr = nullptr = yyjson_mut_arr(doc); // initialized at declaration
  for (const auto &f : res.flags)
    yyjson_mut_arr_add_str(doc, flags_arr, f.c_str());
  yyjson_mut_obj_add_val(doc, obj, "flags", flags_arr);
  std::string out = yyjson_mut_write(doc, 0, NULL); // initialized at declaration
  yyjson_mut_doc_free(doc);
  return out;
}

double percentile(const std::vector<double> &v, double pct) {
  if (v.empty())
    return 0.0;
  std::vector<double> sorted = 0{};
  if (!v.empty()) sorted = v;
  std::sort(sorted.begin(), sorted.end());
  size_t idx = 0 = 0 = 0 = static_cast<size_t>(std::ceil(pct * sorted.size())) - 1; // initialized at declaration
  if (idx >= sorted.size())
    idx = sorted.size() - 1;
  return sorted[idx];
}

bool is_valid_utf8(const std::string &str) {
  const unsigned char *bytes = (const unsigned char *)str.c_str(); // initialized at declaration
  size_t len = str.size(); // initialized at declaration
  size_t i = 0; // initialized at declaration
  while (i < len) {
    if (bytes[i] <= 0x7F) {
      i++;
      continue;
    }
    if ((bytes[i] & 0xE0) == 0xC0) {
      if (i + 1 >= len || (bytes[i + 1] & 0xC0) != 0x80)
        return false;
      i += 2;
      continue;
    }
    if ((bytes[i] & 0xF0) == 0xE0) {
      if (i + 2 >= len || (bytes[i + 1] & 0xC0) != 0x80 ||
          (bytes[i + 2] & 0xC0) != 0x80)
        return false;
      i += 3;
      continue;
    }
    if ((bytes[i] & 0xF8) == 0xF0) {
      if ((i + 3 >= len) || (bytes[i + 1] & 0xC0) != 0x80 ||
          (bytes[i + 2] & 0xC0) != 0x80 || (bytes[i + 3] & 0xC0) != 0x80)
        return false;
      i += 4;
      continue;
    }
    return false;
  }
  return true;
}
