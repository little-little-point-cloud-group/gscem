import os
import re
import sys
import pathlib
import shutil
import argparse
import pandas as pd
import numpy as np
import openpyxl

from collections import defaultdict
from typing import Iterable, List, Optional

from cfg.get_cfg import load_yaml_config

from icecream import ic
ic.disable()


def delete_extensions(folder: pathlib.Path, extensions: List[str]) -> int:
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
    folder = pathlib.Path(folder)
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
    else:
        parts = ic(re.split(r'[:\s]', matched[0]))
        for i in reversed(parts):
            if is_finite_number(i):
                v = float(i)
                break
        ret = v
    
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


def parse_metric_log(log_dir, idendifier):
    results = defaultdict(list)
    
    metric_pattern_str = {
        'rgb_psnr': 'Psnr RGB (avg)',
        'yuv_psnr': 'Psnr YUV (avg)',
        'yuv_ssim': 'SSIM (avg)'
    }
    
    # traverse all frame logs ending with _metric.log
    log_files = list(log_dir.rglob("*_metric.log"))
    for file in log_files:
        if idendifier in str(file):
            for k in metric_pattern_str.keys():
                v = find_lines_with_pattern(file, metric_pattern_str[k])
                results[k].append(v)
    for k, v in results.items():
        ic(k, v)
        if len(v) > 1:
            results[k] = [np.mean(v)]
    
    return results


def parse_one_condition(log_dir, scene_frame, cond, rate_point):
    
    cond = cond[0:2]
    results = defaultdict(list)
    for seq in scene_frame:
        frame_name = re.split('\.', scene_frame[seq]['frame'])[0]
        for rate in rate_point:
            results['Sequence'].append(seq)
            idendifier = '_'.join([seq, cond, rate, 'frame'])

            enc_log_path = pathlib.Path(log_dir) / f"{idendifier}_enc.log"
            tmp = parse_enc_log(enc_log_path)
            for k, v in tmp.items():
                results[k] += v

            dec_log_path = pathlib.Path(log_dir) / f"{idendifier}_dec.log"
            tmp = parse_dec_log(dec_log_path)
            for k, v in tmp.items():
                results[k] += v
            
            metric_log_dir = pathlib.Path(log_dir)
            tmp = parse_metric_log(metric_log_dir, idendifier)
            for k, v in tmp.items():
                results[k] += v

    return results
    

def reset_folder(folder_path: str) -> None:
    if os.path.exists(folder_path):
        shutil.rmtree(folder_path)
    os.makedirs(folder_path, exist_ok=True)
 

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Search a log file for lines containing a specific pattern.")
    parser.add_argument("--test_id", required=True)
    parser.add_argument("--quant_cfg_path", required=True)
    parser.add_argument("--clean_files", action='store_true')
    parser.add_argument('--bitstreams_dir', required=True)
    parser.add_argument('--save_dir', required=True)
    parser.add_argument('--template_path', required=True)
    parser.add_argument('--save_excel', action='store_false')

    args, _ = parser.parse_known_args()
    quant_cfg = load_yaml_config(args.quant_cfg_path)
    scenes = quant_cfg['scenes']
    ratepoint_per_cond = quant_cfg['ratepoint_per_cond']

    test_id = args.test_id
    
    for cond in ratepoint_per_cond.keys():
        ret = parse_one_condition(args.bitstreams_dir, scenes, cond, ratepoint_per_cond[cond])

        df = pd.DataFrame(ret)
        df['rot'] -= df['scaling']
        df['scaling'] -= df['opacity']
        df['opacity'] -= df['sh3']
        df['sh3'] -= df['sh2']
        df['sh2'] -= df['sh1']
        df['sh1'] -= df['sh0']
        df['rgb_psnr'] = df['rgb_psnr']
        df['yuv_psnr'] = df['yuv_psnr']
        df['yuv_ssim'] = df['yuv_ssim']

        desired_order = ['no_output_points', 
                        'total_bits', 'geom_bits', 'ref_bits', 
                        'sh0', 'sh1', 'sh2', 'sh3',
                        'opacity', 'scaling', 'rot', 
                        'rgb_psnr', 'yuv_psnr', 'yuv_ssim',
                        'enc_time', 'dec_time', 
                        'geom_enc_time', 'geom_dec_time', 
                        'refl_enc_time', 'refl_dec_time']

        df_order = df[desired_order]
        save_name = '_'.join(map(str, [test_id, cond + '.csv']))
        save_path = pathlib.Path(args.save_dir) / save_name
        df_order.to_csv(save_path, index=False)

        if args.save_excel:
            output_excel_path = os.path.join(args.save_dir, args.test_id + '.xlsm')
            shutil.copyfile(args.template_path, output_excel_path)
            wb =openpyxl.load_workbook(output_excel_path, keep_vba=True, read_only=False)
            wb["Summary"].cell(row=2, column=3).value = args.test_id
            start_row_index = 5
            start_col_index = 25

            for i, header in enumerate(desired_order):
                for j, value in enumerate(df[header]):
                    wb['C1'].cell(row=j + start_row_index, column=start_col_index + i, value=value)

            wb.save(output_excel_path)

    if args.clean_files:
        deleted = delete_extensions(args.bitstreams_dir, ["ply", "bin", "rgb"])
        print(f"{deleted} files deleted")