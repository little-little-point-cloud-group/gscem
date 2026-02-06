#!/usr/bin/env python3
"""
Sweep per-channel reflectance QP (multi_attr_params) and run pipeline.

Current baseline QP value:
Rate point	SH0(DC)	SH0(DC)	SH1Y	SH2Y	SH3Y	SH1Cb	SH2Cb	SH3Cb	SH1Cr	SH2Cr	SH3Cr	Opacity	  Scale	  Rotation
	         Y	    Cb Cr												
R5	         4	    8	    18	    14	    22	    22	    12	    18	    22	    12	    18	      16	    8   	8
R4	         6	    12	    22	    22	    29	    26	    20	    25	    26	    20	    25	      24	    12	    12
R3	         10	    18	    30	    30	    37	    34	    28	    33	    34	    28	    33	      32	    20	    20
R2	         18	    26	    38	    38	    45	    42	    36	    41	    42	    36	    41	      40	    28	    28
R1	         26	    34	    46	    46	    53	    50	    44	    49	    50	    44	    49	      48	    36	    36




1. To modify QP values, first need to use python3 gen_cfg_multil.py to generate cfg/cfg_predict_multiple, then run the following command.
2. In gscem_all/cfg/cfg_quant/cfg_quant_v0.1.json, the RGB to YUV conversion has been enabled (already turned on).
3. python3 scripts/sweep_qp.py \
  --base_cfg_dir ./cfg/cfg_predict_multiple \
  --base_quant_cfg ./cfg/cfg_quant/cfg_quant_v0.1.json \
  --runner_dir scripts \
  --data_dir /media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly \
  --results_root ../gscem_all/results \
  --excel_template ./template/template.xlsm \
  --version_prefix qpSweep \
  --rates r1 r2 r3 r4 r5 \
  
"""

import argparse, shutil, tempfile, subprocess, json, re, sys, time
from pathlib import Path

