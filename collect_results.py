import os
import re
import sys
import pathlib
import shutil
import argparse
import pandas as pd
import numpy as np
import openpyxl
from icecream import ic

from collections import defaultdict
from typing import Iterable, List, Optional
from argparse import ArgumentParser
from cfg.cfg_quant import load_config
ic.disable()

seq_start_row = {"bartender": 5,
        "cinema": 10,
        "photo": 15,
        "toy": 20,
        "bicycle": 25,
        "garden": 30,
        "alley": 35,
        "rocket": 40,}


def delete_extensions(folder: pathlib.Path, extensions: list[str]) -> int:
    """
    Delete files in *folder* that match any of the suffixes in *extensions*.

    Parameters
    ----------
    folder : pathlib.Path
        Target directory.
    extensions : list[str]
        List of suffixes (without the leading dot), e.g. ['ply', 'bin'].

    Returns
    -------
    int
        Number of files successfully removed.
    """
    if not folder.is_dir():
        print(f"{folder} is not a directory or does not exist.", file=sys.stderr)
        return 0

    removed = 0
    for ext in extensions:
        pattern = f"*.{ext}"
        for file_path in folder.glob(pattern):
            try:
                file_path.unlink()
                print(f"Removed: {file_path}")
                removed += 1
            except Exception as e:
                print(f"Could not delete {file_path}: {e}", file=sys.stderr)

    return removed


def is_finite_number(s: str) -> bool:
    try:
        n = float(s)
        return not (float('nan') == n or float('inf') == n or -float('inf') == n)
    except ValueError:
        return False


def find_lines_with_pattern(
    file_path: pathlib.Path,
    pattern: str,
    case_insensitive: bool = False,
    ) -> List:
    """
    Generator that yields every line from *file_path* that contains *pattern*.

    Parameters
    ----------
    file_path : pathlib.Path
        Path to the log file (e.g. pathlib.Path('log.txt')).
    pattern : str
        Pattern to search for. The search is a simple substring test, not a
        regular expression.  If you need regex support, use ``re.search`` inside
        the loop instead.
    case_insensitive : bool, optional
        If True, the search is case-insensitive (uses .lower()).
    """
    ic(pattern)
    with file_path.open('r', encoding='utf-8') as fh:
        matched = []
        for line_no, line in enumerate(fh, start=1):
            if line.startswith(pattern):
                matched.append(line.rstrip('\n'))
    ic(matched)
    # Gracefully handle missing pattern: return NaN so downstream arithmetic still works
    if not matched:
        return float('nan')
    if len(matched) > 1:
        to_avg = []
        for line in matched:    
            parts = ic(re.split(r'[:\s]', line))
            for i in reversed(parts):
                if is_finite_number(i):
                    v = float(i)
                    break
            ic(to_avg.append(v))
        ret =  ic(np.mean(to_avg))
    elif len(matched) == 1:
        parts = ic(re.split(r'[:\s]', matched[0]))
        for i in reversed(parts):
            if is_finite_number(i):
                v = float(i)
                break
        ret = v
    else:
        ret = float('nan')
    
    return ret


def parse_enc_log(log_path):
    ic(log_path)
    results = defaultdict(list)
    
    enc_pattern_str = {
        'no_output_points': 'All frames number of output points',
        'total_bits': 'All frames total bitstream size',
        'geom_bits': 'All frames geometry bits',
        'ref_bits': 'All frames refl bits',
        'enc_time': 'All frames total processing time (user)',
        'geom_enc_time': 'All frames geometry processing time (user)',
        'refl_enc_time': 'All frames attributes processing time (user)',
        'sh0': 'MultiData 2 Attributes_refl bits',
        'sh1': 'MultiData 11 Attributes_refl bits',
        'sh2': 'MultiData 26 Attributes_refl bits',
        'sh3': 'MultiData 47 Attributes_refl bits',
        'opacity': 'MultiData 48 Attributes_refl bits',
        'scaling': 'MultiData 51 Attributes_refl bits',
        'rot': 'MultiData 55 Attributes_refl bits',
    }
    
    for k in enc_pattern_str.keys():
        v = find_lines_with_pattern(log_path, enc_pattern_str[k])
        results[k].append(v)   
    
    return results


def parse_dec_log(log_path):
    results = defaultdict(list)
    
    dec_pattern_str = {
        'dec_time': 'All frames total processing time (user)',
        'geom_dec_time': 'All frames geometry processing time (user)',
        'refl_dec_time': 'All frames attributes processing time (user)'
    }
    
    for k in dec_pattern_str.keys():
        v = find_lines_with_pattern(log_path, dec_pattern_str[k])
        results[k].append(v)
    
    return results


def parse_metric_log(log_path):
    results = defaultdict(list)
    
    dec_pattern_str = {
        'rgb_psnr': 'Psnr RGB (avg)',
        'yuv_psnr': 'Psnr YUV (avg)',
        'yuv_ssim': 'SSIM (avg)'
    }
    
    for k in dec_pattern_str.keys():
        v = find_lines_with_pattern(log_path, dec_pattern_str[k])
        results[k].append(v)
    
    return results


