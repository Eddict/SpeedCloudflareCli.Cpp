#include "main.h"
#include "benchmarks.h"
#include "cli_args.h"
#include "diagnostics.h"
#include "json_helpers.h"
#include "output.h"
#include "stats.h"
#include "sysinfo.h"
#include "types.h"
#include <iostream>
#include <string_view>

// Modernized: trailing return types, braces, descriptive variable names, auto, nullptr, one declaration per statement, no implicit conversions

void print_help() {
  std::cout << "Usage: SpeedCloudflareCli [options]\n";
  std::cout << "  --parallel, -p           Use parallel download tests "
               "(default: off)\n";
  std::cout
      << "  --minimize-output, -m    Minimize output/logging (default: off)\n";
  std::cout << "  --no-warmup              Disable network warm-up phase "
               "(default: on)\n";
  std::cout << "  --single-core, -s        Pin process to a single core "
               "(default: off)\n";
  std::cout << "  --no-yield               Do not yield (sleep) between test "
               "iterations (default: yield on)\n";
  std::cout << "  --no-nice                Do not lower process priority "
               "(default: nice on)\n";
  std::cout << "  --drop-caches            Drop Linux FS caches before test "
               "(default: off, root only)\n";
  std::cout << "  --mask-sensitive         Mask the last part of your IP "
               "address and hostname in output (default: off)\n";
  std::cout << "  --show-flags-used        Print a line at the end with all "
               "explicitly set flags (default: off)\n";
  std::cout << "  --show-sysinfo           Show basic host architecture, CPU, "
               "and memory info (default: off)\n";
  std::cout << "  --show-sysinfo-only      Only print system info and exit "
               "(supports --mask-sensitive)\n";
  std::cout << "  --json                   Output results as JSON to stdout "
               "(default: off)\n";
  std::cout << "  --summary-table FILES    Print a summary table comparing "
               "multiple JSON result files\n";
  std::cout << "  --debug                  Show debug output (default: off)\n";
  std::cout << "  --help, -h               Show this help message\n";
}

auto main(int argc, char *argv[]) -> int {
  std::vector<std::string> argument_vector(argv, argv + argc);
  CliArgs args = parse_cli_args(argument_vector);
  if (args.summary_table) {
    if (args.summary_files.empty()) {
      std::cerr << "[ERROR] --summary-table requires at least one JSON file as "
                   "argument."
                << std::endl;
      return 1;
    }
    for (const auto &summary_file : args.summary_files) {
      validate_json_schema(summary_file, "result.schema.json", args.diagnostics_mode);
    }
    auto summary_results = load_summary_results(args.summary_files,
                                               args.diagnostics_mode, args.debug_mode);
    print_summary_table(summary_results);
    std::string summary_path = std::string("results/") + SUMMARY_JSON_FILENAME;
    write_summary_json(summary_results, summary_path, args.diagnostics_mode,
                       args.debug_mode);
    validate_json_schema(summary_path, "summary.schema.json",
                         args.diagnostics_mode);
    if (args.diagnostics_mode) {
      yyjson_minimal_test(args.diagnostics_mode, args.debug_mode);
    }
    return 0;
  }
  if (args.show_sysinfo_only) {
    print_sysinfo(args.mask_sensitive);
    return 0;
  }
  if (args.show_sysinfo) {
    print_sysinfo(args.mask_sensitive);
  }
  if (args.pin_single_core) {
    pin_to_core(0);
  }
  if (args.do_nice) {
    set_nice();
  }
  if (args.do_drop_caches) {
    drop_caches();
  }
  if (args.output_json) {
    TestResults results;
    speed_test(args.use_parallel, args.minimize_output, args.warmup,
               args.do_yield, args.mask_sensitive, args.output_json, &results);
    std::cout << serialize_to_json(results) << std::endl;
  } else {
    speed_test(args.use_parallel, args.minimize_output, args.warmup,
               args.do_yield, args.mask_sensitive, args.output_json);
  }
  if (args.debug_mode || args.diagnostics_mode) {
    yyjson_minimal_test(args.diagnostics_mode, args.debug_mode);
  }
  return 0;
}
