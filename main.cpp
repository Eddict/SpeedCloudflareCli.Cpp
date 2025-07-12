#include "main.h"
#include "chalk.h"
#include "stats.h"
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <curl/curl.h>
#define YYJSON_WRITE_MAX_DEPTH 32
#include <yyjson.h>
#include <algorithm>
#include <sys/resource.h>
#include <time.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <fstream>
#include <dirent.h>
#include <fnmatch.h>
#include <cmath>
#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>

// Version macro: build date and time (format: "Jul 11 2025 03:23:25")
const char* BUILD_VERSION = __DATE__ " " __TIME__;

// Constant for summary JSON filename (filename only)
const char* SUMMARY_JSON_FILENAME = "summary.json";

// --- HTTP helpers (very basic, no SSL) ---
// For real HTTPS, use a library like libcurl or Boost.Beast. Here, we use placeholders.

// Helper for libcurl write callback
size_t my_curl_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

std::string http_get(const std::string& hostname, const std::string& path) {
    CURL* curl = curl_easy_init();
    std::string response;
    if (curl) {
        std::string url = "https://" + hostname + path;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_curl_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if (res != CURLE_OK) return "";
    }
    return response;
}

std::string http_post(const std::string& hostname, const std::string& path, const std::string& data) {
    CURL* curl = curl_easy_init();
    std::string response;
    if (curl) {
        std::string url = "https://" + hostname + path;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_curl_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if (res != CURLE_OK) return "";
    }
    return response;
}

// --- JSON parsing helpers (very basic, for demo) ---
std::vector<std::map<std::string, std::string>> parse_locations_json(const std::string& json) {
    std::vector<std::map<std::string, std::string>> result;
    yyjson_doc* doc = yyjson_read(json.c_str(), json.size(), 0);
    if (!doc) return result;
    yyjson_val* arr = yyjson_doc_get_root(doc);
    if (!yyjson_is_arr(arr)) {
        yyjson_doc_free(doc);
        return result;
    }
    size_t idx, max;
    yyjson_val* val;
    yyjson_arr_foreach(arr, idx, max, val) {
        std::map<std::string, std::string> entry;
        yyjson_val* iata = yyjson_obj_get(val, "iata");
        yyjson_val* city = yyjson_obj_get(val, "city");
        if (iata && city) {
            entry["iata"] = yyjson_get_str(iata);
            entry["city"] = yyjson_get_str(city);
            result.push_back(entry);
        }
    }
    yyjson_doc_free(doc);
    return result;
}

std::map<std::string, std::string> parse_cdn_trace(const std::string& text) {
    std::map<std::string, std::string> result;
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
        auto eq = line.find('=');
        if (eq != std::string::npos) {
            result[line.substr(0, eq)] = line.substr(eq + 1);
        }
    }
    return result;
}

// --- Speed test helpers (real implementation) ---
#include <chrono>
#include <future>

std::vector<double> measure_latency() {
    std::vector<double> measurements;
    // Preallocate to avoid repeated allocations
    measurements.reserve(20);
    for (int i = 0; i < 20; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        std::string resp = http_get("speed.cloudflare.com", "/__down?bytes=1000");
        auto end = std::chrono::high_resolution_clock::now();
        if (!resp.empty()) {
            double ms = std::chrono::duration<double, std::milli>(end - start).count();
            measurements.push_back(ms);
        }
    }
    if (measurements.empty()) return std::vector<double>{0,0,0,0,0};
    std::vector<double> result;
    result.push_back(*std::min_element(measurements.begin(), measurements.end()));
    result.push_back(*std::max_element(measurements.begin(), measurements.end()));
    result.push_back(stats::average(measurements));
    result.push_back(stats::median(measurements));
    result.push_back(stats::jitter(measurements));
    return result;
}

// Sequential download for single-core systems
std::vector<double> measure_download(int bytes, int iterations) {
    std::vector<double> results;
    results.reserve(iterations);
    for (int i = 0; i < iterations; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        std::string resp = http_get("speed.cloudflare.com", "/__down?bytes=" + std::to_string(bytes));
        auto end = std::chrono::high_resolution_clock::now();
        if (!resp.empty()) {
            double ms = std::chrono::duration<double, std::milli>(end - start).count();
            results.push_back(measure_speed(bytes, ms));
        }
    }
    return results;
}

