#!/bin/bash
# SpeedCloudflareCli Benchmark Script
# Runs a variety of test scenarios to analyze CPU/mem impact and generates a summary table.

set -e

BIN=./SpeedCloudflareCli
OUTDIR=results
mkdir -p "$OUTDIR"

# Parse extra flags for summary table (e.g., -v, -vv, -vvv, --verbose=N)
SUMMARY_FLAGS=""
USE_PERF=0
USE_GPROF=0
VERBOSE_LEVEL=0
while [[ $# -gt 0 ]]; do
  case $1 in
    -vvv)
      VERBOSE_LEVEL=3
      shift
      ;;
    -vv)
      VERBOSE_LEVEL=2
      shift
      ;;
    -v|--verbose)
      VERBOSE_LEVEL=1
      shift
      ;;
    --verbose=*)
      VERBOSE_LEVEL="${1#--verbose=}"
      shift
      ;;
    --perf)
      USE_PERF=1
      shift
      ;;
    --gprof)
      USE_GPROF=1
      shift
      ;;
    *)
      break
      ;;
  esac
done

if [[ $VERBOSE_LEVEL -gt 0 ]]; then
  SUMMARY_FLAGS+=" --verbose=$VERBOSE_LEVEL"
fi

# Remove old JSON files before running tests
rm -f "$OUTDIR"/*.json

GPROF_SUMMARY_FILE="$OUTDIR/gprof_summary.txt"
if [[ $USE_GPROF -eq 1 ]]; then
  # Remove old profiling files before running tests
  rm -f "$OUTDIR"/*.gmon.out
  # Remove old gprof files before running tests
  rm -f "$OUTDIR"/*.analysis.txt
  # Remove summary file
  rm -f "$GPROF_SUMMARY_FILE"
fi

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

PERF_SUMMARY_FILE="$OUTDIR/perf_summary.txt"
# Check if perf is available if --perf is requested
if [[ $USE_PERF -eq 1 ]]; then
  # Remove summary file
  rm -f "$PERF_SUMMARY_FILE"
  if ! command -v perf >/dev/null 2>&1; then
    echo "[ERROR] 'perf' not found. Please install perf to enable perf analysis. Skipping perf analysis."
    USE_PERF=0
  fi
fi

echo "[INFO] Running all test scenarios..."
for test in "${TESTS[@]}"; do
  eval set -- $test
  FLAGS="$1"
  LABEL="$2"
  OUTFILE="$OUTDIR/$LABEL.json"
  PERFOUT="$OUTDIR/$LABEL.perf.data"
  echo "[TEST] $LABEL ($FLAGS) -> $OUTFILE"
  if [[ $USE_PERF -eq 1 ]]; then
    perf record -o "$PERFOUT" -- $BIN $FLAGS --json --mask-sensitive > "$OUTFILE"
  else
    $BIN $FLAGS --json --mask-sensitive > "$OUTFILE"
  fi
  if [[ $USE_GPROF -eq 1 && -f gmon.out ]]; then
    mv gmon.out "$OUTDIR/$LABEL.gmon.out"
    echo "[INFO] Saved gmon.out as $OUTDIR/$LABEL.gmon.out"
  fi
done

echo "[INFO] All tests complete. Generating summary table:"
if [[ $USE_PERF -eq 1 ]]; then
  perf record -o "$OUTDIR/summary.perf.data" -- $BIN $SUMMARY_FLAGS --summary-table $OUTDIR/*.json
else
  $BIN $SUMMARY_FLAGS --summary-table $OUTDIR/*.json
fi

# If perf was enabled and perf.data exists after summary, move and analyze it
if [[ $USE_PERF -eq 1 && -f perf.data ]]; then
  mv perf.data "$OUTDIR/summary.perf.data"
  echo "[INFO] Saved perf.data from summary table as $OUTDIR/summary.perf.data"
fi

# If gprof was enabled and a gmon.out exists after summary, move and analyze it
if [[ $USE_GPROF -eq 1 && -f gmon.out ]]; then
  mv gmon.out "$OUTDIR/summary.gmon.out"
  echo "[INFO] Saved gmon.out from summary table as $OUTDIR/summary.gmon.out"
fi

# If perf was enabled, run perf report for each scenario, combine summaries
if [[ $USE_PERF -eq 1 ]]; then
  for f in $OUTDIR/*.perf.data; do
    txt="${f%.perf.data}.perf.txt"
    label=$(basename "${f%.perf.data}")
    echo "[INFO] Running perf report for $label..."
    perf report -i "$f" --stdio > "$txt"
    echo "[INFO] perf report saved to $txt"
    echo "==== $(basename "$txt") ====" >> "$PERF_SUMMARY_FILE"
    cat "$txt" >> "$PERF_SUMMARY_FILE"
    echo -e "\n" >> "$PERF_SUMMARY_FILE"
  done
  echo "[INFO] Combined perf summary saved to $PERF_SUMMARY_FILE"
fi

# Check if binary is built with -pg if --gprof is requested
if [[ $USE_GPROF -eq 1 ]]; then
  if command -v nm >/dev/null 2>&1; then
    if nm "$BIN" | grep -qE ' mcount|__monstartup'; then
      echo "[INFO] Binary appears to be built with -pg (gprof enabled)."
    else
      echo "[WARNING] Binary does NOT appear to be built with -pg! gprof will not work. Skipping gprof analysis."
      USE_GPROF=0
    fi
  elif command -v objdump >/dev/null 2>&1; then
    if objdump -t "$BIN" | grep -qE ' mcount|__monstartup'; then
      echo "[INFO] Binary appears to be built with -pg (gprof enabled, checked via objdump)."
    else
      echo "[WARNING] Binary does NOT appear to be built with -pg (checked via objdump)! gprof will not work. Skipping gprof analysis."
      USE_GPROF=0
    fi
  else
    echo "[WARNING] Neither 'nm' nor 'objdump' found. Please install binutils to enable gprof build detection. Proceeding with gprof analysis, but cannot verify -pg instrumentation."
    # Do not set USE_GPROF=0; allow gprof to proceed
  fi
fi

# If gprof was enabled, run gprof for each scenario, combine summaries
if [[ $USE_GPROF -eq 1 ]]; then
  rm -f "$GPROF_SUMMARY_FILE"
  for f in $OUTDIR/*.gmon.out; do
    label=$(basename "${f%.gmon.out}")
    echo "[INFO] Running gprof for $label..."
    gprof $BIN "$f" > "$OUTDIR/$label.analysis.txt"
    echo "[INFO] gprof analysis saved to $OUTDIR/$label.analysis.txt"
    echo "==== $label.analysis.txt ====" >> "$GPROF_SUMMARY_FILE"
    cat "$OUTDIR/$label.analysis.txt" >> "$GPROF_SUMMARY_FILE"
    echo -e "\n" >> "$GPROF_SUMMARY_FILE"
  done
  echo "[INFO] Combined gprof summary saved to $GPROF_SUMMARY_FILE"
fi
