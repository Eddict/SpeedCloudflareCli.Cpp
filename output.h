#pragma once
#include <string>
#include <vector>
#include "types.h" // Ensure SummaryResult is visible

// Output helpers
std::string fmt(double v);
void log_info(const std::string& text, const std::string& data, bool output_json);
void log_latency(const std::vector<double>& data, bool output_json);
void log_speed_test_result(const std::string& size, const std::vector<double>& test, bool output_json);
void log_download_speed(const std::vector<double>& tests, bool output_json);
void log_upload_speed(const std::vector<double>& tests, bool output_json);
void print_summary_table(const std::vector<SummaryResult>& results);
