#include "benchmarks.h"
#include <chrono>         // IWYU pragma: keep  // for operator-, duration, high_resolution_clock
#ifndef CROSSCOMPILING_BUILD
  // code only for non-cross-compiling builds
  #include <bits/chrono.h>  // for operator-, duration, high_resolution_clock
#endif
#include <cstring>        // for strerror
#include <stddef.h>       // for size_t
#include <algorithm>      // for max_element, min, min_element
#include <future>         // for future, async, launch, launch::async
#include <iostream>       // for operator<<, basic_ostream, basic_ostream<>:...
#include <map>            // for map, map<>::mapped_type
#include <ratio>          // for milli
#include <string>         // for string, allocator, operator+, char_traits
#include <thread>         // for thread
#include <vector>         // for vector, vector<>::iterator
#include <fstream>        // IWYU pragma: keep  // for logging errors
#include <pthread.h> // for thread affinity
#include "network.h"      // for http_get, HttpRequest, http_post, parse_cdn...
#include "output.h"       // for log_speed_test_result, log_info, log_downlo...
#include "stats.h"        // for average, jitter, median
#include "sysinfo.h"      // for get_time_ms, yield_cpu, collect_sysinfo
#include "types.h"        // for TestResults

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

// Refactored measure_download, measure_upload, and measure_download_parallel to use BenchmarkParams

void set_benchmark_cpu_affinity(int cpu_core) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_core, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

auto measure_download(const BenchmarkParams& params) -> std::vector<double>
{
  set_benchmark_cpu_affinity(0); // Pin to core 0
  std::vector<double> download_results;
  download_results.reserve(params.num_iterations);
  const std::string url = "/__down?bytes=" + std::to_string(params.num_bytes);
  int failed_requests = 0;
  for (int i = 0; i < params.num_iterations; ++i)
  {
    const auto start_time = std::chrono::high_resolution_clock::now();
    const std::string response = http_get(HttpRequest{"speed.cloudflare.com", url});
    const auto end_time = std::chrono::high_resolution_clock::now();
    if (!response.empty())
    {
      const double milliseconds =
          std::chrono::duration<double, std::milli>(end_time - start_time).count();
      download_results.push_back(measure_speed(params.num_bytes, milliseconds));
    }
    else {
      ++failed_requests;
      std::ofstream errlog("results/download_errors.log", std::ios::app);
      errlog << "Download failed for iteration " << i << "\n";
    }
  }
  // Filter: remove first and slowest run
  if (download_results.size() > 2) {
    download_results.erase(download_results.begin()); // remove first
    auto slowest = std::max_element(download_results.begin(), download_results.end());
    download_results.erase(slowest);
  }
  return download_results;
}

auto measure_upload(const BenchmarkParams& params) -> std::vector<double>
{
  set_benchmark_cpu_affinity(0); // Pin to core 0
  std::vector<double> upload_results;
  upload_results.reserve(params.num_iterations);
  static std::string upload_data(params.num_bytes, '0'); // Reuse buffer
  int failed_requests = 0;
  for (int iteration_index = 0; iteration_index < params.num_iterations; ++iteration_index)
  {
    try {
      const auto start_time = std::chrono::high_resolution_clock::now();
      const std::string response = http_post(HttpRequest{"speed.cloudflare.com", "/__up"}, upload_data);
      const auto end_time = std::chrono::high_resolution_clock::now();
      const double milliseconds =
          std::chrono::duration<double, std::milli>(end_time - start_time).count();
      if (!response.empty())
      {
        upload_results.push_back(measure_speed(params.num_bytes, milliseconds));
      }
      else {
        ++failed_requests;
        std::ofstream errlog("results/upload_errors.log", std::ios::app);
        errlog << "[" << std::time(nullptr) << "] Upload failed for iteration " << iteration_index;
        errlog << ", response='" << response << "'";
        errlog << ", errno=" << errno << " (" << strerror(errno) << ")";
        errlog << "\n";
        std::cerr << "[DEBUG] Upload failed for iteration " << iteration_index << ", errno=" << errno << " (" << strerror(errno) << ")\n";
      }
    } catch (const std::exception& ex) {
      std::ofstream errlog("results/upload_errors.log", std::ios::app);
      errlog << "Upload failed for iteration " << iteration_index << ": exception: " << ex.what() << "\n";
    }
  }
  // Filter: remove first and slowest run
  if (upload_results.size() > 2) {
    upload_results.erase(upload_results.begin()); // remove first
    auto slowest = std::max_element(upload_results.begin(), upload_results.end());
    upload_results.erase(slowest);
  }
  return upload_results;
}

