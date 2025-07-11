#pragma once
#include <string>
#include <vector>
#include <map>

// Struct to hold all results for JSON output
struct TestResults {
    std::string city, colo, ip, loc, host, arch, kernel, cpu_model, mem_total;
    std::string sysinfo_date;
    std::string version; // Build version string
    int cpu_cores = 0;
    std::vector<double> latency, download_100kB, download_1MB, download_10MB, download_25MB, download_100MB;
    std::vector<double> upload_11kB, upload_100kB, upload_1MB;
    std::vector<double> all_downloads, all_uploads;
    double total_time_ms = 0;
    std::vector<std::string> flags;
};

// HTTP request helpers
std::string http_get(const std::string& hostname, const std::string& path);
std::string http_post(const std::string& hostname, const std::string& path, const std::string& data);

// JSON parsing helpers
std::vector<std::map<std::string, std::string>> parse_locations_json(const std::string& json);
std::map<std::string, std::string> parse_cdn_trace(const std::string& text);

// Speed test helpers
std::vector<double> measure_latency();
std::vector<double> measure_download(int bytes, int iterations);
std::vector<double> measure_upload(int bytes, int iterations);
double measure_speed(int bytes, double duration_ms);

// Output helpers
void log_info(const std::string& text, const std::string& data, bool output_json = false);
void log_latency(const std::vector<double>& data, bool output_json = false);
void log_speed_test_result(const std::string& size, const std::vector<double>& test, bool output_json = false);
void log_download_speed(const std::vector<double>& tests, bool output_json = false);
void log_upload_speed(const std::vector<double>& tests, bool output_json = false);

// Main speed test
void speed_test(bool use_parallel = false, bool minimize_output = false, bool warmup = true, bool do_yield = true, bool mask_sensitive = false, bool output_json = false, TestResults* json_results = nullptr);

// System info and privacy helpers
void collect_sysinfo(TestResults& res, bool mask_sensitive);
std::string mask_str(const std::string& s);
