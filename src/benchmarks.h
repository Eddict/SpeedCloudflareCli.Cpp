#pragma once
#include "types.h"
#include <string>
#include <vector>

// Speed test helpers
// Modernized: trailing return types, descriptive parameter names
auto measure_latency() -> std::vector<double>;
auto measure_download(int bytes, int iterations) -> std::vector<double>;
auto measure_download_parallel(int bytes, int iterations) -> std::vector<double>;
auto measure_upload(int bytes, int iterations) -> std::vector<double>;
auto measure_speed(int bytes, double duration_ms) -> double;

// Main speed test
auto speed_test(bool use_parallel = false, bool minimize_output = false,
                bool warmup = true, bool do_yield = true,
                bool mask_sensitive = false, bool output_json = false,
                TestResults *json_results = nullptr) -> void;