auto measure_download_parallel(const BenchmarkParams& params) -> std::vector<double>
{
  const int max_threads = 2;
  int num_threads = std::min(max_threads, params.num_iterations);
  std::vector<std::future<double>> futures;
  futures.reserve(params.num_iterations);
  const std::string url = "/__down?bytes=" + std::to_string(params.num_bytes);
  int launched = 0;
  std::vector<double> results;
  results.reserve(params.num_iterations);
  while (launched < params.num_iterations)
  {
    int batch = std::min(num_threads, params.num_iterations - launched);
    std::vector<std::future<double>> batch_futures;
    batch_futures.reserve(batch);
    for (int i = 0; i < batch; ++i)
    {
      batch_futures.push_back(std::async(
          std::launch::async,
          [&url, &params]()
          {
            auto start = std::chrono::high_resolution_clock::now();
            std::string resp = http_get(HttpRequest{"speed.cloudflare.com", url});
            auto end = std::chrono::high_resolution_clock::now();
            if (!resp.empty())
            {
              auto milliseconds = std::chrono::duration<double, std::milli>(end - start).count();
              return measure_speed(params.num_bytes, milliseconds);
            }
            return 0.0;
          }));
    }
    for (auto& fut : batch_futures)
    {
      results.push_back(fut.get());
    }
    launched += batch;
  }
  return results;
}

// Wrapper for legacy interface: measure_download(int, int)
auto measure_download(int bytes, int iterations) -> std::vector<double>
{
  return measure_download(BenchmarkParams{bytes, iterations});
}

// Wrapper for legacy interface: measure_upload(int bytes, int iterations)
auto measure_upload(int bytes, int iterations) -> std::vector<double>
{
  return measure_upload(BenchmarkParams{bytes, iterations});
}

// Wrapper for legacy interface: measure_download_parallel(int, int)
auto measure_download_parallel(int bytes, int iterations) -> std::vector<double>
{
  return measure_download_parallel(BenchmarkParams{bytes, iterations});
}

// Use braced initializer list for vector return
auto measure_latency() -> std::vector<double>
{
  std::vector<double> measurements;
  measurements.reserve(kLatencySamples);
  for (int sample_index = 0; sample_index < kLatencySamples; ++sample_index)
  {
    const auto start_time = std::chrono::high_resolution_clock::now();
    const std::string response = http_get(HttpRequest{"speed.cloudflare.com", "/__down?bytes=1000"});
    const auto end_time = std::chrono::high_resolution_clock::now();
    if (!response.empty())
    {
      const double milliseconds =
          std::chrono::duration<double, std::milli>(end_time - start_time).count();
      measurements.push_back(milliseconds);
    }
  }
  if (measurements.empty())
  {
    return {0.0, 0.0, 0.0, 0.0, 0.0};
  }
  return {*std::min_element(measurements.begin(), measurements.end()),
          *std::max_element(measurements.begin(), measurements.end()), stats::average(measurements),
          stats::median(measurements), stats::jitter(measurements)};
}

auto measure_speed(int num_bytes, double duration_ms) -> double
{
  return (num_bytes * kBitsPerByte) / (duration_ms / kMsPerSecond) / kMbpsDivisor;
}

