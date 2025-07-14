#include "benchmarks.h"
#include "network.h"
#include "output.h"
#include "stats.h"
#include "sysinfo.h"
#include "types.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <yyjson.h>

constexpr int kLatencySamples = 20;
constexpr int kDownload100kB = 101000;
constexpr int kDownload1MB = 1001000;
constexpr int kDownload10MB = 10001000;
constexpr int kDownload25MB = 25001000;
constexpr int kDownload100MB = 100001000;
constexpr int kUpload11kB = 11000;
constexpr int kUpload100kB = 101000;
constexpr int kUpload1MB = 1001000;
constexpr int kDownloadIters1 = 10;
constexpr int kDownloadIters2 = 8;
constexpr int kDownloadIters3 = 6;
constexpr int kDownloadIters4 = 4;
constexpr int kDownloadIters5 = 1;
constexpr int kUploadIters1 = 10;
constexpr int kUploadIters2 = 10;
constexpr int kUploadIters3 = 8;
constexpr double kBitsPerByte = 8.0;
constexpr double kMsPerSecond = 1000.0;
constexpr double kMbpsDivisor = 1e6;
constexpr int kNumLatencyStats = 5;

auto measure_latency() -> std::vector<double> {
  std::vector<double> measurements;
  measurements.reserve(kLatencySamples);
  for (int sample_index = 0; sample_index < kLatencySamples; ++sample_index) {
    const auto start_time = std::chrono::high_resolution_clock::now();
    const std::string response = http_get("speed.cloudflare.com", "/__down?bytes=1000");
    const auto end_time = std::chrono::high_resolution_clock::now();
    if (!response.empty()) {
      const double milliseconds =
          std::chrono::duration<double, std::milli>(end_time - start_time).count();
      measurements.push_back(milliseconds);
    }
  }
  if (measurements.empty()) {
    return std::vector<double>(kNumLatencyStats, 0.0);
  }
  return {
    *std::min_element(measurements.begin(), measurements.end()),
    *std::max_element(measurements.begin(), measurements.end()),
    stats::average(measurements),
    stats::median(measurements),
    stats::jitter(measurements)
  };
}

auto measure_download(int num_bytes, int num_iterations) -> std::vector<double> {
  std::vector<double> download_results;
  download_results.reserve(num_iterations);
  for (int iteration = 0; iteration < num_iterations; ++iteration) {
    const auto start_time = std::chrono::high_resolution_clock::now();
    const std::string response = http_get("speed.cloudflare.com",
                                "/__down?bytes=" + std::to_string(num_bytes));
    const auto end_time = std::chrono::high_resolution_clock::now();
    if (!response.empty()) {
      const double milliseconds =
          std::chrono::duration<double, std::milli>(end_time - start_time).count();
      download_results.push_back(measure_speed(num_bytes, milliseconds));
    }
  }
  return download_results;
}

auto measure_upload(int num_bytes, int num_iterations) -> std::vector<double> {
  std::vector<double> upload_results;
  upload_results.reserve(num_iterations);
  const std::string upload_data(num_bytes, '0');
  for (int iteration = 0; iteration < num_iterations; ++iteration) {
    const auto start_time = std::chrono::high_resolution_clock::now();
    const std::string response = http_post("speed.cloudflare.com", "/__up", upload_data);
    const auto end_time = std::chrono::high_resolution_clock::now();
    const double milliseconds = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    upload_results.push_back(measure_speed(num_bytes, milliseconds));
  }
  return upload_results;
}

auto measure_speed(int num_bytes, double duration_ms) -> double {
  return (num_bytes * kBitsPerByte) / (duration_ms / kMsPerSecond) / kMbpsDivisor;
}

std::vector<double> measure_download_parallel(int bytes, int iterations) {
  std::vector<std::future<double>> futures;
  futures.reserve(iterations);
  for (int i = 0; i < iterations; ++i) {
    futures.push_back(std::async(std::launch::async, [bytes]() {
      auto start = std::chrono::high_resolution_clock::now();
      std::string resp = http_get("speed.cloudflare.com",
                                  "/__down?bytes=" + std::to_string(bytes));
      auto end = std::chrono::high_resolution_clock::now();
      if (!resp.empty()) {
        double ms =
            std::chrono::duration<double, std::milli>(end - start).count();
        return measure_speed(bytes, ms);
      }
      return 0.0;
    }));
  }
  std::vector<double> results = {0.0};
  results.reserve(iterations);
  for (auto &f : futures)
    results.push_back(f.get());
  return results;
}

