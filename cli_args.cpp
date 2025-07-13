#include "cli_args.h"
#include "types.h"
#include <algorithm>
#include <dirent.h>
#include <fnmatch.h>
#include <iostream>
#include <string>
#include <vector>

std::vector<std::string>
expand_wildcards(const std::vector<std::string> &patterns);

CliArgs parse_cli_args(const std::vector<std::string>& args) {
  CliArgs parsed_args;
  for (size_t i = 1; i < args.size(); ++i) {
    const std::string& arg = args[i];
    if (arg == "--parallel" || arg == "-p") {
      parsed_args.use_parallel = true;
      parsed_args.used_flags.push_back(arg);
      continue;
    }
    if (arg == "--minimize-output" || arg == "-m") {
      parsed_args.minimize_output = true;
      parsed_args.used_flags.push_back(arg);
      continue;
    }
    if (arg == "--no-warmup") {
      parsed_args.warmup = false;
      parsed_args.used_flags.push_back(arg);
      continue;
    }
    if (arg == "--single-core" || arg == "-s") {
      parsed_args.pin_single_core = true;
      parsed_args.used_flags.push_back(arg);
      continue;
    }
    if (arg == "--no-yield") {
      parsed_args.do_yield = false;
      parsed_args.used_flags.push_back(arg);
      continue;
    }
    if (arg == "--no-nice") {
      parsed_args.do_nice = false;
      parsed_args.used_flags.push_back(arg);
      continue;
    }
    if (arg == "--drop-caches") {
      parsed_args.do_drop_caches = true;
      parsed_args.used_flags.push_back(arg);
      continue;
    }
    if (arg == "--mask-sensitive") {
      parsed_args.mask_sensitive = true;
      parsed_args.used_flags.push_back(arg);
      continue;
    }
    if (arg == "--show-sysinfo") {
      parsed_args.show_sysinfo = true;
      parsed_args.used_flags.push_back(arg);
      continue;
    }
    if (arg == "--show-sysinfo-only") {
      parsed_args.show_sysinfo_only = true;
      continue;
    }
    if (arg == "--json") {
      parsed_args.output_json = true;
      parsed_args.used_flags.push_back(arg);
      continue;
    }
    if (arg == "--summary-table") {
      parsed_args.summary_table = true;
      std::vector<std::string> patterns{};
      for (size_t j = i + 1; j < args.size(); ++j) {
        patterns.push_back(args[j]);
      }
      parsed_args.summary_files = expand_wildcards(patterns);
      parsed_args.summary_files.erase(
          std::remove_if(parsed_args.summary_files.begin(), parsed_args.summary_files.end(),
                         [](const std::string &f) {
                           return f.find(SUMMARY_JSON_FILENAME) !=
                                  std::string::npos;
                         }),
          parsed_args.summary_files.end());
      break;
    }
    if (arg == "--debug") {
      parsed_args.debug_mode = true;
      continue;
    }
    if (arg == "--diagnostics") {
      parsed_args.diagnostics_mode = true;
      continue;
    }
    if (arg == "--help" || arg == "-h") {
      continue;
    }
    std::cerr << "[WARN] Unknown flag: " << arg << std::endl;
  }
  return parsed_args;
}

std::vector<std::string>
expand_wildcards(const std::vector<std::string> &patterns) {
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
