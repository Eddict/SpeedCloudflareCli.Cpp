#!/bin/bash
# SpeedCloudflareCli Benchmark Script
# Runs a variety of test scenarios to analyze CPU/mem impact and generates a summary table.

set -e

BIN=./SpeedCloudflareCli
OUTDIR=results
mkdir -p "$OUTDIR"

# Parse extra flags for summary table (e.g., --debug, --diagnostics)
SUMMARY_FLAGS=""
while [[ $# -gt 0 ]]; do
  case $1 in
    --debug|--diagnostics)
      SUMMARY_FLAGS+=" $1"
      shift
      ;;
    *)
      break
      ;;
  esac
done

# Remove old JSON files before running tests
rm -f "$OUTDIR"/*.json

# Run show-sysinfo-only (not a test)
SYSINFO_LABEL="show-sysinfo-only"
SYSINFO_FLAGS="--show-sysinfo-only --mask-sensitive"
$BIN $SYSINFO_FLAGS | tee "$OUTDIR/$SYSINFO_LABEL.txt"

# Test matrix: (flagset, description)
declare -a TESTS=(
  "'' 'default'"
  "'--minimize-output' 'minout'"
  "'--single-core' '1core'"
  "'--no-yield' 'noyield'"
  "'--no-nice' 'nonice'"
  "'--drop-caches' 'dropcache'"
  "'--parallel' 'parallel'"
  "'--parallel --minimize-output' 'par+minout'"
  "'--parallel --single-core' 'par+1core'"
  "'--parallel --no-yield' 'par+noyield'"
  "'--parallel --no-nice' 'par+nonice'"
  "'--parallel --drop-caches' 'par+dropcache'"
)

echo "[INFO] Running all test scenarios..."
for test in "${TESTS[@]}"; do
  eval set -- $test
  FLAGS="$1"
  LABEL="$2"
  OUTFILE="$OUTDIR/$LABEL.json"
  echo "[TEST] $LABEL ($FLAGS) -> $OUTFILE"
  $BIN $FLAGS --json --mask-sensitive > "$OUTFILE"
done

echo "[INFO] All tests complete. Generating summary table:"
$BIN $SUMMARY_FLAGS --summary-table $OUTDIR/*.json
