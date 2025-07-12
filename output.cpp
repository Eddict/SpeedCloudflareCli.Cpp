#include "output.h"
#include "chalk.h"
#include "stats.h"
#include "json_helpers.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstdio>
#include <fstream>
#include <algorithm>
#include <regex>
#include <yyjson.h>
#include "types.h"

std::string fmt(double v) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << v;
    return oss.str();
}

void log_info(const std::string& text, const std::string& data, bool output_json) {
    if (output_json) return;
    std::cout << chalk::bold(std::string(15 - text.length(), ' ') + text + ": " + chalk::blue(data)) << std::endl;
}

void log_latency(const std::vector<double>& data, bool output_json) {
    if (output_json) return;
    std::cout << chalk::bold("     Latency: " + chalk::magenta(fmt(data[2]) + " ms")) << std::endl;
    std::cout << chalk::bold("     Jitter:  " + chalk::magenta(fmt(data[4]) + " ms")) << std::endl;
}

void log_speed_test_result(const std::string& size, const std::vector<double>& test, bool output_json) {
    if (output_json) return;
    double speed = stats::median(test);
    std::cout << chalk::bold(std::string(9 - size.length(), ' ') + size + " speed: " + chalk::yellow(fmt(speed) + " Mbps")) << std::endl;
}

void log_download_speed(const std::vector<double>& tests, bool output_json) {
    if (output_json) return;
    std::cout << chalk::bold("  Download speed: " + chalk::green(fmt(stats::quartile(tests, 0.9)) + " Mbps")) << std::endl;
}

void log_upload_speed(const std::vector<double>& tests, bool output_json) {
    if (output_json) return;
    std::cout << chalk::bold("    Upload speed: " + chalk::green(fmt(stats::quartile(tests, 0.9)) + " Mbps")) << std::endl;
}

void print_summary_table(const std::vector<SummaryResult>& results) {
    if (results.empty()) {
        printf("No results to display.\n");
        return;
    }
    // Print header
    printf("%-20s %-15s %-15s %10s %10s %12s %12s\n",
           "File", "Server City", "IP", "Latency", "Jitter", "Download", "Upload");
    printf("%s\n", std::string(20+15+15+10+10+12+12+6, '-').c_str());
    double sum_latency = 0, sum_jitter = 0, sum_download = 0, sum_upload = 0;
    for (const auto& r : results) {
        printf("%-20s %-15s %-15s %10.2f %10.2f %12.2f %12.2f\n",
               r.file.c_str(), r.server_city.c_str(), r.ip.c_str(),
               r.latency, r.jitter, r.download, r.upload);
        sum_latency += r.latency;
        sum_jitter += r.jitter;
        sum_download += r.download;
        sum_upload += r.upload;
    }
    // Print '=' divider before average row
    size_t n = results.size();
    if (n > 0) {
        printf("%s\n", std::string(20+15+15+10+10+12+12+6, '=').c_str());
        printf("%-20s %-15s %-15s %10.2f %10.2f %12.2f %12.2f\n",
               "AVERAGE", "-", "-",
               sum_latency / n, sum_jitter / n, sum_download / n, sum_upload / n);
    }
}

