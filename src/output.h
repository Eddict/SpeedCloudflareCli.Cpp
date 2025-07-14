#pragma once
#include "types.h" // Ensure SummaryResult is visible
#include <string>
#include <vector>

// Output helpers
// Modernized: trailing return types, descriptive parameter names
auto fmt(double value) -> std::string;
void log_info(const std::string &label, const std::string &data,
              bool output_json);
void log_latency(const std::vector<double> &latency_data, bool output_json);
void log_speed_test_result(const std::string &size_label,
                           const std::vector<double> &test_data, bool output_json);
void log_download_speed(const std::vector<double> &download_tests, bool output_json);
void log_upload_speed(const std::vector<double> &upload_tests, bool output_json);
void print_summary_table(const std::vector<SummaryResult> &summary_results);
