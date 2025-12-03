import os
import re
import pathlib
import shutil
import argparse
import pandas as pd
import numpy as np
from icecream import ic

from collections import defaultdict
from typing import Iterable, List, Optional
from argparse import ArgumentParser
ic.disable()

# 渲染图像的宽、高、视角数、起始帧、帧数、Excel所在行
seq_information = {
"alley"      :[1920,1080,328,0,1,35],
"bartender"  :[1920,1080,21 ,0,1,5],
"bicycle"    :[1920,1080,194,0,1,25],
"cinema"     :[1920,1080,15 ,0,1,10],
"garden"     :[1920,1080,185,0,1,30],
"photo"      :[1920,1080,307,0,1,15],
"rocket"     :[1920,1080,309,0,1,40],
"toy"        :[1920,1080,317,0,1,20],
}


PSNR_columns = {
    "PSNR-RGB": 17,  # PSNR-RGB 列
    "PSNR-YCbCr": 18,  # PSNR-YCbCr 列
    "SSIM-YCbCr": 19,  # SSIM-YCbCr 列
    # "IVSSIM": "I",  # IVSSIM 列
    # "LPIPS": "J"  # LPIPS 列
}

Bitstream_columns = {
"no_output_points"  : 6 ,
"total_bits"        : 7 ,
"geom_bits"         : 8 ,
"ref_bits"          : 9 ,
"enc_time"          : 20 ,
"geom_enc_time"     : 22 ,
"refl_enc_time"     : 24 ,               
"sh0"               : 10 ,     
"sh1"               : 11 ,     
"sh2"               : 12 ,     
"sh3"               : 13 ,     
"opacity"           : 14 ,         
"scaling"           : 15 ,         
"rot"               : 16 ,     
"dec_time"          : 21 ,          
"geom_dec_time"     : 23 ,               
"refl_dec_time"     : 25 ,         

}          


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
    

    results['rot'][0] -= results['scaling'][0]
    results['scaling'][0] -= results['opacity'][0]
    results['opacity'][0] -= results['sh3'][0]
    results['sh3'][0] -= results['sh2'][0]
    results['sh2'][0] -= results['sh1'][0]
    results['sh1'][0] -= results['sh0'][0]

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



if __name__ == "__main__":
    pass
