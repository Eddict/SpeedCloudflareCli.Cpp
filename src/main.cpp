#include "main.h"
#include <iostream>        // for operator<<, ostream, cout, endl, basic_ost...
#include <string>          // for string, basic_string, allocator, operator+
#include <vector>          // for vector
#include "benchmarks.h"    // for speed_test
#include "cli_args.h"      // for CliArgs, parse_cli_args
#include "diagnostics.h"   // for validate_json_schema, yyjson_minimal_test
#include "json_helpers.h"  // for serialize_to_json
#include "output.h"        // for load_summary_results, print_summary_table
#include "sysinfo.h"       // for print_sysinfo, drop_caches, pin_to_core
#include "types.h"         // for SUMMARY_JSON_FILENAME, TestResults

// Modernized: trailing return types, braces, descriptive variable names, auto, nullptr, one
// declaration per statement, no implicit conversions

void print_help()
{
  std::cout << "Usage: SpeedCloudflareCli [options]\n";
  std::cout << "  --parallel, -p           Use parallel download tests (default: off)\n";
  std::cout << "  --minimize-output, -m    Minimize output/logging (default: off)\n";
  std::cout << "  --no-warmup              Disable network warm-up phase (default: on)\n";
  std::cout << "  --single-core, -s        Pin process to a single core (default: off)\n";
  std::cout << "  --no-yield               Do not yield (sleep) between test iterations (default: yield on)\n";
  std::cout << "  --no-nice                Do not lower process priority (default: nice on)\n";
  std::cout << "  --drop-caches            Drop Linux FS caches before test (default: off, root only)\n";
  std::cout << "  --mask-sensitive         Mask the last part of your IP address and hostname in output (default: off)\n";
  std::cout << "  --show-flags-used        Print a line at the end with all explicitly set flags (default: off)\n";
  std::cout << "  --show-sysinfo           Show basic host architecture, CPU, and memory info (default: off)\n";
  std::cout << "  --show-sysinfo-only      Only print system info and exit (supports --mask-sensitive)\n";
  std::cout << "  --json                   Output results as JSON to stdout (default: off)\n";
  std::cout << "  --summary-table FILES    Print a summary table comparing multiple JSON result files\n";
  std::cout << "  -v, --verbose[=N]        Increase verbosity: -v or --verbose=1 for debug, -vv or --verbose=2 for diagnostics, -vvv or --verbose=3 for full diagnostics\n";
  std::cout << "  --help, -h               Show this help message\n";
}

auto main(int argc, char* argv[]) -> int
{
  std::vector<std::string> argument_vector(argv, argv + argc);
  CliArgs args = parse_cli_args(argument_vector);
  if (args.summary_table)
  {
    if (args.summary_files.empty())
    {
      std::cerr << "[ERROR] --summary-table requires at least one JSON file as "
                   "argument."
                << std::endl;
      return 1;
    }
    for (const auto& summary_file : args.summary_files)
    {
      validate_json_schema(summary_file, "result.schema.json", args.is_diagnostics);
    }
    auto summary_results =
        load_summary_results(args.summary_files, args.is_diagnostics, args.is_debug);
    print_summary_table(summary_results);
    std::string summary_path = std::string("results/") + SUMMARY_JSON_FILENAME;
    write_summary_json(summary_results, summary_path, args.is_diagnostics, args.is_debug, args.is_full_diagnostics);
    validate_json_schema(summary_path, "summary.schema.json", args.is_diagnostics);
    if (args.is_diagnostics)
    {
      yyjson_minimal_test(args.is_diagnostics, args.is_debug);
    }
    return 0;
  }
  if (args.show_sysinfo_only)
  {
    print_sysinfo(args.mask_sensitive);
    return 0;
  }
  if (args.show_sysinfo)
  {
    print_sysinfo(args.mask_sensitive);
  }
  if (args.pin_single_core)
  {
    pin_to_core(0);
  }
  if (args.do_nice)
  {
    set_nice();
  }
  if (args.do_drop_caches)
  {
    drop_caches();
  }
  if (args.output_json)
  {
    TestResults results;
    speed_test(args.use_parallel, args.minimize_output, args.warmup, args.do_yield,
               args.mask_sensitive, args.output_json, &results);
    std::cout << serialize_to_json(results) << std::endl;
  }
  else
  {
    speed_test(args.use_parallel, args.minimize_output, args.warmup, args.do_yield,
               args.mask_sensitive, args.output_json);
  }
  if (args.is_debug || args.is_diagnostics)
  {
    yyjson_minimal_test(args.is_diagnostics, args.is_debug);
  }
  return 0;
}
