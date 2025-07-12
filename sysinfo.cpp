#include "sysinfo.h"
#include "types.h"
#include <cstdio>
#include <ctime>
#include <string>
#include <fstream>
#include <sys/utsname.h>
#include <vector>
#include <sched.h>
#include <sys/resource.h>
#include <thread>
#include <chrono>

void print_sysinfo(bool mask_sensitive) {
    char datetime[64];
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    strftime(datetime, sizeof(datetime), "%Y-%m-%dT%H:%M:%S%z", &tm);
    printf("[SYS] Date: %s\n", datetime);
    printf("[SYS] Version: %s\n", BUILD_VERSION);
    struct utsname uts;
    if (uname(&uts) == 0) {
        std::string host = uts.nodename;
        if (mask_sensitive && !host.empty()) {
            size_t len = host.length();
            if (len > 3) host.replace(len/2, len-len/2, std::string(len-len/2, '*'));
        }
        printf("[SYS] Host: %s\n", host.c_str());
        printf("[SYS] Arch: %s\n", uts.machine);
        printf("[SYS] Kernel: %s %s\n", uts.sysname, uts.release);
    }
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line, model;
    int cores = 0;
    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") != std::string::npos) {
            if (model.empty()) model = line.substr(line.find(":") + 2);
            ++cores;
        }
    }
    if (!model.empty()) printf("[SYS] CPU: %s (%d cores)\n", model.c_str(), cores);
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
            double mem_gb = mem_kb / 1048576.0;
            double mem_mb = mem_kb / 1024.0;
            if (mem_gb >= 1.0)
                printf("[SYS] Mem: %.1f GB\n", mem_gb);
            else
                printf("[SYS] Mem: %.0f MB\n", mem_mb);
            break;
        }
    }
}

std::string mask_str(const std::string& s) {
    if (s.empty()) return s;
    size_t pos = s.find_last_of('.');
    if (pos != std::string::npos) return s.substr(0, pos+1) + "***";
    pos = s.find_last_of(':');
    if (pos != std::string::npos) return s.substr(0, pos+1) + "****";
    size_t len = s.length();
    if (len > 3) return s.substr(0, len/2) + std::string(len-len/2, '*');
    return s;
}

void collect_sysinfo(TestResults& res, bool mask_sensitive) {
    char datetime[64];
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    strftime(datetime, sizeof(datetime), "%Y-%m-%dT%H:%M:%S%z", &tm);
    res.sysinfo_date = datetime;
    res.version = BUILD_VERSION;
    struct utsname uts;
    if (uname(&uts) == 0) {
        res.host = mask_sensitive ? mask_str(uts.nodename) : uts.nodename;
        res.arch = uts.machine;
        res.kernel = std::string(uts.sysname) + " " + uts.release;
    }
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line, model;
    int cores = 0;
    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") != std::string::npos) {
            if (model.empty()) model = line.substr(line.find(":") + 2);
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

void set_nice() {
    setpriority(PRIO_PROCESS, 0, 10);
}

void drop_caches() {
    FILE* f = fopen("/proc/sys/vm/drop_caches", "w");
    if (f) {
        fputs("3\n", f);
        fclose(f);
    }
}

double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

void yield_cpu() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}