def parse_one_condition(log_dir, scene_frame, method, cond, rate_point, time_only: bool = False):
    
    cond = cond[0:2]
    results = defaultdict(list)
    for seq in scene_frame:
        frame_name = re.split('\.', scene_frame[seq]['frame'])[0]
        for rate in rate_point:
            results['Sequence'].append(seq)
            idendifier = '_'.join([seq, frame_name, cond, rate, method])

            enc_log_path = pathlib.Path(log_dir) / f"{idendifier}_enc.log"
            tmp = parse_enc_log(enc_log_path)
            for k, v in tmp.items():
                results[k] += v

            dec_log_path = pathlib.Path(log_dir) / f"{idendifier}_dec.log"
            tmp = parse_dec_log(dec_log_path)
            for k, v in tmp.items():
                results[k] += v
            
            if not time_only:
                metric_log_path = pathlib.Path(log_dir) / f"{idendifier}_metric.log"
                tmp = parse_metric_log(metric_log_path)
                for k, v in tmp.items():
                    results[k] += v

    return results
    
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Search a log file for lines containing a specific pattern.")
    parser.add_argument("--test_id", default="qpSweep-sh0_y--8")
    parser.add_argument("--quant_cfg_path", default='cfg/cfg_quant/cfg_quant_v0.1.json')
    # change the following 2 path
    parser.add_argument('--log-dir', default='/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly')
    parser.add_argument('--output-dir', default='./results')
    parser.add_argument('--template_path', default='template/template.xlsm', help="Excel template (.xlsm/.xlsx) with sheets named by cond (e.g., C1, C2)")
    parser.add_argument('--skip_excel', action='store_true', help="Only write CSV; skip Excel output")
    parser.add_argument('--keep_artifacts', action='store_true',
                        help="Do not delete intermediate ply/bin/rgb files in log-dir (default: delete to save space)")
    parser.add_argument('--time-only', action='store_true',
                        help="Only collect time-related fields; skip PSNR/SSIM parsing and output")
    
    args, _ = parser.parse_known_args()
    quant_cfg = load_config(args.quant_cfg_path)
    scenes = quant_cfg['scenes']
    PCRM_methods = quant_cfg['PCRM_methods']
    ratepoint_per_cond = quant_cfg['ratepoint_per_cond']

    args.log_dir = pathlib.Path(args.log_dir) / f"{args.test_id}"
    output_dir = pathlib.Path(args.output_dir)

    # Prepare Excel workbook if needed
    wb = None
    if not args.skip_excel:
        output_dir.mkdir(parents=True, exist_ok=True)
        excel_path = output_dir / f"{args.test_id}.xlsm"
        shutil.copyfile(args.template_path, excel_path)
        wb = openpyxl.load_workbook(excel_path, keep_vba=True, read_only=False)
        if "Summary" in wb.sheetnames:
            wb["Summary"].cell(row=2, column=3).value = args.test_id

    full_order = ['Sequence', 'no_output_points', 
                  'total_bits', 'geom_bits', 'ref_bits', 
                  'sh0', 'sh1', 'sh2', 'sh3',
                  'opacity', 'scaling', 'rot', 
                  'rgb_psnr', 'yuv_psnr', 'yuv_ssim',
                  'enc_time', 'dec_time', 
                  'geom_enc_time', 'geom_dec_time', 
                  'refl_enc_time', 'refl_dec_time']
    time_only_order = ['Sequence', 'enc_time', 'dec_time', 
                       'geom_enc_time', 'geom_dec_time', 
                       'refl_enc_time', 'refl_dec_time']
    time_only_fields = set(time_only_order) - {"Sequence"}

    for method in PCRM_methods.keys():
        for cond in ratepoint_per_cond.keys():
            ret = parse_one_condition(args.log_dir, scenes, method, cond, ratepoint_per_cond[cond], time_only=args.time_only)

            df = pd.DataFrame(ret)
            if not args.time_only:
                df['rot'] -= df['scaling']
                df['scaling'] -= df['opacity']
                df['opacity'] -= df['sh3']
                df['sh3'] -= df['sh2']
                df['sh2'] -= df['sh1']
                df['sh1'] -= df['sh0']
                df['rgb_psnr'] = df['rgb_psnr']
                df['yuv_psnr'] = df['yuv_psnr']
                df['yuv_ssim'] = df['yuv_ssim']
                df_order = df[full_order]
            else:
                df_order = df[time_only_order]
            save_name = '_'.join(map(str, [args.test_id, method, cond + '.csv']))
            save_path = output_dir / save_name
  
            # Write into Excel (sheet named by cond prefix, e.g., C1/C2)
            if wb:
                sheet_name = "C1"
                start_row = 5
                start_col = 25  # column Y=25? Actually 25 -> Y, 26 -> Z; here matches模板要求
                sheet = wb[sheet_name]
                
                a=df_order.iloc[0]["Sequence"]
                row=-1
                for j in range(len(df_order)):
                    # 获取当前行的Sequence值，并查找对应的起始行
                    sequence_value = df_order.iloc[j]["Sequence"]
                    if a==sequence_value:
                        row=row+1
                    else:
                        row=0
                        a=sequence_value
                    current_start_row = seq_start_row.get(sequence_value)
                    
                    if current_start_row is None:
                        print(f"警告：未找到Sequence '{sequence_value}' 对应的起始行，跳过该行")
                        continue
                    
                    # 遍历每一列（按full_order的顺序，保持模板列对齐）
                    for i, header in enumerate(full_order):
                        # 获取当前单元格的值
                        if header=="Sequence":
                            continue
                        if args.time_only and header not in time_only_fields:
                            continue
                        value = df.iloc[j][header] if header in df.columns else float('nan')
                        
                        # 写入Excel（注意：行索引从1开始，列索引从start_col开始）
                        sheet.cell(row=current_start_row+row, column=start_col + i-1, value=value)
   

    if wb:
        wb.save(output_dir / f"{args.test_id}.xlsm")

    if args.keep_artifacts:
        print("keep_artifacts enabled: skip deleting ply/bin/rgb")
    else:
        deleted = delete_extensions(args.log_dir, ["ply", "bin", "rgb"])
        print(f"{deleted} files deleted")
