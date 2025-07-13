#include "sysinfo.h"
#include "types.h"
#include <chrono>
#include <array>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <fstream>
#include <sched.h>
#include <string>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <thread>
#include <vector>

constexpr size_t kDatetimeLen = 64;
constexpr double kMemGBDiv = 1048576.0;
constexpr double kMemMBDiv = 1024.0;
constexpr int kNiceValue = 10;
constexpr int kYieldMs = 10;
constexpr double kMsPerSecond = 1000.0;
constexpr double kNsPerMs = 1e6;

void print_sysinfo(bool mask_sensitive) {
  std::array<char, kDatetimeLen> datetime{};
  time_t now = time(NULL);
  struct tm tm{};
  localtime_r(&now, &tm);
  strftime(datetime.data(), datetime.size(), "%Y-%m-%dT%H:%M:%S%z", &tm);
  std::cout << "[SYS] Date: " << datetime.data() << std::endl;
  std::cout << "[SYS] Version: " << BUILD_VERSION << std::endl;
  struct utsname uts{};
  if (uname(&uts) == 0) {
    std::string host(uts.nodename);
    if (mask_sensitive && !host.empty()) {
      size_t len = host.length();
      if (len > 3)
        host.replace(len / 2, len - len / 2, std::string(len - len / 2, '*'));
    }
    std::cout << "[SYS] Host: " << host << std::endl;
    std::cout << "[SYS] Arch: " << std::string(uts.machine) << std::endl;
    std::cout << "[SYS] Kernel: " << std::string(uts.sysname) << " " << std::string(uts.release) << std::endl;
  }
  std::ifstream cpuinfo("/proc/cpuinfo");
  std::string line, model;
  int cores = 0;
  while (std::getline(cpuinfo, line)) {
    if (line.find("model name") != std::string::npos) {
      if (model.empty())
        model = line.substr(line.find(":") + 2);
      ++cores;
    }
  }
  if (!model.empty())
    std::cout << "[SYS] CPU: " << model << " (" << cores << " cores)" << std::endl;
  std::ifstream meminfo("/proc/meminfo");
  std::string memline;
  while (std::getline(meminfo, memline)) {
    if (memline.find("MemTotal:") == 0) {
      std::string memval = memline.substr(memline.find(":") + 1);
      size_t num_start = memval.find_first_of("0123456789");
      size_t num_end = memval.find_first_not_of("0123456789", num_start);
      long mem_kb = 0;
      if (num_start != std::string::npos) {
        mem_kb = std::stol(memval.substr(num_start, num_end - num_start));
      }
      double mem_gb = static_cast<double>(mem_kb) / kMemGBDiv;
      double mem_mb = static_cast<double>(mem_kb) / kMemMBDiv;
      if (mem_gb >= 1.0)
        std::cout << "[SYS] Mem: " << std::fixed << std::setprecision(1) << mem_gb << " GB" << std::endl;
      else
        std::cout << "[SYS] Mem: " << std::fixed << std::setprecision(0) << mem_mb << " MB" << std::endl;
      break;
    }
  }
}

std::string mask_str(const std::string &s) {
  if (s.empty())
    return s;
  size_t pos = s.find_last_of('.');
  if (pos != std::string::npos)
    return s.substr(0, pos + 1) + "***";
  pos = s.find_last_of(':');
  if (pos != std::string::npos)
    return s.substr(0, pos + 1) + "****";
  size_t len = s.length();
  if (len > 3)
    return s.substr(0, len / 2) + std::string(len - len / 2, '*');
  return s;
}

void collect_sysinfo(TestResults &res, bool mask_sensitive) {
  std::array<char, kDatetimeLen> datetime{};
  time_t now = time(NULL);
  struct tm tm{};
  localtime_r(&now, &tm);
  strftime(datetime.data(), datetime.size(), "%Y-%m-%dT%H:%M:%S%z", &tm);
  res.sysinfo_date = datetime.data();
  res.version = BUILD_VERSION;
  struct utsname uts{};
  if (uname(&uts) == 0) {
    res.host = mask_sensitive ? mask_str(std::string(uts.nodename)) : std::string(uts.nodename);
    res.arch = std::string(uts.machine);
    res.kernel = std::string(uts.sysname) + " " + std::string(uts.release);
  }
  std::ifstream cpuinfo("/proc/cpuinfo");
  std::string line, model;
  int cores = 0;
  while (std::getline(cpuinfo, line)) {
    if (line.find("model name") != std::string::npos) {
      if (model.empty())
        model = line.substr(line.find(":") + 2);
      ++cores;
    }
  }
  res.cpu_model = model;
  res.cpu_cores = cores;
  std::ifstream meminfo("/proc/meminfo");
  std::string memline;
  while (std::getline(meminfo, memline)) {
    if (memline.find("MemTotal:") == 0) {
      res.mem_total = memline.substr(memline.find(":") + 1);
      break;
    }
  }
}

void pin_to_core(int core_id) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core_id, &cpuset);
  sched_setaffinity(0, sizeof(cpuset), &cpuset);
}

void set_nice() { setpriority(PRIO_PROCESS, 0, kNiceValue); }

void drop_caches() {
  FILE *f = fopen("/proc/sys/vm/drop_caches", "w");
  if (f) {
    fputs("3\n", f);
    fclose(f);
  }
}

double get_time_ms() {
  struct timespec ts{};
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<double>(ts.tv_sec) * kMsPerSecond + static_cast<double>(ts.tv_nsec) / kNsPerMs;
}

void yield_cpu() { std::this_thread::sleep_for(std::chrono::milliseconds(kYieldMs)); }