std::vector<SummaryResult> load_summary_results(const std::vector<std::string>& files, bool diagnostics_mode, bool debug_mode) {
    std::vector<SummaryResult> results;
    for (const auto& file : files) {
        std::ifstream in(file);
        if (!in) {
            if (debug_mode)
                fprintf(stderr, "[DEBUG] Skipping %s: could not open file\n", file.c_str());
            continue;
        }
        std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        size_t file_size = json.size();
        if (diagnostics_mode) {
            fprintf(stderr, "[DIAG] File: %s\n", file.c_str());
            fprintf(stderr, "[DIAG]   Size: %zu bytes\n", file_size);
            size_t print_len = std::min<size_t>(64, file_size);
            fprintf(stderr, "[DIAG]   First %zu bytes (hex): ", print_len);
            for (size_t i = 0; i < print_len; ++i) fprintf(stderr, "%02X ", (unsigned char)json[i]);
            fprintf(stderr, "\n[DIAG]   First %zu bytes (text): ", print_len);
            for (size_t i = 0; i < print_len; ++i) {
                char c = json[i];
                if (c >= 32 && c <= 126) fputc(c, stderr); else fputc('.', stderr);
            }
            fprintf(stderr, "\n");
            if (file_size > 64) {
                size_t start = file_size - 64;
                fprintf(stderr, "[DIAG]   Last 64 bytes (hex): ");
                for (size_t i = start; i < file_size; ++i) fprintf(stderr, "%02X ", (unsigned char)json[i]);
                fprintf(stderr, "\n[DIAG]   Last 64 bytes (text): ");
                for (size_t i = start; i < file_size; ++i) {
                    char c = json[i];
                    if (c >= 32 && c <= 126) fputc(c, stderr); else fputc('.', stderr);
                }
                fprintf(stderr, "\n");
            }
            bool has_null = false, has_nonprint = false;
            for (size_t i = 0; i < file_size; ++i) {
                unsigned char c = json[i];
                if (c == 0) has_null = true;
                if ((c < 32 && c != '\n' && c != '\r' && c != '\t') || c == 127) has_nonprint = true;
            }
            if (has_null) fprintf(stderr, "[DIAG]   WARNING: File contains null (\\0) bytes!\n");
            if (has_nonprint) fprintf(stderr, "[DIAG]   WARNING: File contains suspicious non-printable characters!\n");
        }
        yyjson_doc* doc = yyjson_read(json.c_str(), json.size(), 0);
        if (!doc) {
            if (debug_mode)
                fprintf(stderr, "[DEBUG] Skipping %s: failed to parse JSON\n", file.c_str());
            continue;
        }
        yyjson_val* root = yyjson_doc_get_root(doc);
        if (!yyjson_is_obj(root)) {
            if (debug_mode)
                fprintf(stderr, "[DEBUG] Skipping %s: root is not a JSON object\n", file.c_str());
            yyjson_doc_free(doc);
            continue;
        }
        SummaryResult r;
        size_t pos = file.find_last_of("/\\");
        r.file = (pos == std::string::npos) ? file : file.substr(pos + 1);
        yyjson_val* v;
        v = yyjson_obj_get(root, "server_city");
        r.server_city = (v && yyjson_is_str(v) && yyjson_get_str(v)) ? yyjson_get_str(v) : "";
        v = yyjson_obj_get(root, "ip");
        r.ip = (v && yyjson_is_str(v) && yyjson_get_str(v)) ? yyjson_get_str(v) : "";
        v = yyjson_obj_get(root, "latency_avg");
        r.latency = v && yyjson_is_num(v) ? yyjson_get_real(v) : 0.0;
        v = yyjson_obj_get(root, "jitter");
        r.jitter = v && yyjson_is_num(v) ? yyjson_get_real(v) : 0.0;
        v = yyjson_obj_get(root, "download_90pct");
        r.download = v && yyjson_is_num(v) ? yyjson_get_real(v) : 0.0;
        v = yyjson_obj_get(root, "upload_90pct");
        r.upload = v && yyjson_is_num(v) ? yyjson_get_real(v) : 0.0;
        if (debug_mode)
            fprintf(stderr, "[DEBUG] Loaded: %s | server_city='%s' ip='%s' latency=%.2f jitter=%.2f download=%.2f upload=%.2f\n",
                r.file.c_str(), r.server_city.c_str(), r.ip.c_str(), r.latency, r.jitter, r.download, r.upload);
        results.push_back(r);
        yyjson_doc_free(doc);
    }
    return results;
}