GROUPS = {
    "sh0_y": [0],
    "sh1_y": [3, 6, 9],
    "sh2_y": [12, 15, 18, 21, 24],
    "sh3_y": [27, 30, 33, 36, 39, 42, 45],
    # Cb/Cr 
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

BASE_HEADER = "parse_multi_attr_params: 1\nnum_multi_attr_params_set: 56\nattribute: refl\n"

def load_qps(path: Path):
    lines = path.read_text().splitlines()
    qps = []
    for i in range(56):
        qp_line = lines[3 + i*3 + 2]    
        qps.append(int(qp_line.split(":")[1]))
    return qps

def write_qps(path: Path, qps):
    out = [BASE_HEADER]
    for i, qp in enumerate(qps):
        out.append(f"multi_attr_params_set_idx: {i}\nrefl_output_depth: 32\nrefl_quant_param: {qp}\n")
    path.write_text("".join(out))

def _inject_attr_block(cfg_file: Path, attr_dir: Path):
    """
    Replace the multi-attribute block inside a single cfg file using the
    per-rate parameters stored in attr_dir (one file per rate).
    """
    m = re.search(r"/(r[0-9]+)/", str(cfg_file).replace("\\", "/"))
    if not m:
        return
    rate = m.group(1)
    attr_path = attr_dir / f"{rate}.txt"
    if not attr_path.exists():
        return
    block = attr_path.read_text()
    text = cfg_file.read_text()
    # Replace the old block (from parse_multi_attr_params to EOF) with the new one
    if "parse_multi_attr_params" in text:
        # Replace everything from the first parse_multi_attr_params line to EOF.
        # The previous pattern used `[\\s\\S]`, which (inside a character class)
        # only matches backslash or literal "s"/"S", so it never consumed the
        # block and left configs unmodified. Use [\s\S] to match any char.
        text = re.sub(r"parse_multi_attr_params:[\s\S]*$", block, text, flags=re.MULTILINE)
    else:
        text = text + ("\n" if not text.endswith("\n") else "") + block
    cfg_file.write_text(text)


def patch_cfg_tree(src_cfg: Path, attr_dir: Path, out_cfg: Path):
    """
    Copy a base cfg tree into a temp dir and inject per-rate attr params.
    gen_commands.py expects configs under cfg_dir/cfg_predict/... so we
    place the copied tree inside a cfg_predict subfolder to match that.
    """
    if out_cfg.exists():
        shutil.rmtree(out_cfg)

    dest_root = out_cfg / "cfg_predict"
    shutil.copytree(src_cfg, dest_root)


    for cfg_file in dest_root.rglob("encoder.cfg"):
        _inject_attr_block(cfg_file, attr_dir)


def run_pipeline(args, cfg_dir, version):
    script_dir = Path(args.runner_dir) / version
    script_dir.mkdir(parents=True, exist_ok=True)
    subprocess.run([
        sys.executable,"gen_commands.py",
        "--cfg_dir", str(cfg_dir),
        "--quant_cfg_path", args.base_quant_cfg,
        "--script_dir", str(script_dir),
        "--data_dir", args.data_dir,
        "--test_id", version
    ], check=True)
    if args.time_only:
        start = time.perf_counter()
        for idx, procs in [(0,8),(1,6),(2,20)]:
            script = f"{script_dir}/{idx}_pre.sh" if idx==0 else f"{script_dir}/{idx}_enc.sh" if idx==1 else f"{script_dir}/{idx}_dec.sh"
            subprocess.run([sys.executable,"run_parallel.py", script, str(procs)], check=True)
        total = time.perf_counter() - start
        print(f"[time-only] {version} total wall time (pre+enc+dec): {total:.3f} s")
    else:
        for idx, procs in [(0,8),(1,6),(2,20),(3,20),(4,24),(5,5)]:
            subprocess.run([sys.executable,"run_parallel.py", f"{script_dir}/{idx}_pre.sh" if idx==0 else f"{script_dir}/{idx}_enc.sh" if idx==1 else f"{script_dir}/{idx}_dec.sh" if idx==2 else f"{script_dir}/{idx}_deq.sh" if idx==3 else f"{script_dir}/{idx}_cam.sh" if idx==4 else f"{script_dir}/{idx}_metric.sh", str(procs)], check=True)
    # Collect results for this sweep into its own folder to avoid overwriting others
    results_dir = Path(args.results_root) / version
    results_dir.mkdir(parents=True, exist_ok=True)
    cmd = [
        sys.executable, "collect_results.py",
        "--test_id", version,
        "--quant_cfg_path", args.base_quant_cfg,
        "--log-dir", args.data_dir,
        "--output-dir", str(results_dir),
        "--template_path", args.excel_template
    ]
    if args.time_only:
        cmd.append("--time-only")
    if args.keep_artifacts:
        cmd.append("--keep_artifacts")
    subprocess.run(cmd, check=True)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--base_cfg_dir", required=True)
    ap.add_argument("--base_quant_cfg", required=True)
    ap.add_argument("--runner_dir", default="scripts")
    ap.add_argument("--data_dir", required=True)
    ap.add_argument("--results_root", help="where to store collected CSVs; default <data_dir>/results")
    ap.add_argument("--excel_template", required=True, help="path to Excel template (.xlsm/.xlsx) for collect_results")
    ap.add_argument("--version_prefix", default="qpSweep")
    ap.add_argument("--rates", nargs="+", default=["r1","r2","r3","r4","r5"])
    ap.add_argument("--channels", nargs="+", default=["sh2_y"],
                    help="sweep channels; default: sh2_y")
    ap.add_argument("--deltas", nargs="+", type=int, default=[0],
                    help="qp delta(s) to apply; default: 0 (no change)")
    ap.add_argument("--keep_cfg", action="store_true")
    ap.add_argument("--keep_artifacts", action="store_true", help="forward to collect_results to retain ply/bin/rgb")
    ap.add_argument("--time-only", action="store_true", help="only run 0_pre/1_enc/2_dec and report total time")
    args = ap.parse_args()
    if args.results_root is None:
        args.results_root = str(Path(args.data_dir) / "results")

    base_cfg = Path(args.base_cfg_dir)
    tmp_root = Path("qp_sweep_work").resolve()
    tmp_root.mkdir(parents=True, exist_ok=True)


    for ch in args.channels:

        ch = CBCR_ALIAS.get(ch, ch)
        idxs = GROUPS.get(ch)
        if idxs is None:
            raise ValueError(f"Unknown channel '{ch}'.  {sorted(GROUPS.keys())}")
        for d in args.deltas:

            attr_dir = tmp_root / f"attr_{ch}_d{d}"
            attr_dir.mkdir(parents=True, exist_ok=True)
            for rate in args.rates:
                base_attr = Path("cfg") / f"{rate}_multi_attr_params.txt"
                qps = load_qps(base_attr)
                for i in idxs:
                    qps[i] = max(0, qps[i] + d)
                write_qps(attr_dir / f"{rate}.txt", qps)

            out_cfg = tmp_root / f"cfg_{ch}_d{d}"
            patch_cfg_tree(base_cfg, attr_dir, out_cfg)
            version = f"{args.version_prefix}-{ch}-{d:+}"
            print(f"==> {version} using cfg {out_cfg}")
            run_pipeline(args, out_cfg, version)

    if not args.keep_cfg:
        shutil.rmtree(tmp_root, ignore_errors=True)

if __name__ == "__main__":
    main()