void speed_test(bool use_parallel, bool minimize_output, bool warmup,
                bool do_yield, bool mask_sensitive, bool output_json,
                TestResults *json_results) {
  double t0 = get_time_ms();
  if (warmup) {
    for (int i = 0; i < 3; ++i) {
      http_get("speed.cloudflare.com", "/__down?bytes=1000");
      if (do_yield)
        yield_cpu();
    }
  }
  // Fetch server location data
  double t_loc = get_time_ms();
  std::string loc_json = http_get("speed.cloudflare.com", "/locations");
  if (!minimize_output && !output_json)
    std::cout << "[TIME] Fetch locations: " << (get_time_ms() - t_loc) << " ms\n";
  auto locations = parse_locations_json(loc_json);
  std::map<std::string, std::string> serverLocationData = {};
  for (const auto &entry : locations) {
    serverLocationData[entry.at("iata")] = entry.at("city");
  }
  // Fetch CDN trace
  double t_trace = get_time_ms();
  std::string trace = http_get("speed.cloudflare.com", "/cdn-cgi/trace");
  if (!minimize_output && !output_json)
    std::cout << "[TIME] Fetch trace: " << (get_time_ms() - t_trace) << " ms\n";
  auto cfTrace = parse_cdn_trace(trace);
  // Measure latency
  double t_ping = get_time_ms();
  auto ping = measure_latency();
  if (!minimize_output && !output_json)
    std::cout << "[TIME] Latency: " << (get_time_ms() - t_ping) << " ms\n";
  std::string city = serverLocationData.count(cfTrace["colo"])
                         ? serverLocationData[cfTrace["colo"]]
                         : cfTrace["colo"];
  log_info("Server location", city + " (" + cfTrace["colo"] + ")", output_json);
  std::string ip_out = cfTrace["ip"];
  if (mask_sensitive && !ip_out.empty()) {
    size_t pos = ip_out.find_last_of('.');
    if (pos != std::string::npos)
      ip_out.replace(pos + 1, std::string::npos, "***");
    else if ((pos = ip_out.find_last_of(':')) != std::string::npos)
      ip_out.replace(pos + 1, std::string::npos, "****");
  }
  log_info("Your IP", ip_out + " (" + cfTrace["loc"] + ")", output_json);
  log_latency(ping, output_json);
  double t_down = get_time_ms();
  int cpu_count = static_cast<int>(std::thread::hardware_concurrency());
  auto download_func = use_parallel && cpu_count > 1 ? measure_download_parallel
                                                     : measure_download;
  auto testDown1 = download_func(kDownload100kB, kDownloadIters1);
  if (do_yield)
    yield_cpu();
  log_speed_test_result("100kB", testDown1, output_json);
  auto testDown2 = download_func(kDownload1MB, kDownloadIters2);
  if (do_yield)
    yield_cpu();
  log_speed_test_result("1MB", testDown2, output_json);
  auto testDown3 = download_func(kDownload10MB, kDownloadIters3);
  if (do_yield)
    yield_cpu();
  log_speed_test_result("10MB", testDown3, output_json);
  auto testDown4 = download_func(kDownload25MB, kDownloadIters4);
  if (do_yield)
    yield_cpu();
  log_speed_test_result("25MB", testDown4, output_json);
  auto testDown5 = download_func(kDownload100MB, kDownloadIters5);
  if (do_yield)
    yield_cpu();
  log_speed_test_result("100MB", testDown5, output_json);
  if (!minimize_output && !output_json)
    std::cout << "[TIME] Download tests: " << (get_time_ms() - t_down) << " ms\n";
  std::vector<double> downloadTests = {0.0};
  downloadTests.insert(downloadTests.end(), testDown1.begin(), testDown1.end());
  downloadTests.insert(downloadTests.end(), testDown2.begin(), testDown2.end());
  downloadTests.insert(downloadTests.end(), testDown3.begin(), testDown3.end());
  downloadTests.insert(downloadTests.end(), testDown4.begin(), testDown4.end());
  downloadTests.insert(downloadTests.end(), testDown5.begin(), testDown5.end());
  log_download_speed(downloadTests, output_json);
  double t_up = get_time_ms();
  auto testUp1 = measure_upload(kUpload11kB, kUploadIters1);
  if (do_yield)
    yield_cpu();
  auto testUp2 = measure_upload(kUpload100kB, kUploadIters2);
  if (do_yield)
    yield_cpu();
  auto testUp3 = measure_upload(kUpload1MB, kUploadIters3);
  if (do_yield)
    yield_cpu();
  if (!minimize_output && !output_json)
    std::cout << "[TIME] Upload tests: " << (get_time_ms() - t_up) << " ms\n";
  std::vector<double> uploadTests = {0.0};
  uploadTests.insert(uploadTests.end(), testUp1.begin(), testUp1.end());
  uploadTests.insert(uploadTests.end(), testUp2.begin(), testUp2.end());
  uploadTests.insert(uploadTests.end(), testUp3.begin(), testUp3.end());
  log_upload_speed(uploadTests, output_json);
  if (!minimize_output && !output_json)
    std::cout << "[TIME] Total: " << (get_time_ms() - t0) << " ms\n";
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