void speed_test(bool use_parallel, bool minimize_output, bool warmup, bool do_yield,
                bool mask_sensitive, bool output_json, TestResults* json_results)
{
  auto start_time_ms = get_time_ms();
  if (warmup)
  {
    for (int warmup_index = 0; warmup_index < 3; ++warmup_index)
    {
      http_get(HttpRequest{"speed.cloudflare.com", "/__down?bytes=1000"});
      if (do_yield)
      {
        yield_cpu();
      }
    }
  }
  // Fetch server location data
  auto t_loc = get_time_ms();
  std::string loc_json = http_get(HttpRequest{"speed.cloudflare.com", "/locations"});
  if (!minimize_output && !output_json)
  {
    std::cout << "[TIME] Fetch locations: " << (get_time_ms() - t_loc) << " ms\n";
  }
  auto locations = parse_locations_json(loc_json);
  std::map<std::string, std::string> serverLocationData = {};
  for (const auto& entry : locations)
  {
    serverLocationData[entry.at("iata")] = entry.at("city");
  }
  // Fetch CDN trace
  auto t_trace = get_time_ms();
  std::string trace = http_get(HttpRequest{"speed.cloudflare.com", "/cdn-cgi/trace"});
  if (!minimize_output && !output_json)
  {
    std::cout << "[TIME] Fetch trace: " << (get_time_ms() - t_trace) << " ms\n";
  }
  auto cfTrace = parse_cdn_trace(trace);
  // Measure latency
  auto t_ping = get_time_ms();
  auto ping = measure_latency();
  if (!minimize_output && !output_json)
  {
    std::cout << "[TIME] Latency: " << (get_time_ms() - t_ping) << " ms\n";
  }
  std::string city = serverLocationData.count(cfTrace["colo"]) ? serverLocationData[cfTrace["colo"]]
                                                               : cfTrace["colo"];
  log_info("Server location", city + " (" + cfTrace["colo"] + ")", output_json);
  std::string ip_out = cfTrace["ip"];
  if (mask_sensitive && !ip_out.empty())
  {
    size_t pos = ip_out.find_last_of('.');
    if (pos != std::string::npos)
    {
      ip_out.replace(pos + 1, std::string::npos, "***");
    }
    else if ((pos = ip_out.find_last_of(':')) != std::string::npos)
    {
      ip_out.replace(pos + 1, std::string::npos, "****");
    }
  }
  log_info("Your IP", ip_out + " (" + cfTrace["loc"] + ")", output_json);
  log_latency(ping, output_json);
  auto t_down = get_time_ms();
  int cpu_count = static_cast<int>(std::thread::hardware_concurrency());
  auto download_func =
      use_parallel && cpu_count > 1
          ? static_cast<std::vector<double> (*)(int, int)>(measure_download_parallel)
          : static_cast<std::vector<double> (*)(int, int)>(measure_download);
  auto testDown1 = download_func(kDownload100kB, kDownloadIters1);
  if (do_yield)
  {
    yield_cpu();
  }
  log_speed_test_result("100kB", testDown1, output_json);
  auto testDown2 = download_func(kDownload1MB, kDownloadIters2);
  if (do_yield)
  {
    yield_cpu();
  }
  log_speed_test_result("1MB", testDown2, output_json);
  auto testDown3 = download_func(kDownload10MB, kDownloadIters3);
  if (do_yield)
  {
    yield_cpu();
  }
  log_speed_test_result("10MB", testDown3, output_json);
  auto testDown4 = download_func(kDownload25MB, kDownloadIters4);
  if (do_yield)
  {
    yield_cpu();
  }
  log_speed_test_result("25MB", testDown4, output_json);
  auto testDown5 = download_func(kDownload100MB, kDownloadIters5);
  if (do_yield)
  {
    yield_cpu();
  }
  log_speed_test_result("100MB", testDown5, output_json);
  if (!minimize_output && !output_json)
  {
    std::cout << "[TIME] Download tests: " << (get_time_ms() - t_down) << " ms\n";
  }
  std::vector<double> downloadTests;
  downloadTests.insert(downloadTests.end(), testDown1.begin(), testDown1.end());
  downloadTests.insert(downloadTests.end(), testDown2.begin(), testDown2.end());
  downloadTests.insert(downloadTests.end(), testDown3.begin(), testDown3.end());
  downloadTests.insert(downloadTests.end(), testDown4.begin(), testDown4.end());
  downloadTests.insert(downloadTests.end(), testDown5.begin(), testDown5.end());
  log_download_speed(downloadTests, output_json);
  auto t_up = get_time_ms();
  auto testUp1 = measure_upload(kUpload11kB, kUploadIters1);
  if (do_yield)
  {
    yield_cpu();
  }
  auto testUp2 = measure_upload(kUpload100kB, kUploadIters2);
  if (do_yield)
  {
    yield_cpu();
  }
  auto testUp3 = measure_upload(kUpload1MB, kUploadIters3);
  if (do_yield)
  {
    yield_cpu();
  }
  if (!minimize_output && !output_json)
  {
    std::cout << "[TIME] Upload tests: " << (get_time_ms() - t_up) << " ms\n";
  }
  std::vector<double> uploadTests;
  uploadTests.insert(uploadTests.end(), testUp1.begin(), testUp1.end());
  uploadTests.insert(uploadTests.end(), testUp2.begin(), testUp2.end());
  uploadTests.insert(uploadTests.end(), testUp3.begin(), testUp3.end());
  log_upload_speed(uploadTests, output_json);
  if (!minimize_output && !output_json)
  {
    std::cout << "[TIME] Total: " << (get_time_ms() - start_time_ms) << " ms\n";
  }
  // If JSON output requested, fill TestResults struct
  if (output_json && json_results)
  {
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
    json_results->total_time_ms = get_time_ms() - start_time_ms;
  }
}
