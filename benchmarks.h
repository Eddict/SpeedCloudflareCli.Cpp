#pragma once
#include <vector>
#include <string>
#include "types.h"

// Speed test helpers
std::vector<double> measure_latency();
std::vector<double> measure_download(int bytes, int iterations);
std::vector<double> measure_download_parallel(int bytes, int iterations);
std::vector<double> measure_upload(int bytes, int iterations);
double measure_speed(int bytes, double duration_ms);

// Main speed test
void speed_test(bool use_parallel = false, bool minimize_output = false, bool warmup = true, bool do_yield = true, bool mask_sensitive = false, bool output_json = false, TestResults* json_results = nullptr);
