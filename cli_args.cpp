#include "cli_args.h"
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <fnmatch.h>
#include <dirent.h>

extern const char* SUMMARY_JSON_FILENAME;
std::vector<std::string> expand_wildcards(const std::vector<std::string>& patterns);

CliArgs parse_cli_args(int argc, char* argv[]) {
    CliArgs args;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        bool recognized = false;
        if (arg == "--parallel" || arg == "-p") { args.use_parallel = true; args.used_flags.push_back(arg); recognized = true; }
        if (arg == "--minimize-output" || arg == "-m") { args.minimize_output = true; args.used_flags.push_back(arg); recognized = true; }
        if (arg == "--no-warmup") { args.warmup = false; args.used_flags.push_back(arg); recognized = true; }
        if (arg == "--single-core" || arg == "-s") { args.pin_single_core = true; args.used_flags.push_back(arg); recognized = true; }
        if (arg == "--no-yield") { args.do_yield = false; args.used_flags.push_back(arg); recognized = true; }
        if (arg == "--no-nice") { args.do_nice = false; args.used_flags.push_back(arg); recognized = true; }
        if (arg == "--drop-caches") { args.do_drop_caches = true; args.used_flags.push_back(arg); recognized = true; }
        if (arg == "--mask-sensitive") { args.mask_sensitive = true; args.used_flags.push_back(arg); recognized = true; }
        if (arg == "--show-sysinfo") { args.show_sysinfo = true; args.used_flags.push_back(arg); recognized = true; }
        if (arg == "--show-sysinfo-only") { args.show_sysinfo_only = true; recognized = true; }
        if (arg == "--json") { args.output_json = true; args.used_flags.push_back(arg); recognized = true; }
        if (arg == "--summary-table") {
            args.summary_table = true;
            recognized = true;
            std::vector<std::string> patterns;
            for (int j = i + 1; j < argc; ++j) {
                patterns.push_back(argv[j]);
            }
            args.summary_files = expand_wildcards(patterns);
            args.summary_files.erase(
                std::remove_if(args.summary_files.begin(), args.summary_files.end(),
                    [](const std::string& f) {
                        return f.find(SUMMARY_JSON_FILENAME) != std::string::npos;
                    }),
                args.summary_files.end());
            break;
        }
        if (arg == "--debug") { args.debug_mode = true; recognized = true; }
        if (arg == "--diagnostics") { args.diagnostics_mode = true; recognized = true; }
        if (arg == "--help" || arg == "-h") { recognized = true; }
        if (!recognized) {
            std::cerr << "[WARN] Unknown flag: " << arg << std::endl;
        }
    }
    return args;
}

std::vector<std::string> expand_wildcards(const std::vector<std::string>& patterns) {
    std::vector<std::string> files;
    for (const auto& pat : patterns) {
        // If no wildcard, just add
        if (pat.find('*') == std::string::npos && pat.find('?') == std::string::npos) {
            files.push_back(pat);
            continue;
        }
        // Split path and pattern
        size_t slash = pat.find_last_of("/");
        std::string dir = (slash == std::string::npos) ? "." : pat.substr(0, slash);
        std::string pattern = (slash == std::string::npos) ? pat : pat.substr(slash + 1);
        DIR* dp = opendir(dir.c_str());
        if (!dp) continue;
        struct dirent* ep;
        while ((ep = readdir(dp))) {
            if (fnmatch(pattern.c_str(), ep->d_name, 0) == 0) {
                std::string full = (dir == ".") ? ep->d_name : dir + "/" + ep->d_name;
                files.push_back(full);
            }
        }
        closedir(dp);
    }
    return files;
}
