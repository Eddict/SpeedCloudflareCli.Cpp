#include "output.h"
#include "chalk.h"
#include "json_helpers.h"
#include "stats.h"
#include "types.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <yyjson.h>

constexpr int kFieldWidthFile = 20;
constexpr int kFieldWidthCity = 15;
constexpr int kFieldWidthIP = 15;
constexpr int kFieldWidthLatency = 10;
constexpr int kFieldWidthJitter = 10;
constexpr int kFieldWidthDownload = 12;
constexpr int kFieldWidthUpload = 12;
constexpr int kFieldWidthDivider = 6;
constexpr int kSummaryDividerLen = kFieldWidthFile + kFieldWidthCity + kFieldWidthIP + kFieldWidthLatency + kFieldWidthJitter + kFieldWidthDownload + kFieldWidthUpload + kFieldWidthDivider;
constexpr int kLogInfoPad = 15;
constexpr int kLogSpeedPad = 9;
constexpr double kPercentile90 = 0.9;
constexpr int kPrintableAsciiMin = 32;
constexpr int kPrintableAsciiMax = 126;
constexpr int kHexDumpPreviewLen = 64;
constexpr int kHexDumpLastLen = 64;
constexpr int kAsciiDelete = 127;

// Modernized: braces, descriptive variable names, trailing return types, auto, nullptr, one declaration per statement, no implicit conversions

auto fmt(double value) -> std::string {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2) << value;
  return oss.str();
}

void log_info(const std::string &label, const std::string &data,
              bool output_json) {
  if (output_json) {
    return;
  }
  std::cout << chalk::bold(std::string(kLogInfoPad - label.length(), ' ') + label + ": " +
                           chalk::blue(data))
            << std::endl;
}

void log_latency(const std::vector<double> &latency_data, bool output_json) {
  if (output_json) {
    return;
  }
  std::cout << chalk::bold("     Latency: " +
                           chalk::magenta(fmt(latency_data[2]) + " ms"))
            << std::endl;
  std::cout << chalk::bold("     Jitter:  " +
                           chalk::magenta(fmt(latency_data[4]) + " ms"))
            << std::endl;
}

void log_speed_test_result(const std::string &size_label,
                           const std::vector<double> &test_results, bool output_json) {
  if (output_json) {
    return;
  }
  const double speed = test_results.empty() ? NAN : stats::median(test_results);
  std::cout << chalk::bold(std::string(kLogSpeedPad - size_label.length(), ' ') + size_label +
                           " speed: " + chalk::yellow(fmt(speed) + " Mbps"))
            << std::endl;
}

void log_download_speed(const std::vector<double> &download_tests, bool output_json) {
  if (output_json) {
    return;
  }
  std::cout << chalk::bold(
                   "  Download speed: " +
                   chalk::green(fmt(stats::quartile(download_tests, kPercentile90)) + " Mbps"))
            << std::endl;
}

void log_upload_speed(const std::vector<double> &upload_tests, bool output_json) {
  if (output_json) {
    return;
  }
  std::cout << chalk::bold(
                   "    Upload speed: " +
                   chalk::green(fmt(stats::quartile(upload_tests, kPercentile90)) + " Mbps"))
            << std::endl;
}

void print_summary_table(const std::vector<SummaryResult> &results) {
  if (results.empty()) {
    std::cout << "No results to display." << std::endl;
    return;
  }
  // Print header
  std::cout << std::left << std::setw(kFieldWidthFile) << "File"
            << std::setw(kFieldWidthCity) << "Server City"
            << std::setw(kFieldWidthIP) << "IP"
            << std::setw(kFieldWidthLatency) << "Latency"
            << std::setw(kFieldWidthJitter) << "Jitter"
            << std::setw(kFieldWidthDownload) << "Download"
            << std::setw(kFieldWidthUpload) << "Upload" << std::endl;
  std::cout << std::string(kSummaryDividerLen, '-') << std::endl;
  double sum_latency = 0.0, sum_jitter = 0.0, sum_download = 0.0, sum_upload = 0.0;
  for (const auto &r : results) {
    std::cout << std::left << std::setw(kFieldWidthFile) << r.file
              << std::setw(kFieldWidthCity) << r.server_city
              << std::setw(kFieldWidthIP) << r.ip
              << std::setw(kFieldWidthLatency) << r.latency
              << std::setw(kFieldWidthJitter) << r.jitter
              << std::setw(kFieldWidthDownload) << r.download
              << std::setw(kFieldWidthUpload) << r.upload << std::endl;
    sum_latency += r.latency;
    sum_jitter += r.jitter;
    sum_download += r.download;
    sum_upload += r.upload;
  }
  size_t n = results.size();
  if (n > 0) {
    std::cout << std::string(kSummaryDividerLen, '=') << std::endl;
    std::cout << std::left << std::setw(kFieldWidthFile) << "AVERAGE"
              << std::setw(kFieldWidthCity) << "-"
              << std::setw(kFieldWidthIP) << "-"
              << std::setw(kFieldWidthLatency) << (sum_latency / static_cast<double>(n))
              << std::setw(kFieldWidthJitter) << (sum_jitter / static_cast<double>(n))
              << std::setw(kFieldWidthDownload) << (sum_download / static_cast<double>(n))
              << std::setw(kFieldWidthUpload) << (sum_upload / static_cast<double>(n)) << std::endl;
  }
}

