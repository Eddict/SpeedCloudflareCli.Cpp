#pragma once
#include "types.h"
#include <string>

// System info helpers
typedef struct TestResults TestResults;
void print_sysinfo(bool mask_sensitive);
void collect_sysinfo(TestResults &res, bool mask_sensitive);
std::string mask_str(const std::string &s);
void pin_to_core(int core_id);
void set_nice();
void drop_caches();
double get_time_ms();
void yield_cpu();