void write_summary_json(const std::vector<SummaryResult>& results, const std::string& filename, bool diagnostics_mode, bool debug_mode) {
    if (diagnostics_mode) {
        for (size_t n = 1; n <= results.size(); ++n) {
            yyjson_mut_doc* test_doc = yyjson_mut_doc_new(NULL);
            fprintf(stderr, "[DIAG] test_doc ptr: %p\n", (void*)test_doc);
            yyjson_mut_val* test_arr = yyjson_mut_arr(test_doc);
            fprintf(stderr, "[DIAG] test_arr ptr: %p\n", (void*)test_arr);
            std::regex label_re(R"(^(.*)\\.json$)");
            bool ok = true;
            std::vector<void*> obj_ptrs;
            for (size_t i = 0; i < n; ++i) {
                const auto& entry = results[i];
                std::string label = entry.file;
                std::smatch m;
                if (std::regex_search(entry.file, m, label_re) && m.size() > 1) label = m[1].str();
                struct FieldCheck { const char* name; const std::string& value; };
                std::vector<FieldCheck> fields = {
                    {"file", entry.file},
                    {"server_city", entry.server_city},
                    {"ip", entry.ip}
                };
                for (const auto& f : fields) {
                    bool valid = is_valid_utf8(f.value);
                    fprintf(stderr, "[DIAG] Entry %zu, field '%s' (file: %s): %s, ptr: %p, len: %zu\n", i, f.name, entry.file.c_str(), valid ? "valid UTF-8" : "INVALID UTF-8", (const void*)f.value.c_str(), f.value.size());
                    fprintf(stderr, "[DIAG]   Value (hex): ");
                    for (unsigned char c : f.value) fprintf(stderr, "%02X ", c);
                    fprintf(stderr, "\n");
                }
                fprintf(stderr, "[DIAG] Entry %zu, label (used in output): '%s', ptr: %p, len: %zu\n", i, label.c_str(), (const void*)label.c_str(), label.size());
                fprintf(stderr, "[DIAG]   Label (hex): ");
                for (unsigned char c : label) fprintf(stderr, "%02X ", c);
                fprintf(stderr, "\n");
                yyjson_mut_val* test_obj = yyjson_mut_obj(test_doc);
                fprintf(stderr, "[DIAG] test_obj[%zu] ptr: %p\n", i, (void*)test_obj);
                obj_ptrs.push_back((void*)test_obj);
                if (!test_obj) { ok = false; break; }
                auto safe = [](const std::string& s) -> const char* { return s.empty() ? "" : s.c_str(); };
                #undef ADD_STR
                #define ADD_STR(key, val) yyjson_mut_obj_add_strncpy(test_doc, test_obj, key, (val).data(), (val).size())
                #undef ADD_NUM
                #define ADD_NUM(key, val) do { \
                    double _v = (val); \
                    if (std::isnan(_v) || std::isinf(_v)) { _v = 0.0; } \
                    yyjson_mut_obj_add_real(test_doc, test_obj, key, _v); \
                } while(0)
                ADD_STR("label", label);
                ADD_STR("server_city", entry.server_city);
                ADD_STR("ip", entry.ip);
                ADD_NUM("latency", entry.latency);
                ADD_NUM("jitter", entry.jitter);
                ADD_NUM("download", entry.download);
                ADD_NUM("upload", entry.upload);
                int arr_add_ret = yyjson_mut_arr_add_val(test_arr, test_obj);
                fprintf(stderr, "[DIAG] yyjson_mut_arr_add_val[%zu] ret: %d\n", i, arr_add_ret);
            }
            if (ok) {
                yyjson_mut_val* test_root = yyjson_mut_obj(test_doc);
                fprintf(stderr, "[DIAG] test_root ptr: %p\n", (void*)test_root);
                int obj_add_ret = yyjson_mut_obj_add_val(test_doc, test_root, "results", test_arr);
                fprintf(stderr, "[DIAG] yyjson_mut_obj_add_val (results) ret: %d\n", obj_add_ret);
                yyjson_mut_doc_set_root(test_doc, test_root);
                yyjson_write_err err;
                char* test_out = yyjson_mut_write_opts(test_doc, 0, NULL, NULL, &err);
                if (!test_out) {
                    fprintf(stderr, "[DIAG] Serialization failed at n=%zu (label='%s'): yyjson error: %s (code %d)\n", n, results[n-1].file.c_str(), err.msg, err.code);
                    char* arr_json = yyjson_mut_val_write(test_arr, 0, NULL);
                    if (arr_json) {
                        fprintf(stderr, "[DIAG] Direct array serialization at n=%zu succeeded: %s\n", n, arr_json);
                        free(arr_json);
                    } else {
                        fprintf(stderr, "[DIAG] Direct array serialization at n=%zu FAILED!\n", n);
                    }
                    yyjson_mut_doc *single_doc = yyjson_mut_doc_new(NULL);
                    yyjson_mut_val *single_obj = yyjson_mut_obj(single_doc);
                    fprintf(stderr, "[DIAG] single_doc ptr: %p, single_obj ptr: %p\n", (void*)single_doc, (void*)single_obj);
                    std::string label = results[n-1].file;
                    std::smatch m;
                    if (std::regex_search(results[n-1].file, m, label_re) && m.size() > 1) label = m[1].str();
                    auto safe = [](const std::string& s) -> const char* { return s.empty() ? "" : s.c_str(); };
                    #undef ADD_STR
                    #define ADD_STR(key, val) yyjson_mut_obj_add_strncpy(single_doc, single_obj, key, (val).data(), (val).size())
                    #undef ADD_NUM
                    #define ADD_NUM(key, val) do { \
                        double _v = (val); \
                        if (std::isnan(_v) || std::isinf(_v)) { _v = 0.0; } \
                        yyjson_mut_obj_add_real(single_doc, single_obj, key, _v); \
                    } while(0)
                    ADD_STR("label", label);
                    ADD_STR("server_city", results[n-1].server_city);
                    ADD_STR("ip", results[n-1].ip);
                    ADD_NUM("latency", results[n-1].latency);
                    ADD_NUM("jitter", results[n-1].jitter);
                    ADD_NUM("download", results[n-1].download);
                    ADD_NUM("upload", results[n-1].upload);
                    yyjson_mut_doc_set_root(single_doc, single_obj);
                    char *single_json = yyjson_mut_write(single_doc, 0, NULL);
                    if (single_json) {
                        fprintf(stderr, "[DIAG] Problematic entry JSON: %s\n", single_json);
                        free(single_json);
                    } else {
                        fprintf(stderr, "[DIAG] Problematic entry could not be serialized individually!\n");
                    }
                    yyjson_mut_doc_free(single_doc);
                    yyjson_mut_doc_free(test_doc);
                    fprintf(stderr, "[DIAG] Created %zu objects in this run\n", obj_ptrs.size());
                    break;
                } else {
                    free(test_out);
                }
            }
            yyjson_mut_doc_free(test_doc);
        }
    }
    // Full serialization
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    if (!doc) { if (diagnostics_mode || debug_mode) fprintf(stderr, "[ERROR] Failed to create yyjson_mut_doc!\n"); return; }
    yyjson_mut_val* arr = yyjson_mut_arr(doc);
    if (!arr) { if (diagnostics_mode || debug_mode) fprintf(stderr, "[ERROR] Failed to create yyjson_mut_arr!\n"); yyjson_mut_doc_free(doc); return; }
    std::regex label_re(R"(^(.*)\\.json$)");
    for (const auto& r : results) {
        yyjson_mut_val* obj = yyjson_mut_obj(doc);
        if (!obj) { if (diagnostics_mode || debug_mode) fprintf(stderr, "[ERROR] Failed to create yyjson_mut_obj!\n"); continue; }
        std::string label = r.file;
        std::smatch m;
        if (std::regex_search(r.file, m, label_re) && m.size() > 1) label = m[1].str();
        #undef ADD_STR
        #define ADD_STR(key, val) yyjson_mut_obj_add_strncpy(doc, obj, key, (val).data(), (val).size())
        #undef ADD_NUM
        #define ADD_NUM(key, val) do { \
            double _v = (val); \
            if (std::isnan(_v) || std::isinf(_v)) { _v = 0.0; } \
            yyjson_mut_obj_add_real(doc, obj, key, _v); \
        } while(0)
        ADD_STR("label", label);
        ADD_STR("server_city", r.server_city);
        ADD_STR("ip", r.ip);
        ADD_NUM("latency", r.latency);
        ADD_NUM("jitter", r.jitter);
        ADD_NUM("download", r.download);
        ADD_NUM("upload", r.upload);
        yyjson_mut_arr_add_val(arr, obj);
    }
    yyjson_mut_val* root_obj = yyjson_mut_obj(doc);
    yyjson_mut_obj_add_val(doc, root_obj, "results", arr);
    yyjson_mut_doc_set_root(doc, root_obj);
    char* out_cstr = yyjson_mut_write(doc, 0, NULL);
    if (!out_cstr) {
        if (diagnostics_mode || debug_mode) fprintf(stderr, "[ERROR] Failed to serialize JSON: yyjson_mut_write returned NULL!\n");
        yyjson_mut_doc_free(doc);
        return;
    }
    yyjson_mut_doc_free(doc);
    std::ofstream f(filename);
    if (!f) {
        if (diagnostics_mode || debug_mode) fprintf(stderr, "[ERROR] Could not open %s for writing!\n", filename.c_str());
        free(out_cstr);
        return;
    }
    f << out_cstr;
    free(out_cstr);
}