// Reuse buffer for uploads to minimize allocations
std::vector<double> measure_upload(int bytes, int iterations) {
    std::vector<double> results;
    results.reserve(iterations);
    std::string data(bytes, '0'); // Allocate once
    for (int i = 0; i < iterations; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        std::string resp = http_post("speed.cloudflare.com", "/__up", data);
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        results.push_back(measure_speed(bytes, ms));
    }
    return results;
}

// Parallel download/upload tests for multi-core systems
std::vector<double> measure_download_parallel(int bytes, int iterations) {
    std::vector<std::future<double>> futures;
    for (int i = 0; i < iterations; ++i) {
        futures.push_back(std::async(std::launch::async, [bytes]() {
            auto start = std::chrono::high_resolution_clock::now();
            std::string resp = http_get("speed.cloudflare.com", "/__down?bytes=" + std::to_string(bytes));
            auto end = std::chrono::high_resolution_clock::now();
            if (!resp.empty()) {
                double ms = std::chrono::duration<double, std::milli>(end - start).count();
                return measure_speed(bytes, ms);
            }
            return 0.0;
        }));
    }
    std::vector<double> results;
    results.reserve(iterations);
    for (auto& f : futures) results.push_back(f.get());
    return results;
}

double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

void pin_to_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    sched_setaffinity(0, sizeof(cpuset), &cpuset);
}

void drop_caches() {
    FILE* f = fopen("/proc/sys/vm/drop_caches", "w");
    if (f) {
        fputs("3\n", f);
        fclose(f);
    }
}

void set_nice() {
    setpriority(PRIO_PROCESS, 0, 10);
}

void print_help() {
    printf("Usage: SpeedCloudflareCli [options]\n");
    printf("  --parallel, -p           Use parallel download tests (default: off)\n");
    printf("  --minimize-output, -m    Minimize output/logging (default: off)\n");
    printf("  --no-warmup              Disable network warm-up phase (default: on)\n");
    printf("  --single-core, -s        Pin process to a single core (default: off)\n");
    printf("  --no-yield               Do not yield (sleep) between test iterations (default: yield on)\n");
    printf("  --no-nice                Do not lower process priority (default: nice on)\n");
    printf("  --drop-caches            Drop Linux FS caches before test (default: off, root only)\n");
    printf("  --mask-sensitive         Mask the last part of your IP address and hostname in output (default: off)\n");
    printf("  --show-flags-used        Print a line at the end with all explicitly set flags (default: off)\n");
    printf("  --show-sysinfo           Show basic host architecture, CPU, and memory info (default: off)\n");
    printf("  --show-sysinfo-only      Only print system info and exit (supports --mask-sensitive)\n");
    printf("  --json                   Output results as JSON to stdout (default: off)\n");
    printf("  --summary-table FILES    Print a summary table comparing multiple JSON result files\n");
    printf("  --debug                  Show debug output (default: off)\n");
    printf("  --help, -h               Show this help message\n");
}

