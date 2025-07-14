#pragma once
#include "types.h"
#include <string>

// System info helpers
// Modernized: trailing return types, descriptive parameter names
auto print_sysinfo(bool mask_sensitive) -> void;
auto collect_sysinfo(TestResults& results, bool mask_sensitive) -> void;
auto mask_str(const std::string& input) -> std::string;
auto pin_to_core(int core_id) -> void;
auto set_nice() -> void;
auto drop_caches() -> void;
auto get_time_ms() -> double;
auto yield_cpu() -> void;
