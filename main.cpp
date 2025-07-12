#include "main.h"
#include "cli_args.h"
#include "output.h"
#include "json_helpers.h"
#include "diagnostics.h"
#include "sysinfo.h"
#include "benchmarks.h"
#include "types.h"
#include "stats.h"
#include <iostream>

// Version macro: build date and time (format: "Jul 11 2025 03:23:25")
const char* BUILD_VERSION = __DATE__ " " __TIME__;

// Constant for summary JSON filename (filename only)
const char* SUMMARY_JSON_FILENAME = "summary.json";

void print_help() {
    printf("Usage: SpeedCloudflareCli [options]\n");
    printf("  --parallel, -p           Use parallel download tests (default: off)\n");
    printf("  --minimize-output, -m    Minimize output/logging (default: off)\n");
    printf("  --no-warmup              Disable network warm-up phase (default: on)\n");
    printf("  --single-core, -s        Pin process to a single core (default: off)\n");
    printf("  --no-yield               Do not yield (sleep) between test iterations (default: yield on)\n");
    printf("  --no-nice                Do not lower process priority (default: nice on)\n");
    printf("  --drop-caches            Drop Linux FS caches before test (default: off, root only)\n");
    printf("  --mask-sensitive         Mask the last part of your IP address and hostname in output (default: off)\n");
    printf("  --show-flags-used        Print a line at the end with all explicitly set flags (default: off)\n");
    printf("  --show-sysinfo           Show basic host architecture, CPU, and memory info (default: off)\n");
    printf("  --show-sysinfo-only      Only print system info and exit (supports --mask-sensitive)\n");
    printf("  --json                   Output results as JSON to stdout (default: off)\n");
    printf("  --summary-table FILES    Print a summary table comparing multiple JSON result files\n");
    printf("  --debug                  Show debug output (default: off)\n");
    printf("  --help, -h               Show this help message\n");
}

int main(int argc, char* argv[]) {
    CliArgs args = parse_cli_args(argc, argv);
    if (args.summary_table) {
        if (args.summary_files.empty()) {
            std::cerr << "[ERROR] --summary-table requires at least one JSON file as argument." << std::endl;
            return 1;
        }
        for (const auto& file : args.summary_files) {
            validate_json_schema(file, "result.schema.json", args.diagnostics_mode);
        }
        auto results = load_summary_results(args.summary_files, args.diagnostics_mode, args.debug_mode);
        print_summary_table(results);
        std::string summary_path = std::string("results/") + SUMMARY_JSON_FILENAME;
        write_summary_json(results, summary_path, args.diagnostics_mode, args.debug_mode);
        validate_json_schema(summary_path, "summary.schema.json", args.diagnostics_mode);
        if (args.diagnostics_mode) yyjson_minimal_test(args.diagnostics_mode, args.debug_mode);
        return 0;
    }
    if (args.show_sysinfo_only) {
        print_sysinfo(args.mask_sensitive);
        return 0;
    }
    if (args.show_sysinfo) print_sysinfo(args.mask_sensitive);
    if (args.pin_single_core) pin_to_core(0);
    if (args.do_nice) set_nice();
    if (args.do_drop_caches) drop_caches();
    if (args.output_json) {
        TestResults results;
        speed_test(args.use_parallel, args.minimize_output, args.warmup, args.do_yield, args.mask_sensitive, args.output_json, &results);
        std::cout << serialize_to_json(results) << std::endl;
    } else {
        speed_test(args.use_parallel, args.minimize_output, args.warmup, args.do_yield, args.mask_sensitive, args.output_json);
    }
    if (args.debug_mode || args.diagnostics_mode) yyjson_minimal_test(args.diagnostics_mode, args.debug_mode);
    return 0;
}
