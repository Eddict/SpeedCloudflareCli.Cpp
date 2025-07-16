# SpeedCloudflareCli

A minimal, flexible, and low-overhead command-line tool for benchmarking Cloudflare network speed. Designed for accurate measurements with minimal CPU and memory impact, it supports advanced benchmarking options for both single-core and multi-core systems.

## Features
- Download and upload speed tests using Cloudflare's speed endpoints
- Latency and jitter measurement
- Parallel or sequential benchmarking
- Minimal output mode for scripting/automation
- Network stack warm-up (optional)
- Process pinning to a single CPU core (optional)
- CPU yielding between tests (optional)
- Process niceness adjustment (optional)
- Linux filesystem cache dropping (optional, root only)
- Comprehensive help output

## Usage
```
./SpeedCloudflareCli [options]
```

## Command-Line Options

| Option                  | Short | Description                                                                 |
|-------------------------|-------|-----------------------------------------------------------------------------|
| `--parallel`            | `-p`  | Use parallel download tests (default: off)                                  |
| `--minimize-output`     | `-m`  | Minimize output/logging (default: off)                                      |
| `--no-warmup`           |       | Disable network warm-up phase (default: on)                                 |
| `--single-core`         | `-s`  | Pin process to a single core (default: off)                                 |
| `--no-yield`            |       | Do not yield (sleep) between test iterations (default: yield on)            |
| `--no-nice`             |       | Do not lower process priority (default: nice on)                            |
| `--drop-caches`         |       | Drop Linux FS caches before test (default: off, root only)                  |
| `--mask-sensitive`      |       | Mask the last part of your IP address and hostname in output (default: off) |
| `--show-flags-used`     |       | Print a line at the end with all explicitly set flags (default: off)        |
| `--show-sysinfo`        |       | Show basic host architecture, CPU, and memory info (default: off)           |
| `--show-sysinfo-only`   |       | Only print system info and exit (supports --mask-sensitive)                 |
| `--json`                |       | Output results as JSON to stdout (default: off)                             |
| `--summary-table FILES` |       | Print a summary table comparing multiple JSON result files                   |
| `-v`, `-vv`, `-vvv`     |       | Increase verbosity: -v (debug), -vv (diagnostics), -vvv (full diagnostics)  |
| `--verbose[=N]`         |       | Set verbosity level (1=debug, 2=diagnostics, 3=full diagnostics)            |
| `--help`                | `-h`  | Show help message and exit                                                  |

### Default Behavior
- Runs sequential download/upload tests
- Performs a network warm-up phase
- Yields CPU between test iterations
- Lowers process priority (nice)
- Does **not** drop filesystem caches
- Provides detailed output

### Example
Run a parallel test with minimal output, pinning to a single core:
```
./SpeedCloudflareCli -p -m -s
```
Run with debug output enabled:
```
./SpeedCloudflareCli -v --summary-table results/*.json
```
Run with diagnostics:
```
./SpeedCloudflareCli -vv --summary-table results/*.json
```
Run with full diagnostics:
```
./SpeedCloudflareCli -vvv --summary-table results/*.json
```

## Build Instructions

Native build:
```
cmake . && make -j
```

Cross-compile for ARM (example):
```
CC=arm-linux-gnueabihf-gcc cmake . && make -j
```

## Requirements
- Linux
- libcurl
- yyjson

## License
See `COPYING`.

### For the latest options, run:
```
./SpeedCloudflareCli --help
```
