#!/usr/bin/env bash
set -euo pipefail

# ============================================================
# Save ORIGINAL arguments BEFORE parsing
# ============================================================
original_args="$@"

usage() {
cat <<EOF
Usage: $0 \
    --version <ver> \
    --quant_cfg_path <path> \
    --codec_cfg_dir <path> \
    --metrics_cfg_path <path> \
    --data_dir <path> \
    --output_dir <path> \
    --template_path <path>

All arguments are required.
EOF
exit 1
}

# ============================================================
# Parse long options
# ============================================================
version=""
data_dir=""
output_dir=""
quant_cfg_path=""
codec_cfg_dir=""
metrics_cfg_path=""
template_path=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --version)          shift; version="$1"; shift ;;
        --quant_cfg_path)   shift; quant_cfg_path="$1"; shift ;;
        --metrics_cfg_path) shift; metrics_cfg_path="$1"; shift ;;
        --codec_cfg_dir)    shift; codec_cfg_dir="$1"; shift ;;
        --data_dir)         shift; data_dir="$1"; shift ;;
        --output_dir)       shift; output_dir="$1"; shift ;;
        --template_path)    shift; template_path="$1"; shift ;;
        --help|-h)          usage ;;
        *) echo "Unknown option: $1" >&2; usage ;;
    esac
done

if [[ -z $version || -z $data_dir || -z $output_dir ||
      -z $quant_cfg_path || -z $codec_cfg_dir ||
      -z $metrics_cfg_path || -z $template_path ]]; then
    echo "ERROR: missing required args" >&2
    usage
fi

# ============================================================
# Directories
# ============================================================
test_dir="$output_dir/experiments/$version"
scripts_dir="$test_dir/scripts"
bitstreams_dir="$test_dir/bitstreams"
cfg_dir="$test_dir/cfgs"

rm -rf "$scripts_dir" && mkdir -p "$scripts_dir"
rm -rf "$bitstreams_dir" && mkdir -p "$bitstreams_dir"
rm -rf "$cfg_dir" && mkdir -p "$cfg_dir"

cp $quant_cfg_path "$cfg_dir/"
cp -r $codec_cfg_dir "$cfg_dir/"
cp $metrics_cfg_path "$cfg_dir/"

log_file="$test_dir/running.log"
: > "$log_file"        # overwrite logfile

# ============================================================
# Detect Git repo
# ============================================================
script_path="$(realpath "${BASH_SOURCE[0]}")"
script_dir="$(dirname "$script_path")"
repo_root=$(git -C "$script_dir" rev-parse --show-toplevel 2>/dev/null || echo "")

# ============================================================
# Write static log header (NEVER overwritten)
# ============================================================
{
    echo "==================== GIT INFO ===================="
    if [[ -n $repo_root ]]; then
        echo "Repo Root : $repo_root"
        echo "Branch    : $(git -C "$repo_root" rev-parse --abbrev-ref HEAD)"
        echo "Commit    : $(git -C "$repo_root" rev-parse HEAD)"
    else
        echo "Not in a Git repository."
    fi
    echo

    echo "=============== SCRIPT INVOCATION ==============="
    echo "$0 $original_args"
    echo

    echo "============== QUANT CONFIG CONTENT ============="
    echo "File: $quant_cfg_path"
    if [[ -f $quant_cfg_path ]]; then
        cat "$quant_cfg_path"
    else
        echo "ERROR: quant_cfg_path not found"
    fi
    echo "=================================================="
    echo
} >> "$log_file"

# ============================================================
# Logging runner
# ============================================================
run() {
    local cmd="$*"
    echo "=== RUNNING: $cmd ===" | tee -a "$log_file"
    $cmd 2>&1 | tee -a "$log_file"
    echo "=== DONE: $cmd @ $(date '+%Y-%m-%d %H:%M:%S') ===" | tee -a "$log_file"
    echo | tee -a "$log_file"
}

# ============================================================
# Main workflow
# ============================================================
run python gen_commands.py \
    --reset_dir \
    --data_dir "$data_dir" \
    --scripts_dir "$scripts_dir" \
    --bitstreams_dir "$bitstreams_dir" \
    --quant_cfg_path "$quant_cfg_path" \
    --codec_cfg_dir "$codec_cfg_dir" \
    --metrics_cfg_path "$metrics_cfg_path"

run python run_parallel.py "$scripts_dir/0_pre.sh" 45
run python run_parallel.py "$scripts_dir/1_enc.sh" 45
run python run_parallel.py "$scripts_dir/2_dec.sh" 45
run python run_parallel.py "$scripts_dir/3_deq.sh" 45
run python run_parallel.py "$scripts_dir/4_cam.sh" 54
run python run_parallel.py "$scripts_dir/5_metric.sh" 80

run python collect_results.py \
    --test_id "$version" \
    --quant_cfg_path "$quant_cfg_path" \
    --clean_files \
    --bitstreams_dir "$bitstreams_dir" \
    --save_dir "$test_dir" \
    --template_path "$template_path"

echo "All commands finished. Log stored at: $log_file"