void yield_cpu() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void speed_test(bool use_parallel, bool minimize_output, bool warmup, bool do_yield, bool mask_sensitive, bool output_json, TestResults* json_results) {
    double t0 = get_time_ms();
    if (warmup) {
        for (int i = 0; i < 3; ++i) {
            http_get("speed.cloudflare.com", "/__down?bytes=1000");
            if (do_yield) yield_cpu();
        }
    }
    // Fetch server location data
    double t_loc = get_time_ms();
    std::string loc_json = http_get("speed.cloudflare.com", "/locations");
    if (!minimize_output && !output_json) printf("[TIME] Fetch locations: %.2f ms\n", get_time_ms() - t_loc);
    auto locations = parse_locations_json(loc_json);
    std::map<std::string, std::string> serverLocationData;
    for (const auto& entry : locations) {
        serverLocationData[entry.at("iata")] = entry.at("city");
    }
    // Fetch CDN trace
    double t_trace = get_time_ms();
    std::string trace = http_get("speed.cloudflare.com", "/cdn-cgi/trace");
    if (!minimize_output && !output_json) printf("[TIME] Fetch trace: %.2f ms\n", get_time_ms() - t_trace);
    auto cfTrace = parse_cdn_trace(trace);
    // Measure latency
    double t_ping = get_time_ms();
    auto ping = measure_latency();
    if (!minimize_output && !output_json) printf("[TIME] Latency: %.2f ms\n", get_time_ms() - t_ping);
    std::string city = serverLocationData.count(cfTrace["colo"]) ? serverLocationData[cfTrace["colo"]] : cfTrace["colo"];
    log_info("Server location", city + " (" + cfTrace["colo"] + ")", output_json);
    std::string ip_out = cfTrace["ip"];
    if (mask_sensitive && !ip_out.empty()) {
        size_t pos = ip_out.find_last_of('.');
        if (pos != std::string::npos) ip_out.replace(pos + 1, std::string::npos, "***");
        else if ((pos = ip_out.find_last_of(':')) != std::string::npos) ip_out.replace(pos + 1, std::string::npos, "****");
    }
    log_info("Your IP", ip_out + " (" + cfTrace["loc"] + ")", output_json);
    log_latency(ping, output_json);
    double t_down = get_time_ms();
    int cpu_count = std::thread::hardware_concurrency();
    auto download_func = use_parallel && cpu_count > 1 ? measure_download_parallel : measure_download;
    auto testDown1 = download_func(101000, 10);
    if (do_yield) yield_cpu();
    log_speed_test_result("100kB", testDown1, output_json);
    auto testDown2 = download_func(1001000, 8);
    if (do_yield) yield_cpu();
    log_speed_test_result("1MB", testDown2, output_json);
    auto testDown3 = download_func(10001000, 6);
    if (do_yield) yield_cpu();
    log_speed_test_result("10MB", testDown3, output_json);
    auto testDown4 = download_func(25001000, 4);
    if (do_yield) yield_cpu();
    log_speed_test_result("25MB", testDown4, output_json);
    auto testDown5 = download_func(100001000, 1);
    if (do_yield) yield_cpu();
    log_speed_test_result("100MB", testDown5, output_json);
    if (!minimize_output && !output_json) printf("[TIME] Download tests: %.2f ms\n", get_time_ms() - t_down);
    std::vector<double> downloadTests;
    downloadTests.insert(downloadTests.end(), testDown1.begin(), testDown1.end());
    downloadTests.insert(downloadTests.end(), testDown2.begin(), testDown2.end());
    downloadTests.insert(downloadTests.end(), testDown3.begin(), testDown3.end());
    downloadTests.insert(downloadTests.end(), testDown4.begin(), testDown4.end());
    downloadTests.insert(downloadTests.end(), testDown5.begin(), testDown5.end());
    log_download_speed(downloadTests, output_json);
    double t_up = get_time_ms();
    auto testUp1 = measure_upload(11000, 10);
    if (do_yield) yield_cpu();
    auto testUp2 = measure_upload(101000, 10);
    if (do_yield) yield_cpu();
    auto testUp3 = measure_upload(1001000, 8);
    if (do_yield) yield_cpu();
    if (!minimize_output && !output_json) printf("[TIME] Upload tests: %.2f ms\n", get_time_ms() - t_up);
    std::vector<double> uploadTests;
    uploadTests.insert(uploadTests.end(), testUp1.begin(), testUp1.end());
    uploadTests.insert(uploadTests.end(), testUp2.begin(), testUp2.end());
    uploadTests.insert(uploadTests.end(), testUp3.begin(), testUp3.end());
    log_upload_speed(uploadTests, output_json);
    if (!minimize_output && !output_json) printf("[TIME] Total: %.2f ms\n", get_time_ms() - t0);
    // If JSON output requested, fill TestResults struct
    if (output_json && json_results) {
        json_results->city = city;
        json_results->colo = cfTrace["colo"];
        json_results->ip = ip_out;
        json_results->loc = cfTrace["loc"];
        // System info
        collect_sysinfo(*json_results, mask_sensitive);
        json_results->latency = ping;
        json_results->download_100kB = testDown1;
        json_results->download_1MB = testDown2;
        json_results->download_10MB = testDown3;
        json_results->download_25MB = testDown4;
        json_results->download_100MB = testDown5;
        json_results->all_downloads = downloadTests;
        json_results->upload_11kB = testUp1;
        json_results->upload_100kB = testUp2;
        json_results->upload_1MB = testUp3;
        json_results->all_uploads = uploadTests;
        json_results->total_time_ms = get_time_ms() - t0;
    }
}

