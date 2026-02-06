#!/usr/bin/env bash
# launch.sh v0.0.1
set -euo pipefail

version="${1:-}"
mode="${2:-}"
time_only=false

if [[ -z "${version}" ]]; then
  echo "Usage: $0 <version> [--time-only]" >&2
  exit 1
fi

if [[ "${mode}" == "--time-only" ]]; then
  time_only=true
fi

script_dir="./scripts/$version"
mkdir -p "$script_dir"
log_file="$script_dir/script.log"

# Start the log file fresh (if you want an empty file each run),
# or comment the next line to keep previous logs.
: > "$log_file"

run() {
  local cmd="$*"

  echo "=== RUNNING: $cmd ===" >> "$log_file"
  # Execute the command, pipe all output (both stdout & stderr) to
  # the terminal AND the log file. `tee` writes to both.
  ${cmd} 2>&1 | tee -a "$log_file"

  # Optional: log a separator + a timestamp
  echo "=== DONE: $cmd @ $(date '+%Y-%m-%d %H:%M:%S') ===" >> "$log_file"
  echo
}

run python3 gen_commands.py \
  --script_dir \
  "$script_dir" \
  --quant_cfg_path \
  "./cfg/cfg_quant/cfg_quant_$version.json" \
  --test_id \
  "$version"

# The last number is the number of processes run in parallel
run python3 run_parallel.py "$script_dir/0_pre.sh" 8
run python3 run_parallel.py "$script_dir/1_enc.sh" 6
run python3 run_parallel.py "$script_dir/2_dec.sh" 20

if [[ "${time_only}" == "true" ]]; then
  echo "Time-only mode: skip dequant, camera rendering, metrics, and collect_results." | tee -a "$log_file"
  echo "All commands finished. Log stored at: $log_file"
  exit 0
fi

run python3 run_parallel.py "$script_dir/3_deq.sh" 20
run python3 run_parallel.py "$script_dir/4_cam.sh" 24
run python3 run_parallel.py "$script_dir/5_metric.sh" 5
run python3 collect_results.py --test_id "$version" --quant_cfg_path "./cfg/cfg_quant/cfg_quant_$version.json"

echo "All commands finished. Log stored at: $log_file"
