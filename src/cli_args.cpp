#include "cli_args.h"
#include "types.h"
#include <algorithm>
#include <dirent.h>
#include <fnmatch.h>
#include <iostream>
#include <string>
#include <vector>

// Modernized: braces, descriptive variable names, trailing return types, auto, nullptr, one declaration per statement, no implicit conversions

auto expand_wildcards(const std::vector<std::string> &patterns) -> std::vector<std::string>;

auto parse_cli_args(const std::vector<std::string>& arguments) -> CliArgs {
  CliArgs parsed_args;
  for (size_t arg_index = 1; arg_index < arguments.size(); ++arg_index) {
    const std::string& argument = arguments[arg_index];
    if (argument == "--parallel" || argument == "-p") {
      parsed_args.use_parallel = true;
      parsed_args.used_flags.push_back(argument);
      continue;
    }
    if (argument == "--minimize-output" || argument == "-m") {
      parsed_args.minimize_output = true;
      parsed_args.used_flags.push_back(argument);
      continue;
    }
    if (argument == "--no-warmup") {
      parsed_args.warmup = false;
      parsed_args.used_flags.push_back(argument);
      continue;
    }
    if (argument == "--single-core" || argument == "-s") {
      parsed_args.pin_single_core = true;
      parsed_args.used_flags.push_back(argument);
      continue;
    }
    if (argument == "--no-yield") {
      parsed_args.do_yield = false;
      parsed_args.used_flags.push_back(argument);
      continue;
    }
    if (argument == "--no-nice") {
      parsed_args.do_nice = false;
      parsed_args.used_flags.push_back(argument);
      continue;
    }
    if (argument == "--drop-caches") {
      parsed_args.do_drop_caches = true;
      parsed_args.used_flags.push_back(argument);
      continue;
    }
    if (argument == "--mask-sensitive") {
      parsed_args.mask_sensitive = true;
      parsed_args.used_flags.push_back(argument);
      continue;
    }
    if (argument == "--show-sysinfo") {
      parsed_args.show_sysinfo = true;
      parsed_args.used_flags.push_back(argument);
      continue;
    }
    if (argument == "--show-sysinfo-only") {
      parsed_args.show_sysinfo_only = true;
      continue;
    }
    if (argument == "--json") {
      parsed_args.output_json = true;
      parsed_args.used_flags.push_back(argument);
      continue;
    }
    if (argument == "--summary-table") {
      parsed_args.summary_table = true;
      std::vector<std::string> summary_patterns{};
      for (size_t pattern_index = arg_index + 1; pattern_index < arguments.size(); ++pattern_index) {
        summary_patterns.push_back(arguments[pattern_index]);
      }
      parsed_args.summary_files = expand_wildcards(summary_patterns);
      parsed_args.summary_files.erase(
          std::remove_if(parsed_args.summary_files.begin(), parsed_args.summary_files.end(),
                         [](const std::string &filename) {
                           return filename.find(SUMMARY_JSON_FILENAME) !=
                                  std::string::npos;
                         }),
          parsed_args.summary_files.end());
      break;
    }
    if (argument == "--debug") {
      parsed_args.debug_mode = true;
      continue;
    }
    if (argument == "--diagnostics") {
      parsed_args.diagnostics_mode = true;
      continue;
    }
    if (argument == "--help" || argument == "-h") {
      continue;
    }
    std::cerr << "[WARN] Unknown flag: " << argument << std::endl;
  }
  return parsed_args;
}

auto expand_wildcards(const std::vector<std::string> &patterns) -> std::vector<std::string> {
  std::vector<std::string> files{};
  for (const auto &pat : patterns) {
    // If no wildcard, just add
    if (pat.find('*') == std::string::npos &&
        pat.find('?') == std::string::npos) {
      files.push_back(pat);
      continue;
    }
    // Split path and pattern
    size_t slash = pat.find_last_of("/");
    std::string dir = (slash == std::string::npos) ? "." : pat.substr(0, slash);
    std::string pattern =
        (slash == std::string::npos) ? pat : pat.substr(slash + 1);
    DIR *dp = opendir(dir.c_str());
    if (!dp)
      continue;
    struct dirent *ep = nullptr;
    while ((ep = readdir(dp))) {
      if (fnmatch(pattern.c_str(), static_cast<const char*>(ep->d_name), 0) == 0) {
        std::string full = (dir == ".") ? std::string(static_cast<const char*>(ep->d_name)) : dir + "/" + std::string(static_cast<const char*>(ep->d_name));
        files.push_back(full);
      }
    }
    closedir(dp);
  }
  return files;
}
