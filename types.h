#pragma once
#include <string>
#include <vector>

extern const char* BUILD_VERSION;

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

struct SummaryResult {
    std::string file;
    std::string server_city;
    std::string ip;
    double latency;
    double jitter;
    double download;
    double upload;
};
