#include "sysinfo.h"
#include <bits/chrono.h>           // for milliseconds
#include <sched.h>                 // for sched_setaffinity, cpu_set_t, CPU_SET
#include <stdio.h>                 // for fopen, fputs, FILE, fclose
#include <sys/resource.h>          // for setpriority, PRIO_PROCESS
#include <sys/utsname.h>           // for utsname, uname
#include <array>                   // for array
#include <ctime>                   // for localtime_r, size_t, strftime, time
#include <fstream>                 // IWYU pragma: keep  // for ifstream
#include <iomanip>                 // for operator<<, setprecision
#include <iostream>                // for operator<<, basic_ostream, endl
#include <memory>                  // for unique_ptr
#include <string>                  // for string, allocator, char_traits
#include <string_view>             // for string_view
#include <thread>                  // for sleep_for
#include "types.h"                 // for TestResults, BUILD_VERSION

constexpr size_t kDatetimeLen = 64;
constexpr double kMemGBDiv = 1048576.0;
constexpr double kMemMBDiv = 1024.0;
constexpr int kNiceValue = 10;
constexpr int kYieldMs = 10;
constexpr double kMsPerSecond = 1000.0;
constexpr double kNsPerMs = 1e6;

// Modernized: trailing return types, braces, descriptive variable names, auto, nullptr, one
// declaration per statement, no implicit conversions

void print_sysinfo(bool mask_sensitive)
{
  std::array<char, kDatetimeLen> datetime{};
  const time_t now = time(nullptr);
  struct tm time_info
  {
  };
  localtime_r(&now, &time_info);
  (void)strftime(datetime.data(), datetime.size(), "%Y-%m-%dT%H:%M:%S%z", &time_info);
  std::cout << "[SYS] Date: " << datetime.data() << std::endl;
  std::cout << "[SYS] Version: " << BUILD_VERSION << std::endl;
  struct utsname uts
  {
  };
  if (uname(&uts) == 0)
  {
    std::string_view nodename_sv(
        static_cast<const char*>(uts.nodename),
        std::char_traits<char>::length(static_cast<const char*>(uts.nodename)));
    std::string_view machine_sv(
        static_cast<const char*>(uts.machine),
        std::char_traits<char>::length(static_cast<const char*>(uts.machine)));
    std::string_view sysname_sv(
        static_cast<const char*>(uts.sysname),
        std::char_traits<char>::length(static_cast<const char*>(uts.sysname)));
    std::string_view release_sv(
        static_cast<const char*>(uts.release),
        std::char_traits<char>::length(static_cast<const char*>(uts.release)));
    std::string host(nodename_sv);
    if (mask_sensitive && !host.empty())
    {
      const size_t host_len = host.length();
      if (host_len > 3)
      {
        host.replace(host_len / 2, host_len - host_len / 2,
                     std::string(host_len - host_len / 2, '*'));
      }
    }
    std::cout << "[SYS] Host: " << host << std::endl;
    std::cout << "[SYS] Arch: " << std::string(machine_sv) << std::endl;
    std::cout << "[SYS] Kernel: " << std::string(sysname_sv) << " " << std::string(release_sv)
              << std::endl;
  }
  std::ifstream cpuinfo("/proc/cpuinfo");
  std::string line;
  std::string model;
  int core_count = 0;
  while (std::getline(cpuinfo, line))
  {
    if (line.find("model name") != std::string::npos)
    {
      if (model.empty())
      {
        model = line.substr(line.find(':') + 2);
      }
      ++core_count;
    }
  }
  if (!model.empty())
  {
    std::cout << "[SYS] CPU: " << model << " (" << core_count << " cores)" << std::endl;
  }
  std::ifstream meminfo("/proc/meminfo");
  std::string memline;
  while (std::getline(meminfo, memline))
  {
    if (memline.find("MemTotal:") == 0)
    {
      std::string memval = memline.substr(memline.find(':') + 1);
      const size_t num_start = memval.find_first_of("0123456789");
      const size_t num_end = memval.find_first_not_of("0123456789", num_start);
      long mem_kb = 0;
      if (num_start != std::string::npos)
      {
        mem_kb = std::stol(memval.substr(num_start, num_end - num_start));
      }
      const double mem_gb = static_cast<double>(mem_kb) / kMemGBDiv;
      const double mem_mb = static_cast<double>(mem_kb) / kMemMBDiv;
      if (mem_gb >= 1.0)
      {
        std::cout << "[SYS] Mem: " << std::fixed << std::setprecision(1) << mem_gb << " GB"
                  << std::endl;
      }
      else
      {
        std::cout << "[SYS] Mem: " << std::fixed << std::setprecision(0) << mem_mb << " MB"
                  << std::endl;
      }
      break;
    }
  }
}