double measure_speed(int bytes, double duration_ms) {
    return (bytes * 8.0) / (duration_ms / 1000.0) / 1e6;
}

// Helper to compute 90th percentile (or fallback to max if not enough data)
double percentile(const std::vector<double>& v, double pct) {
    if (v.empty()) return 0.0;
    std::vector<double> sorted = v;
    std::sort(sorted.begin(), sorted.end());
    size_t idx = static_cast<size_t>(std::ceil(pct * sorted.size())) - 1;
    if (idx >= sorted.size()) idx = sorted.size() - 1;
    return sorted[idx];
}

// --- Output helpers ---
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

void print_sysinfo(bool mask_sensitive) {
    // Date/time (ISO 8601, with timezone, precision in seconds)
    char datetime[64];
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    strftime(datetime, sizeof(datetime), "%Y-%m-%dT%H:%M:%S%z", &tm);
    printf("[SYS] Date: %s\n", datetime);
    printf("[SYS] Version: %s\n", BUILD_VERSION);
    // Hostname, architecture, kernel
    struct utsname uts;
    if (uname(&uts) == 0) {
        std::string host = uts.nodename;
        if (mask_sensitive && !host.empty()) {
            size_t len = host.length();
            if (len > 3) host.replace(len/2, len-len/2, std::string(len-len/2, '*'));
        }
        printf("[SYS] Host: %s\n", host.c_str());
        printf("[SYS] Arch: %s\n", uts.machine);
        printf("[SYS] Kernel: %s %s\n", uts.sysname, uts.release);
    }
    // CPU info (model name, cores)
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line, model;
    int cores = 0;
    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") != std::string::npos) {
            if (model.empty()) model = line.substr(line.find(":") + 2);
            ++cores;
        }
    }
    if (!model.empty()) printf("[SYS] CPU: %s (%d cores)\n", model.c_str(), cores);
    // Mem info
    std::ifstream meminfo("/proc/meminfo");
    std::string memline;
    while (std::getline(meminfo, memline)) {
        if (memline.find("MemTotal:") == 0) {
            std::string memval = memline.substr(memline.find(":") + 1);
            // Extract number (in kB)
            size_t num_start = memval.find_first_of("0123456789");
            size_t num_end = memval.find_first_not_of("0123456789", num_start);
            long mem_kb = 0;
            if (num_start != std::string::npos) {
                mem_kb = std::stol(memval.substr(num_start, num_end - num_start));
            }
            double mem_gb = mem_kb / 1048576.0;
            double mem_mb = mem_kb / 1024.0;
            if (mem_gb >= 1.0)
                printf("[SYS] Mem: %.1f GB\n", mem_gb);
            else
                printf("[SYS] Mem: %.0f MB\n", mem_mb);
            break;
        }
    }
}

// Prototype for mask_str (if defined later)
std::string mask_str(const std::string& s);

// Make sure mask_str is defined before collect_sysinfo
std::string mask_str(const std::string& s) {
    if (s.empty()) return s;
    size_t pos = s.find_last_of('.');
    if (pos != std::string::npos) return s.substr(0, pos+1) + "***";
    pos = s.find_last_of(':');
    if (pos != std::string::npos) return s.substr(0, pos+1) + "****";
    size_t len = s.length();
    if (len > 3) return s.substr(0, len/2) + std::string(len-len/2, '*');
    return s;
}

void collect_sysinfo(TestResults& res, bool mask_sensitive) {
    // Date/time (ISO 8601, with timezone, precision in seconds)
    char datetime[64];
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    strftime(datetime, sizeof(datetime), "%Y-%m-%dT%H:%M:%S%z", &tm);
    res.sysinfo_date = datetime;
    res.version = BUILD_VERSION;

    struct utsname uts;
    if (uname(&uts) == 0) {
        res.host = mask_sensitive ? mask_str(uts.nodename) : uts.nodename;
        res.arch = uts.machine;
        res.kernel = std::string(uts.sysname) + " " + uts.release;
    }
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line, model;
    int cores = 0;
    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") != std::string::npos) {
            if (model.empty()) model = line.substr(line.find(":") + 2);
            ++cores;
        }
    }
    res.cpu_model = model;
    res.cpu_cores = cores;
    std::ifstream meminfo("/proc/meminfo");
    std::string memline;
    while (std::getline(meminfo, memline)) {
        if (memline.find("MemTotal:") == 0) {
            res.mem_total = memline.substr(memline.find(":") + 1);
            break;
        }
    }
}

