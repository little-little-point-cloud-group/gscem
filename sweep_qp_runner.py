#!/usr/bin/env python3
"""
Sweep per-channel multi-attribute reflectance QP and call runner.sh for each point.
"""

import argparse
import re
import shutil
import subprocess
from pathlib import Path
from typing import Dict, List, Optional, Tuple


GROUPS: Dict[str, List[int]] = {
    "sh0_y": [0],
    "sh1_y": [3, 6, 9],
    "sh2_y": [12, 15, 18, 21, 24],
    "sh3_y": [27, 30, 33, 36, 39, 42, 45],
    "sh0_cbcr": [1, 2],
    "sh1_cbcr": [4, 5, 7, 8, 10, 11],
    "sh2_cbcr": [13, 14, 16, 17, 19, 20, 22, 23, 25, 26],
    "sh3_cbcr": [28, 29, 31, 32, 34, 35, 37, 38, 40, 41, 43, 44, 46, 47],
    "opacity": [48],
    "scale": [49, 50, 51],
    "rotation": [52, 53, 54, 55],
}

CBCR_ALIAS = {
    "sh0_cb": "sh0_cbcr",
    "sh0_cr": "sh0_cbcr",
    "sh1_cb": "sh1_cbcr",
    "sh1_cr": "sh1_cbcr",
    "sh2_cb": "sh2_cbcr",
    "sh2_cr": "sh2_cbcr",
    "sh3_cb": "sh3_cbcr",
    "sh3_cr": "sh3_cbcr",
}


def _parse_int(value: str, ctx: str) -> int:
    try:
        return int(value.strip())
    except ValueError as exc:
        raise ValueError(f"Invalid integer '{value}' in {ctx}") from exc


def load_attr_params(path: Path) -> Tuple[List[int], Optional[List[Optional[int]]]]:
    text = path.read_text(encoding="utf-8")
    lines = text.splitlines()

    count = 56
    entries: Dict[int, Dict[str, int]] = {}
    current_idx: Optional[int] = None

    for line in lines:
        s = line.strip()
        if not s or ":" not in s:
            continue
        key, value = [token.strip() for token in s.split(":", 1)]
        if key == "num_multi_attr_params_set":
            count = _parse_int(value, str(path))
        elif key == "multi_attr_params_set_idx":
            current_idx = _parse_int(value, str(path))
            entries.setdefault(current_idx, {})
        elif current_idx is not None and key in {"refl_quant_param", "refl_output_depth"}:
            entries[current_idx][key] = _parse_int(value, str(path))

    if not entries:
        raise ValueError(f"No multi_attr_params_set entries found in {path}")

    total = max(count, max(entries.keys()) + 1)
    qps: List[int] = []
    depths: List[Optional[int]] = []
    for idx in range(total):
        if idx not in entries or "refl_quant_param" not in entries[idx]:
            raise ValueError(f"Missing refl_quant_param for index {idx} in {path}")
        qps.append(entries[idx]["refl_quant_param"])
        depths.append(entries[idx].get("refl_output_depth"))

    if any(depth is not None for depth in depths):
        return qps, depths
    return qps, None


def dump_attr_block(qps: List[int], depths: Optional[List[Optional[int]]]) -> str:
    out: List[str] = [
        "parse_multi_attr_params: 1",
        f"num_multi_attr_params_set: {len(qps)}",
        "attribute: refl",
    ]
    for idx, qp in enumerate(qps):
        out.append(f"multi_attr_params_set_idx: {idx}")
        if depths is not None:
            depth_val = depths[idx] if depths[idx] is not None else 32
            out.append(f"refl_output_depth: {depth_val}")
        out.append(f"refl_quant_param: {qp}")
    return "\n".join(out) + "\n"


def inject_attr_block(cfg_file: Path, attr_block: str) -> None:
    text = cfg_file.read_text(encoding="utf-8")
    match = re.search(r"(?m)^parse_multi_attr_params\s*:", text)
    if match:
        new_text = text[: match.start()].rstrip() + "\n" + attr_block
    else:
        new_text = text.rstrip() + "\n" + attr_block
    cfg_file.write_text(new_text, encoding="utf-8")


def patch_codec_cfg_dir(codec_cfg_dir: Path, blocks_by_rate: Dict[str, str]) -> int:
    patched = 0
    for encoder_cfg in codec_cfg_dir.rglob("encoder.cfg"):
        rate = encoder_cfg.parent.name
        block = blocks_by_rate.get(rate)
        if block is None:
            continue
        inject_attr_block(encoder_cfg, block)
        patched += 1
    return patched


def normalize_channels(channels: List[str]) -> List[str]:
    normalized: List[str] = []
    for channel in channels:
        mapped = CBCR_ALIAS.get(channel, channel)
        if mapped not in GROUPS:
            names = ", ".join(sorted(GROUPS.keys()))
            raise ValueError(f"Unknown channel '{channel}'. Available: {names}")
        normalized.append(mapped)
    return normalized