auto mask_str(const std::string& input) -> std::string
{
  if (input.empty())
  {
    return input;
  }
  size_t pos = input.find_last_of('.');
  if (pos != std::string::npos)
  {
    return input.substr(0, pos + 1) + "***";
  }
  pos = input.find_last_of(':');
  if (pos != std::string::npos)
  {
    return input.substr(0, pos + 1) + "****";
  }
  const size_t input_len = input.length();
  if (input_len > 3)
  {
    return input.substr(0, input_len / 2) + std::string(input_len - input_len / 2, '*');
  }
  return input;
}

void collect_sysinfo(TestResults& res, bool mask_sensitive)
{
  std::array<char, kDatetimeLen> datetime{};
  time_t now = time(nullptr);
  struct tm time_info
  {
  };
  localtime_r(&now, &time_info);
  (void)strftime(datetime.data(), datetime.size(), "%Y-%m-%dT%H:%M:%S%z", &time_info);
  res.sysinfo_date = datetime.data();
  res.version = BUILD_VERSION;
  struct utsname uts
  {
  };
  if (uname(&uts) == 0)
  {
    std::string_view nodename_sv(
        static_cast<const char*>(uts.nodename),
        std::char_traits<char>::length(static_cast<const char*>(uts.nodename)));
    std::string_view machine_sv(
        static_cast<const char*>(uts.machine),
        std::char_traits<char>::length(static_cast<const char*>(uts.machine)));
    std::string_view sysname_sv(
        static_cast<const char*>(uts.sysname),
        std::char_traits<char>::length(static_cast<const char*>(uts.sysname)));
    std::string_view release_sv(
        static_cast<const char*>(uts.release),
        std::char_traits<char>::length(static_cast<const char*>(uts.release)));
    res.host = mask_sensitive ? mask_str(std::string(nodename_sv)) : std::string(nodename_sv);
    res.arch = std::string(machine_sv);
    res.kernel = std::string(sysname_sv) + " " + std::string(release_sv);
  }
  std::ifstream cpuinfo("/proc/cpuinfo");
  std::string line;
  std::string model;
  int core_count = 0;
  while (std::getline(cpuinfo, line))
  {
    if (line.find("model name") != std::string::npos)
    {
      if (model.empty())
      {
        model = line.substr(line.find(':') + 2);
      }
      ++core_count;
    }
  }
  res.cpu_model = model;
  res.cpu_cores = core_count;
  std::ifstream meminfo("/proc/meminfo");
  std::string memline;
  while (std::getline(meminfo, memline))
  {
    if (memline.find("MemTotal:") == 0)
    {
      res.mem_total = memline.substr(memline.find(':') + 1);
      break;
    }
  }
}

auto get_time_ms() -> double
{
  struct timespec time_spec
  {
  };
  clock_gettime(CLOCK_MONOTONIC, &time_spec);
  return static_cast<double>(time_spec.tv_sec) * kMsPerSecond +
         static_cast<double>(time_spec.tv_nsec) / kNsPerMs;
}

void drop_caches()
{
  using file_ptr_t = std::unique_ptr<FILE, int(*)(FILE*)>;
  file_ptr_t file(fopen("/proc/sys/vm/drop_caches", "w"), fclose);
  if (file)
  {
    (void)fputs("3\n", file.get());
  }
}

void pin_to_core(int core_id)
{
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core_id, &cpuset);
  sched_setaffinity(0, sizeof(cpuset), &cpuset);
}

void set_nice() { setpriority(PRIO_PROCESS, 0, kNiceValue); }

void yield_cpu() { std::this_thread::sleep_for(std::chrono::milliseconds(kYieldMs)); }