// Helper: validate UTF-8 (returns true if valid)
bool is_valid_utf8(const std::string& str) {
    const unsigned char* bytes = (const unsigned char*)str.c_str();
    size_t len = str.size();
    size_t i = 0;
    while (i < len) {
        if (bytes[i] <= 0x7F) { i++; continue; }
        if ((bytes[i] & 0xE0) == 0xC0) {
            if (i+1 >= len || (bytes[i+1] & 0xC0) != 0x80) return false;
            i += 2; continue;
        }
        if ((bytes[i] & 0xF0) == 0xE0) {
            if (i+2 >= len || (bytes[i+1] & 0xC0) != 0x80 || (bytes[i+2] & 0xC0) != 0x80) return false;
            i += 3; continue;
        }
        if ((bytes[i] & 0xF8) == 0xF0) {
            if (i+3 >= len || (bytes[i+1] & 0xC0) != 0x80 || (bytes[i+2] & 0xC0) != 0x80 || (bytes[i+3] & 0xC0) != 0x80) return false;
            i += 4; continue;
        }
        return false;
    }
    return true;
}

// --- Summary Table Feature ---
#include <set>
#include <regex>

// Helper: expand wildcards in file patterns (POSIX version, no <filesystem>)
std::vector<std::string> expand_wildcards(const std::vector<std::string>& patterns) {
    std::vector<std::string> files;
    for (const auto& pat : patterns) {
        // If no wildcard, just add
        if (pat.find('*') == std::string::npos && pat.find('?') == std::string::npos) {
            files.push_back(pat);
            continue;
        }
        // Split path and pattern
        size_t slash = pat.find_last_of("/");
        std::string dir = (slash == std::string::npos) ? "." : pat.substr(0, slash);
        std::string pattern = (slash == std::string::npos) ? pat : pat.substr(slash + 1);
        DIR* dp = opendir(dir.c_str());
        if (!dp) continue;
        struct dirent* ep;
        while ((ep = readdir(dp))) {
            if (fnmatch(pattern.c_str(), ep->d_name, 0) == 0) {
                std::string full = (dir == ".") ? ep->d_name : dir + "/" + ep->d_name;
                files.push_back(full);
            }
        }
        closedir(dp);
    }
    return files;
}

struct SummaryResult {
    std::string file;
    std::string server_city;
    std::string ip;
    double latency;
    double jitter;
    double download;
    double upload;
};

// Forward declaration for summary table printer
void print_summary_table(const std::vector<SummaryResult>& results);