std::vector<SummaryResult> load_summary_results(const std::vector<std::string> &files,
                     bool diagnostics_mode, bool debug_mode) {
  std::vector<SummaryResult> results{};
  for (const auto &file : files) {
    std::ifstream in(file);
    if (!in) {
      if (debug_mode)
        std::clog << "[DEBUG] Skipping " << file << ": could not open file" << std::endl;
      continue;
    }
    std::string json((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
    size_t file_size = json.size();
    if (diagnostics_mode) {
      std::clog << "[DIAG] File: " << file << std::endl;
      std::clog << "[DIAG]   Size: " << file_size << " bytes" << std::endl;
      size_t print_len = std::min<size_t>(kHexDumpPreviewLen, file_size);
      std::clog << "[DIAG]   First " << print_len << " bytes (hex): ";
      for (size_t i = 0; i < print_len; ++i)
        std::clog << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (unsigned int)(unsigned char)json[i] << " ";
      std::clog << std::dec << std::endl;
      std::clog << "[DIAG]   First " << print_len << " bytes (text): ";
      for (size_t i = 0; i < print_len; ++i) {
        char c = json[i];
        if (c >= kPrintableAsciiMin && c <= kPrintableAsciiMax)
          std::clog << c;
        else
          std::clog << '.';
      }
      std::clog << std::endl;
      if (file_size > kHexDumpLastLen) {
        size_t start = file_size - kHexDumpLastLen;
        std::clog << "[DIAG]   Last 64 bytes (hex): ";
        for (size_t i = start; i < file_size; ++i)
          std::clog << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (unsigned int)(unsigned char)json[i] << " ";
        std::clog << std::dec << std::endl;
        std::clog << "[DIAG]   Last 64 bytes (text): ";
        for (size_t i = start; i < file_size; ++i) {
          char c = json[i];
          if (c >= kPrintableAsciiMin && c <= kPrintableAsciiMax)
            std::clog << c;
          else
            std::clog << '.';
        }
        std::clog << std::endl;
      }
      bool has_null = false, has_nonprint = false;
      for (size_t i = 0; i < file_size; ++i) {
        unsigned char c = json[i];
        if (c == 0)
          has_null = true;
        if ((c < kPrintableAsciiMin && c != '\n' && c != '\r' && c != '\t') || c == kAsciiDelete)
          has_nonprint = true;
      }
      if (has_null)
        std::clog << "[DIAG]   WARNING: File contains null (\\0) bytes!" << std::endl;
      if (has_nonprint)
        std::clog << "[DIAG]   WARNING: File contains suspicious "
                        "non-printable characters!" << std::endl;
    }
    yyjson_doc *doc = yyjson_read(json.c_str(), json.size(), 0);
    if (!doc) {
      if (debug_mode)
        std::clog << "[DEBUG] Skipping " << file << ": failed to parse JSON" << std::endl;
      continue;
    }
    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!yyjson_is_obj(root)) {
      if (debug_mode)
        std::clog << "[DEBUG] Skipping " << file << ": root is not a JSON object" << std::endl;
      yyjson_doc_free(doc);
      continue;
    }
    SummaryResult r;
    size_t pos = file.find_last_of("/\\");
    r.file = (pos == std::string::npos) ? file : file.substr(pos + 1);
    yyjson_val *v;
    v = yyjson_obj_get(root, "server_city");
    r.server_city =
        (v && yyjson_is_str(v) && yyjson_get_str(v)) ? yyjson_get_str(v) : "";
    v = yyjson_obj_get(root, "ip");
    r.ip =
        (v && yyjson_is_str(v) && yyjson_get_str(v)) ? yyjson_get_str(v) : "";
    v = yyjson_obj_get(root, "latency_avg");
    r.latency = v && yyjson_is_num(v) ? yyjson_get_real(v) : 0.0;
    v = yyjson_obj_get(root, "jitter");
    r.jitter = v && yyjson_is_num(v) ? yyjson_get_real(v) : 0.0;
    v = yyjson_obj_get(root, "download_90pct");
    r.download = v && yyjson_is_num(v) ? yyjson_get_real(v) : 0.0;
    v = yyjson_obj_get(root, "upload_90pct");
    r.upload = v && yyjson_is_num(v) ? yyjson_get_real(v) : 0.0;
    if (debug_mode)
      std::clog <<
              "[DEBUG] Loaded: " << r.file << " | server_city='" << r.server_city << "' ip='" << r.ip << "' latency=" << r.latency <<
              " jitter=" << r.jitter << " download=" << r.download << " upload=" << r.upload << std::endl;
    results.push_back(r);
    yyjson_doc_free(doc);
  }
  return results;
}

void write_summary_json(const std::vector<SummaryResult> &results,
                        const std::string &filename, bool diagnostics_mode,
                        bool debug_mode) {
  if (diagnostics_mode) {
    for (size_t n = 1; n <= results.size(); ++n) {
      yyjson_mut_doc *test_doc = yyjson_mut_doc_new(NULL);
      std::clog << "[DIAG] test_doc ptr: " << (void *)test_doc << std::endl;
      yyjson_mut_val *test_arr = yyjson_mut_arr(test_doc);
      std::clog << "[DIAG] test_arr ptr: " << (void *)test_arr << std::endl;
      std::regex label_re(R"(^(.*)\\.json$)");
      bool ok = true;
      std::vector<void *> obj_ptrs;
      for (size_t i = 0; i < n; ++i) {
        const auto &entry = results[i];
        std::string label = entry.file;
        std::smatch m;
        if (std::regex_search(entry.file, m, label_re) && m.size() > 1)
          label = m[1].str();
        struct FieldCheck {
          const char *name;
          const std::string &value;
        };
        std::vector<FieldCheck> fields = {{"file", entry.file},
                                          {"server_city", entry.server_city},
                                          {"ip", entry.ip}};
        for (const auto &f : fields) {
          bool valid = is_valid_utf8(f.value);
          std::clog <<
                  "[DIAG] Entry " << i << ", field '" << f.name << "' (file: " << entry.file.c_str() <<
                  "): " << (valid ? "valid UTF-8" : "INVALID UTF-8") <<
                  ", ptr: " << (const void *)f.value.c_str() << ", len: "
                  << f.value.size() << std::endl;
          std::clog << "[DIAG]   Value (hex): ";
          for (unsigned char c : f.value)
            std::clog << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << c;
          std::clog << std::dec << std::endl;
        }
        std::clog <<
                "[DIAG] Entry " << i << ", label (used in output): '" << label.c_str() << "', ptr: " << (const void *)label.c_str() << ", len: "
                << label.size() << std::endl;
        std::clog << "[DIAG]   Label (hex): ";
        for (unsigned char c : label)
          std::clog << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << c;
        std::clog << std::dec << std::endl;
        yyjson_mut_val *test_obj = yyjson_mut_obj(test_doc);
        std::clog << "[DIAG] test_obj[" << i << "] ptr: " << (void *)test_obj << std::endl;
        obj_ptrs.push_back((void *)test_obj);
        if (!test_obj) {
          ok = false;
          break;
        }
        auto safe = [](const std::string &s) -> const char * {
          return s.empty() ? "" : s.c_str();
        };
        add_str(test_doc, test_obj, "label", label);
        add_str(test_doc, test_obj, "server_city", entry.server_city);
        add_str(test_doc, test_obj, "ip", entry.ip);
        add_num(test_doc, test_obj, "latency", entry.latency);
        add_num(test_doc, test_obj, "jitter", entry.jitter);
        add_num(test_doc, test_obj, "download", entry.download);
        add_num(test_doc, test_obj, "upload", entry.upload);
        int arr_add_ret = yyjson_mut_arr_add_val(test_arr, test_obj);
        std::clog << "[DIAG] yyjson_mut_arr_add_val[" << i << "] ret: " << arr_add_ret << std::endl;
      }
      if (ok) {
        yyjson_mut_val *test_root = yyjson_mut_obj(test_doc);
        std::clog << "[DIAG] test_root ptr: " << (void *)test_root << std::endl;
        int obj_add_ret =
            yyjson_mut_obj_add_val(test_doc, test_root, "results", test_arr);
        std::clog << "[DIAG] yyjson_mut_obj_add_val (results) ret: " << obj_add_ret << std::endl;
        yyjson_mut_doc_set_root(test_doc, test_root);
        yyjson_write_err err;
        char *test_out = yyjson_mut_write_opts(test_doc, 0, NULL, NULL, &err);
        std::unique_ptr<char, decltype(&free)> test_out_ptr(test_out, &free);
        if (!test_out_ptr) {
          std::clog <<
                  "[DIAG] Serialization failed at n=" << n << " (label='" << results[n - 1].file.c_str() << "'): yyjson "
                  "error: " << err.msg << " (code " << err.code << ")" << std::endl;
          char *arr_json = yyjson_mut_val_write(test_arr, 0, NULL);
          std::unique_ptr<char, decltype(&free)> arr_json_ptr(arr_json, &free);
          if (arr_json_ptr) {
            std::clog <<
                    "[DIAG] Direct array serialization at n=" << n << " succeeded: " << arr_json_ptr.get() << std::endl;
          } else {
            std::clog <<
                    "[DIAG] Direct array serialization at n=" << n << " FAILED!" << std::endl;
          }
          yyjson_mut_doc *single_doc = yyjson_mut_doc_new(NULL);
          yyjson_mut_val *single_obj = yyjson_mut_obj(single_doc);
          std::clog << "[DIAG] single_doc ptr: " << (void *)single_doc << ", single_obj ptr: " << (void *)single_obj << std::endl;
          std::string label = results[n - 1].file;
          std::smatch m;
          if (std::regex_search(results[n - 1].file, m, label_re) &&
              m.size() > 1)
            label = m[1].str();
          auto safe = [](const std::string &s) -> const char * {
            return s.empty() ? "" : s.c_str();
          };
          add_str(single_doc, single_obj, "label", label);
          add_str(single_doc, single_obj, "server_city", results[n - 1].server_city);
          add_str(single_doc, single_obj, "ip", results[n - 1].ip);
          add_num(single_doc, single_obj, "latency", results[n - 1].latency);
          add_num(single_doc, single_obj, "jitter", results[n - 1].jitter);
          add_num(single_doc, single_obj, "download", results[n - 1].download);
          add_num(single_doc, single_obj, "upload", results[n - 1].upload);
          yyjson_mut_doc_set_root(single_doc, single_obj);
          char *single_json = yyjson_mut_write(single_doc, 0, NULL);
          std::unique_ptr<char, decltype(&free)> single_json_ptr(single_json, &free);
          if (single_json_ptr) {
            std::clog << "[DIAG] Problematic entry JSON: " << single_json_ptr.get() << std::endl;
          } else {
            std::clog << "[DIAG] Problematic entry could not be serialized individually!" << std::endl;
          }
          yyjson_mut_doc_free(single_doc);
          yyjson_mut_doc_free(test_doc);
          std::clog << "[DIAG] Created " << obj_ptrs.size() << " objects in this run" << std::endl;
          break;
        } else {
          // Serialization succeeded
        }
      }
      yyjson_mut_doc_free(test_doc);
    }
  }
  // Full serialization
  yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
  if (!doc) {
    if (diagnostics_mode || debug_mode)
      std::cerr << "[ERROR] Failed to create yyjson_mut_doc!" << std::endl;
    return;
  }
  yyjson_mut_val *arr = yyjson_mut_arr(doc);
  if (!arr) {
    if (diagnostics_mode || debug_mode)
      std::cerr << "[ERROR] Failed to create yyjson_mut_arr!" << std::endl;
    yyjson_mut_doc_free(doc);
    return;
  }
  std::regex label_re(R"(^(.*)\\.json$)");
  for (const auto &r : results) {
    yyjson_mut_val *obj = yyjson_mut_obj(doc);
    if (!obj) {
      if (diagnostics_mode || debug_mode)
        std::cerr << "[ERROR] Failed to create yyjson_mut_obj!" << std::endl;
      continue;
    }
    std::string label = r.file;
    std::smatch m;
    if (std::regex_search(r.file, m, label_re) && m.size() > 1)
      label = m[1].str();
    add_str(doc, obj, "label", label);
    add_str(doc, obj, "server_city", r.server_city);
    add_str(doc, obj, "ip", r.ip);
    add_num(doc, obj, "latency", r.latency);
    add_num(doc, obj, "jitter", r.jitter);
    add_num(doc, obj, "download", r.download);
    add_num(doc, obj, "upload", r.upload);
    yyjson_mut_arr_add_val(arr, obj);
  }
  yyjson_mut_val *root_obj = yyjson_mut_obj(doc);
  yyjson_mut_obj_add_val(doc, root_obj, "results", arr);
  yyjson_mut_doc_set_root(doc, root_obj);
  char *out_cstr = yyjson_mut_write(doc, 0, NULL);
  std::unique_ptr<char, decltype(&free)> out_cstr_ptr(out_cstr, &free);
  if (!out_cstr_ptr) {
    if (diagnostics_mode || debug_mode)
      std::cerr << "[ERROR] Failed to serialize JSON: yyjson_mut_write returned NULL!" << std::endl;
    yyjson_mut_doc_free(doc);
    return;
  }
  yyjson_mut_doc_free(doc);
  std::ofstream f(filename);
  if (!f) {
    if (diagnostics_mode || debug_mode)
      std::cerr << "[ERROR] Could not open " << filename << " for writing!" << std::endl;
    return;
  }
  f << out_cstr_ptr.get();
  // No manual free needed
}