def call_runner(
    runner_path: Path,
    version: str,
    quant_cfg_path: Path,
    codec_cfg_dir: Path,
    metrics_cfg_path: Path,
    data_dir: Path,
    output_dir: Path,
    template_path: Path,
) -> None:
    cmd = [
        "bash",
        str(runner_path),
        "--version",
        version,
        "--quant_cfg_path",
        str(quant_cfg_path),
        "--codec_cfg_dir",
        str(codec_cfg_dir),
        "--metrics_cfg_path",
        str(metrics_cfg_path),
        "--data_dir",
        str(data_dir),
        "--output_dir",
        str(output_dir),
        "--template_path",
        str(template_path),
    ]
    subprocess.run(cmd, check=True)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Nested sweep launcher: modify multi-attr QP block and call runner.sh repeatedly."
    )
    parser.add_argument("--version", required=True, help="Base version prefix used by runner.sh.")
    parser.add_argument("--quant_cfg_path", required=True)
    parser.add_argument("--codec_cfg_dir", required=True)
    parser.add_argument("--metrics_cfg_path", required=True)
    parser.add_argument("--data_dir", required=True)
    parser.add_argument("--output_dir", required=True)
    parser.add_argument("--template_path", required=True)

    parser.add_argument("--runner_path", default="./runner.sh")
    parser.add_argument("--base_attr_dir", default="./cfg", help="Directory containing r*_multi_attr_params.txt.")
    parser.add_argument("--work_dir", default="./qp_sweep_work", help="Temporary workspace for patched cfg trees.")
    parser.add_argument("--rates", nargs="+", default=["r1", "r2", "r3", "r4", "r5"])
    parser.add_argument("--channels", nargs="+", default=["sh2_y"])
    parser.add_argument("--deltas", nargs="+", type=int, default=[0])
    parser.add_argument("--keep_cfg", action="store_true", help="Keep generated cfg trees under work_dir.")
    parser.add_argument("--dry_run", action="store_true", help="Only print planned runs without executing runner.sh.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    runner_path = Path(args.runner_path).resolve()
    quant_cfg_path = Path(args.quant_cfg_path).resolve()
    codec_cfg_dir = Path(args.codec_cfg_dir).resolve()
    metrics_cfg_path = Path(args.metrics_cfg_path).resolve()
    data_dir = Path(args.data_dir).resolve()
    output_dir = Path(args.output_dir).resolve()
    template_path = Path(args.template_path).resolve()
    base_attr_dir = Path(args.base_attr_dir).resolve()
    work_dir = Path(args.work_dir).resolve()

    if not runner_path.exists():
        raise FileNotFoundError(f"runner.sh not found: {runner_path}")
    if not codec_cfg_dir.is_dir():
        raise FileNotFoundError(f"codec_cfg_dir not found: {codec_cfg_dir}")
    if not base_attr_dir.is_dir():
        raise FileNotFoundError(f"base_attr_dir not found: {base_attr_dir}")

    channels = normalize_channels(args.channels)

    base_rate_params: Dict[str, Tuple[List[int], Optional[List[Optional[int]]]]] = {}
    for rate in args.rates:
        attr_file = base_attr_dir / f"{rate}_multi_attr_params.txt"
        if not attr_file.exists():
            raise FileNotFoundError(f"Missing attr params file for rate '{rate}': {attr_file}")
        base_rate_params[rate] = load_attr_params(attr_file)

    work_dir.mkdir(parents=True, exist_ok=True)
    try:
        for channel in channels:
            idxs = GROUPS[channel]
            for delta in args.deltas:
                run_version = f"{args.version}-{channel}-{delta:+d}"
                cfg_variant_dir = work_dir / run_version
                if cfg_variant_dir.exists():
                    shutil.rmtree(cfg_variant_dir)
                shutil.copytree(codec_cfg_dir, cfg_variant_dir)

                blocks_by_rate: Dict[str, str] = {}
                for rate, (base_qps, base_depths) in base_rate_params.items():
                    qps = base_qps.copy()
                    for idx in idxs:
                        if idx >= len(qps):
                            raise ValueError(
                                f"Rate '{rate}' has only {len(qps)} attrs, cannot modify index {idx}."
                            )
                        qps[idx] = max(0, qps[idx] + delta)
                    depths = base_depths.copy() if base_depths is not None else None
                    blocks_by_rate[rate] = dump_attr_block(qps, depths)

                patched = patch_codec_cfg_dir(cfg_variant_dir, blocks_by_rate)
                if patched == 0:
                    raise RuntimeError(
                        f"No encoder.cfg patched in {cfg_variant_dir}. Check rate folder names and codec_cfg_dir."
                    )

                print(f"[sweep] {run_version} | patched encoder.cfg files: {patched}")
                if args.dry_run:
                    continue

                call_runner(
                    runner_path=runner_path,
                    version=run_version,
                    quant_cfg_path=quant_cfg_path,
                    codec_cfg_dir=cfg_variant_dir,
                    metrics_cfg_path=metrics_cfg_path,
                    data_dir=data_dir,
                    output_dir=output_dir,
                    template_path=template_path,
                )
    finally:
        if not args.keep_cfg:
            shutil.rmtree(work_dir, ignore_errors=True)


if __name__ == "__main__":
    main()