std::vector<SummaryResult> load_summary_results(const std::vector<std::string>& files, bool diagnostics_mode = false, bool debug_mode = false) {
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

void write_summary_json(const std::vector<SummaryResult>& results, const std::string& filename, bool diagnostics_mode = false, bool debug_mode = false) {
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
    // ...existing code for full serialization...
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

void yyjson_minimal_test(bool diagnostics_mode = false, bool debug_mode = false) {
    if (!(diagnostics_mode || debug_mode)) return;
    fprintf(stderr, "[DIAG] Running minimal yyjson test with 8 objects...\n");
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *arr = yyjson_mut_arr(doc);
    for (int i = 0; i < 8; ++i) {
        yyjson_mut_val *obj = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_str(doc, obj, "label", "test");
        yyjson_mut_obj_add_int(doc, obj, "value", i);
        yyjson_mut_arr_add_val(arr, obj);
    }
    yyjson_mut_val *root = yyjson_mut_obj(doc);
    yyjson_mut_obj_add_val(doc, root, "results", arr);
    yyjson_mut_doc_set_root(doc, root);
    size_t len = 0;
    char *json = (char *)yyjson_mut_write(doc, 0, &len);
    if (json) {
        fprintf(stderr, "[DIAG] Minimal test serialization succeeded! Length: %zu\n", len);
        free(json);
    } else {
        fprintf(stderr, "[DIAG] Minimal test serialization FAILED!\n");
    }
    yyjson_mut_doc_free(doc);
}

// Validate a JSON file against a schema file using nlohmann_json_schema_validator
bool validate_json_schema(const std::string& json_path, const std::string& schema_path, bool diagnostics_mode = false) {
    using nlohmann::json;
    using nlohmann::json_schema::json_validator;
    std::ifstream schema_stream(schema_path);
    std::ifstream json_stream(json_path);
    if (!schema_stream) {
        if (diagnostics_mode)
            fprintf(stderr, "[SCHEMA] Could not open schema file: %s\n", schema_path.c_str());
        return false;
    }
    if (!json_stream) {
        if (diagnostics_mode)
            fprintf(stderr, "[SCHEMA] Could not open JSON file: %s\n", json_path.c_str());
        return false;
    }
    json schema_json;
    json data_json;
    try {
        schema_stream >> schema_json;
        json_stream >> data_json;
    } catch (const std::exception& e) {
        if (diagnostics_mode)
            fprintf(stderr, "[SCHEMA] Failed to parse JSON or schema: %s\n", e.what());
        return false;
    }
    json_validator validator;
    try {
        validator.set_root_schema(schema_json);
    } catch (const std::exception& e) {
        if (diagnostics_mode)
            fprintf(stderr, "[SCHEMA] Invalid schema: %s\n", e.what());
        return false;
    }
    try {
        validator.validate(data_json);
        if (diagnostics_mode)
            fprintf(stderr, "[SCHEMA] JSON is valid according to schema.\n");
        return true;
    } catch (const std::exception& e) {
        if (diagnostics_mode)
            fprintf(stderr, "[SCHEMA] JSON validation failed: %s\n", e.what());
        return false;
    }
}

// Print a summary table to stdout from summary results
void print_summary_table(const std::vector<SummaryResult>& results) {
    if (results.empty()) {
        printf("No results to display.\n");
        return;
    }
    // Print header
    printf("%-20s %-15s %-15s %10s %10s %12s %12s\n",
           "File", "Server City", "IP", "Latency", "Jitter", "Download", "Upload");
    printf("%s\n", std::string(20+15+15+10+10+12+12+6, '-').c_str());
    // Print each result
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

// Serialize TestResults to JSON string using yyjson
std::string serialize_to_json(const TestResults& res) {
    // Defensive: ensure all strings are valid
    auto safe = [](const std::string& s) -> const char* { return s.empty() ? "" : s.c_str(); };
    #undef ADD_STR
    #define ADD_STR(key, val) yyjson_mut_obj_add_str(doc, obj, key, safe(val))
    #undef ADD_NUM
    #define ADD_NUM(key, val) yyjson_mut_obj_add_real(doc, obj, key, val)
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val* obj = yyjson_mut_obj(doc);
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
    // Write cpu_cores as integer
    yyjson_mut_obj_add_int(doc, obj, "cpu_cores", res.cpu_cores);
    // Helper for vector<double> to array
    auto add_vec = [&](const char* key, const std::vector<double>& v) {
        yyjson_mut_val* arr = yyjson_mut_arr(doc);
        for (double d : v) yyjson_mut_arr_add_real(doc, arr, d);
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
    // Add summary fields for summary table compatibility
    ADD_NUM("latency_avg", res.latency.size() > 2 ? res.latency[2] : 0.0);
    ADD_NUM("jitter", res.latency.size() > 4 ? res.latency[4] : 0.0);
    ADD_NUM("download_90pct", percentile(res.all_downloads, 0.9));
    ADD_NUM("upload_90pct", percentile(res.all_uploads, 0.9));
    // Flags used
    yyjson_mut_val* flags_arr = yyjson_mut_arr(doc);
    for (const auto& f : res.flags) yyjson_mut_arr_add_str(doc, flags_arr, f.c_str());
    yyjson_mut_obj_add_val(doc, obj, "flags", flags_arr);
    std::string out = yyjson_mut_write(doc, 0, NULL);
    yyjson_mut_doc_free(doc);
    return out;
}

int main(int argc, char* argv[]) {
    bool use_parallel = false;
    bool minimize_output = false;
    bool warmup = true;
    bool pin_single_core = false;
    bool do_yield = true;
    bool do_nice = true;
    bool do_drop_caches = false;
    bool output_json = false;
    bool mask_sensitive = false;
    bool show_sysinfo = false;
    bool show_sysinfo_only = false;
    bool summary_table = false;
    bool debug_mode = false;
    bool diagnostics_mode = false;
    std::vector<std::string> used_flags;
    std::vector<std::string> summary_files;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        bool recognized = false;
        if (arg == "--parallel" || arg == "-p") { use_parallel = true; used_flags.push_back(arg); recognized = true; }
        if (arg == "--minimize-output" || arg == "-m") { minimize_output = true; used_flags.push_back(arg); recognized = true; }
        if (arg == "--no-warmup") { warmup = false; used_flags.push_back(arg); recognized = true; }
        if (arg == "--single-core" || arg == "-s") { pin_single_core = true; used_flags.push_back(arg); recognized = true; }
        if (arg == "--no-yield") { do_yield = false; used_flags.push_back(arg); recognized = true; }
        if (arg == "--no-nice") { do_nice = false; used_flags.push_back(arg); recognized = true; }
        if (arg == "--drop-caches") { do_drop_caches = true; used_flags.push_back(arg); recognized = true; }
        if (arg == "--mask-sensitive") { mask_sensitive = true; used_flags.push_back(arg); recognized = true; }
        if (arg == "--show-sysinfo") { show_sysinfo = true; used_flags.push_back(arg); recognized = true; }
        if (arg == "--show-sysinfo-only") { show_sysinfo_only = true; recognized = true; }
        if (arg == "--json") { output_json = true; used_flags.push_back(arg); recognized = true; }
        if (arg == "--summary-table") {
            summary_table = true;
            recognized = true;
            // All following args are files or globs
            std::vector<std::string> patterns;
            for (int j = i + 1; j < argc; ++j) {
                patterns.push_back(argv[j]);
            }
            summary_files = expand_wildcards(patterns);
            // Exclude summary.json from summary_files
            summary_files.erase(
                std::remove_if(summary_files.begin(), summary_files.end(),
                    [](const std::string& f) {
                        return f.find(SUMMARY_JSON_FILENAME) != std::string::npos;
                    }),
                summary_files.end());
            break;
        }
        if (arg == "--debug") { debug_mode = true; recognized = true; }
        if (arg == "--diagnostics") { diagnostics_mode = true; recognized = true; }
        if (arg == "--help" || arg == "-h") { print_help(); return 0; }
        if (!recognized) {
            std::cerr << "[WARN] Unknown flag: " << arg << std::endl;
        }
    }
    if (summary_table) {
        if (summary_files.empty()) {
            std::cerr << "[ERROR] --summary-table requires at least one JSON file as argument." << std::endl;
            return 1;
        }
        // Debug: print summary_files
        if (debug_mode) {
            fprintf(stderr, "[DEBUG] Summary files to be loaded (%zu):\n", summary_files.size());
            for (const auto& f : summary_files) fprintf(stderr, "  %s\n", f.c_str());
        }
        // Validate each individual result file before summary
        for (const auto& file : summary_files) {
            validate_json_schema(file, "result.schema.json", diagnostics_mode);
        }
        auto results = load_summary_results(summary_files, diagnostics_mode, debug_mode);
        print_summary_table(results);
        std::string summary_path = std::string("results/") + SUMMARY_JSON_FILENAME;
        write_summary_json(results, summary_path, diagnostics_mode, debug_mode);
        // Validate the summary JSON against the schema
        validate_json_schema(summary_path, "summary.schema.json", diagnostics_mode);
        if (diagnostics_mode) yyjson_minimal_test(diagnostics_mode, debug_mode);
        return 0;
    }
    if (show_sysinfo_only) {
        print_sysinfo(mask_sensitive);
        return 0;
    }
    if (show_sysinfo) print_sysinfo(mask_sensitive);
    if (pin_single_core) pin_to_core(0);
    if (do_nice) set_nice();
    if (do_drop_caches) drop_caches();
    if (output_json) {
        TestResults results;
        speed_test(use_parallel, minimize_output, warmup, do_yield, mask_sensitive, output_json, &results);
        std::cout << serialize_to_json(results) << std::endl;
    } else {
        speed_test(use_parallel, minimize_output, warmup, do_yield, mask_sensitive, output_json);
    }
    if (debug_mode || diagnostics_mode) yyjson_minimal_test(diagnostics_mode, debug_mode);
    return 0;
}
