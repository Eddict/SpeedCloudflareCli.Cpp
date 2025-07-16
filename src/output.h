#pragma once
#include <string>  // for string
#include <vector>  // for vector

struct SummaryResult;

// Output helpers
// Modernized: trailing return types, descriptive parameter names
auto fmt(double value) -> std::string;
void log_info(const std::string& label, const std::string& data, bool output_json);
void log_latency(const std::vector<double>& latency_data, bool output_json);
void log_speed_test_result(const std::string& size_label, const std::vector<double>& test_data,
                           bool output_json);
void log_download_speed(const std::vector<double>& download_tests, bool output_json);
void log_upload_speed(const std::vector<double>& upload_tests, bool output_json);
auto load_summary_results(const std::vector<std::string>& files, bool is_diagnostics = false,
                          bool is_debug = false) -> std::vector<SummaryResult>;
void write_summary_json(const std::vector<SummaryResult>& results, const std::string& filename,
                        bool is_diagnostics = false, bool is_debug = false,
                        bool is_full_diagnostics = false);
void print_summary_table(const std::vector<SummaryResult>& summary_results);
