#!/usr/bin/env python3
"""
Set per-channel QP values inside r*_multi_attr_params.txt files.

Usage:
  python3 ./scripts/set_multi_attr_qp.py

Behavior:
  - Reads INPUT_FILE
  - For each channel in CHANNEL_QP_MAP:
      updates refl_quant_param for indices listed in GROUPS[channel]
  - Writes to OUTPUT_FILE if provided, otherwise edits in place
"""

from __future__ import annotations

from pathlib import Path
from typing import Dict, List, Tuple

# ------------------ 固定分组定义 ------------------

GROUPS: Dict[str, List[int]] = {
    "sh0_y": [0],
    "sh1_y": [3, 6, 9],
    "sh2_y": [12, 15, 18, 21, 24],
    "sh3_y": [27, 30, 33, 36, 39, 42, 45],
    # Cb/Cr must move together; indices are paired already
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

# ------------------ 用户配置区------------------

INPUT_FILE = "cfg/r1_multi_attr_params.txt"
OUTPUT_FILE = None  # None = 原地修改；也可以写成 "cfg/r1_multi_attr_params_out.txt"

# 所有可用 channel 都列在这里；不想改的就删掉或注释掉该行
CHANNEL_QP_MAP: Dict[str, int] = {
    # ---------- Y ----------
    "sh0_y": 26,
    "sh1_y": 46,
    "sh2_y": 46,
    "sh3_y": 53,

    # ---------- Cb / Cr（成对） ----------
    "sh0_cbcr": 34,
    "sh1_cbcr": 50,
    "sh2_cbcr": 44,
    "sh3_cbcr": 49,

    # ---------- 其他 ----------
    "opacity": 48,
    "scale": 36,
    "rotation": 36,

    # 也允许写别名（等价于 *cbcr）
    # "sh1_cb": 40,
    # "sh1_cr": 40,
}

# ------------------ 实现 ------------------

Block = Tuple[int, str, int]  # (idx, depth, qp)


def parse_file(path: Path) -> tuple[list[str], list[Block]]:
    """Return header lines and per-index (idx, depth, qp) blocks."""
    lines = path.read_text().splitlines()

    if len(lines) < 3:
        raise SystemExit(f"Bad file format: {path} has <3 lines")

    header = lines[:3]
    rest = lines[3:]

    if len(rest) % 3 != 0:
        raise SystemExit(
            f"Bad file format: {path} body lines should be multiple of 3, got {len(rest)}"
        )

    blocks: list[Block] = []
    for i in range(0, len(rest), 3):
        idx_line = rest[i]
        depth_line = rest[i + 1]
        qp_line = rest[i + 2]

        try:
            idx = int(idx_line.split(":")[1].strip())
            depth = depth_line.split(":")[1].strip()
            qp = int(qp_line.split(":")[1].strip())
        except Exception as e:
            raise SystemExit(f"Bad block at body line {i+4} in {path}: {e}")

        blocks.append((idx, depth, qp))

    return header, blocks


def write_file(path: Path, header: list[str], blocks: list[Block]) -> None:
    out_lines = header.copy()
    for idx, depth, qp in blocks:
        out_lines.append(f"multi_attr_params_set_idx: {idx}")
        out_lines.append(f"refl_output_depth: {depth}")
        out_lines.append(f"refl_quant_param: {qp}")
    path.write_text("\n".join(out_lines) + "\n")


def build_qp_by_index(channel_qp_map: Dict[str, int]) -> Dict[int, int]:
    """
    Expand CHANNEL_QP_MAP to a per-index map.
    If multiple channels overlap (正常情况下不会), later keys override earlier ones.
    """
    qp_by_idx: Dict[int, int] = {}

    for ch_raw, qp in channel_qp_map.items():
        ch = CBCR_ALIAS.get(ch_raw, ch_raw)
        if ch not in GROUPS:
            choices = ", ".join(sorted(GROUPS.keys()))
            raise SystemExit(f"Unknown channel '{ch_raw}'. Choices: {choices}")

        for idx in GROUPS[ch]:
            qp_by_idx[idx] = int(qp)

    return qp_by_idx


def apply_qp(blocks: list[Block], qp_by_idx: Dict[int, int]) -> tuple[list[Block], list[tuple[int, int, int]]]:
    """
    Apply qp_by_idx to blocks.
    Returns:
      - updated blocks
      - changes list: (idx, old_qp, new_qp)
    """
    changes: list[tuple[int, int, int]] = []
    out: list[Block] = []

    for idx, depth, old_qp in blocks:
        if idx in qp_by_idx:
            new_qp = qp_by_idx[idx]
            if new_qp != old_qp:
                changes.append((idx, old_qp, new_qp))
            out.append((idx, depth, new_qp))
        else:
            out.append((idx, depth, old_qp))

    return out, changes


def main() -> None:
    in_path = Path(INPUT_FILE)
    out_path = Path(OUTPUT_FILE) if OUTPUT_FILE else in_path

    if not in_path.exists():
        raise SystemExit(f"Input file not found: {in_path}")

    qp_by_idx = build_qp_by_index(CHANNEL_QP_MAP)

    header, blocks = parse_file(in_path)
    new_blocks, changes = apply_qp(blocks, qp_by_idx)

    write_file(out_path, header, new_blocks)

    # 打印变更摘要
    if changes:
        changes_sorted = sorted(changes, key=lambda x: x[0])
        print(f"Updated file: {out_path}")
        print(f"Changed {len(changes_sorted)} indices:")
        for idx, old_qp, new_qp in changes_sorted:
            print(f"  idx {idx}: {old_qp} -> {new_qp}")
    else:
        print(f"No changes needed. File already matches CHANNEL_QP_MAP. ({out_path})")


if __name__ == "__main__":
    main()
