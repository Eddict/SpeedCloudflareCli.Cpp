#pragma once
#include <string>
#include <vector>

// Modernized: trailing return types, descriptive parameter names
struct CliArgs {
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
};

auto parse_cli_args(const std::vector<std::string>& arguments) -> CliArgs;
