import os
import argparse
import shutil
import numpy as np
from plyfile import PlyData
from icecream import ic
ic.disable()

def reset_folder(folder_path: str) -> None:
    if os.path.exists(folder_path):
        shutil.rmtree(folder_path)
    os.makedirs(folder_path, exist_ok=True)


INRIA_FORMATS = [
    'x', 'y', 'z',
    'f_dc_0', 'f_dc_1', 'f_dc_2',
    *[f'f_rest_{i}' for i in range(45)],
    'opacity',
    'scale_0', 'scale_1', 'scale_2',
    'rot_0', 'rot_1', 'rot_2', 'rot_3'
]

PCRM_FORMATS = [
    'x', 'y', 'z',
    'reflectance',
    *[f'reflectance{i}' for i in range(1, len(INRIA_FORMATS) - 3)]
]

def read_ply(input_path, header='INRIA'):
    assert input_path.endswith('.ply'), 'input path must be a .ply file.'
    ply_data= PlyData.read(input_path).elements[0]
    if header=='INRIA':
        ret = np.stack([ply_data.data[name] for name in INRIA_FORMATS], axis=1)
    else:
        ret = np.stack([ply_data.data[name] for name in PCRM_FORMATS], axis=1)
    return ret

def write_ply(gs_data, output_path, header='INRIA'):
    assert output_path.endswith('.ply'), 'destination path must be a .ply file.'
    gs_data = gs_data.astype(float)
    with open(output_path, 'w') as f:
        f.writelines(['ply\n', 'format ascii 1.0\n', f'element vertex {gs_data.shape[0]}\n'])
        if header == 'INRIA':
            if gs_data.shape[1] == len(INRIA_FORMATS):
                f.writelines(f'property float {i}\n' for i in INRIA_FORMATS)
            else:
                f.writelines(f'property float {i}\n' for i in INRIA_FORMATS[0:3])
                f.writelines(f'property float {i}\n' for i in ['nx', 'ny', 'nz'])
                f.writelines(f'property float {i}\n' for i in INRIA_FORMATS[3:])
        else:
            f.writelines(f'property float32 {i}\n' for i in PCRM_FORMATS)
        f.writelines(['end_header\n'])
        np.savetxt(f, gs_data, fmt='%.8f')

def dc_2d_to_3d(dc_2d):
 
    vertex_num = dc_2d.shape[0]
    dc_3d = np.zeros((vertex_num, 1, 3), dtype=dc_2d.dtype)
    dc_3d[:, 0, :] = dc_2d  
    return dc_3d


def dc_3d_to_2d(dc_3d):
   
    dc_2d = dc_3d[:, 0, :].reshape(-1, 3)  
    return dc_2d

def ac_2d_to_3d(ac_2d):
   
    
    vertex_num = ac_2d.shape[0]
    ac_3d = np.zeros((vertex_num, 15, 3), dtype=ac_2d.dtype)  
   
    for j in range(1, 16):  
        for i in range(3):   
            ac_3d[:, j-1, i] = ac_2d[:, (j-1) + i*15] 
    return ac_3d


def ac_3d_to_2d(ac_3d):
    
    vertex_num = ac_3d.shape[0]
    ac_2d = np.zeros((vertex_num, 45), dtype=ac_3d.dtype)
    
 
    for j in range(1, 16):
        for i in range(3):  
            ac_2d[:, (j-1) + i*15] = ac_3d[:, j-1, i]
    return ac_2d

def rgb2yuv_bt709(sh_rgb):
    sh_yuv = np.zeros(sh_rgb.shape)
    for i in range(sh_rgb.shape[1]):
        R = sh_rgb[:,i,0]
        G = sh_rgb[:,i,1]
        B = sh_rgb[:,i,2]
        sh_yuv[:,i,0] =  0.2126 * R + 0.7152 * G + 0.0722 * B   # Y
        sh_yuv[:,i,1] = -0.1146 * R - 0.3854 * G + 0.5    * B   # U
        sh_yuv[:,i,2] =  0.5    * R - 0.4542 * G - 0.0458 * B   # V
    return sh_yuv

def yuv2rgb_bt709(sh_yuv):
    sh_rgb = np.zeros(sh_yuv.shape)
    for i in range(sh_yuv.shape[1]):
        Y = sh_yuv[:,i,0]
        U = sh_yuv[:,i,1]
        V = sh_yuv[:,i,2]
        sh_rgb[:,i,0] = Y + 1.5748 * V                   # R
        sh_rgb[:,i,1] = Y - 0.1873 * U - 0.4681 * V      # G
        sh_rgb[:,i,2] = Y + 1.8556 * U                   # B
    return sh_rgb

def quantize_scalar(x, bits, eps=1e-8, percentiles=(0.25, 99.75), force_percentiles=False):
    x = np.asarray(x, dtype=np.float64)
    scale = (1 << bits) - 1

    finite_mask = np.isfinite(x)
    x_fin = x[finite_mask]

    low_p, hi_p = percentiles
    low_p = float(low_p)
    hi_p  = float(hi_p)

    if force_percentiles:        
        min_val = float(np.percentile(x_fin, low_p))
        max_val = float(np.percentile(x_fin, hi_p))
    else:
        has_neg_inf = np.isneginf(x).any()
        has_pos_inf = np.isposinf(x).any()

        if has_neg_inf:
            min_val = float(np.percentile(x_fin, low_p))
        else:
            min_val = float(x_fin.min())

        if has_pos_inf:
            max_val = float(np.percentile(x_fin, hi_p))
        else:
            max_val = float(x_fin.max())

    # Avoid invalid range
    if max_val - min_val < eps:
        max_val = min_val + eps

    # Replace infinities with endpoints
    xw = x.copy()
    xw[np.isneginf(xw)] = min_val
    xw[np.isposinf(xw)] = max_val

    # Clip to [min_val, max_val]
    xw = np.clip(xw, min_val, max_val)

    # Normalize to [0, 1]
    x_norm = (xw - min_val) / (max_val - min_val)

    # Scale, round and clip to [0, scale]
    q = np.rint(x_norm * scale)
    q = np.clip(q, 0, scale).astype(np.int64)

    return q, min_val, max_val

def dequantize_scalar(q, bitdepth, min_val, max_val):
    scale = 2**bitdepth - 1
    return q.astype(np.float64) / scale * (max_val - min_val) + min_val

def normalize_rot(rot):
    q_rot = np.zeros(rot.shape)
    for i in range(rot.shape[0]):
        norm = np.inner(rot[i,:], rot[i,:])
        if norm > 1e-6:
            if rot[i,0] < 0:
                q_rot[i,:] = -rot[i,:]/np.sqrt(norm)
            else:
                q_rot[i,:] = +rot[i,:]/np.sqrt(norm)
        else:
            q_rot[i,:] = [1,0,0,0]
    return q_rot

def main():
    parser = argparse.ArgumentParser(description='3DGS PLY file percentile quantization')
    parser.add_argument('--input_path', help='input PLY file path')
    parser.add_argument('--output_path', help='output PLY file path')
    parser.add_argument('--meta_path', help='metadata npz path')
    parser.add_argument('--quantize', action='store_true', help='quantize data to integers')
    parser.add_argument('--dequantize', action='store_true', help='dequantize data to floats')
    parser.add_argument('--bits_geom', type=int, default=18, help='no. of bits for geom')
    parser.add_argument('--bits_sh_dc', type=int, default=12, help='no. of bits for SH DC')
    parser.add_argument('--bits_sh_ac', type=int, default=12, help='no. of bits for SH AC')
    parser.add_argument('--bits_opacity', type=int, default=12, help='no. of bits for opacity')
    parser.add_argument('--bits_scale', type=int, default=12, help='no. of bits for scale')
    parser.add_argument('--bits_rot', type=int, default=12, help='no. of bits for rot')
    parser.add_argument('--percentiles', type=str, default='0.25,99.75', help='percentile range for min/max, e.g. "0.5,99.5"')
    parser.add_argument('--header_format', type=str, default='inria')
    args = parser.parse_args()

    percentiles = tuple(float(x) for x in args.percentiles.split(','))

    if args.quantize:
       
        data = read_ply(args.input_path, 'INRIA')
        geom = data[:, 0:3]
        sh_dc_rgb_2d = data[:, 3:6]   
        sh_ac_rgb_2d = data[:, 6:51] 
        opacity = data[:, 51:52]
        scale = data[:, 52:55]
        rot = data[:, 55:]

        sh_dc_rgb_3d = dc_2d_to_3d(sh_dc_rgb_2d)  
        sh_dc_yuv_3d = rgb2yuv_bt709(sh_dc_rgb_3d)      
        sh_dc = dc_3d_to_2d(sh_dc_yuv_3d)  

        sh_ac_rgb_3d = ac_2d_to_3d(sh_ac_rgb_2d)  
        sh_ac_yuv_3d = rgb2yuv_bt709(sh_ac_rgb_3d)      
        sh_ac = ac_3d_to_2d(sh_ac_yuv_3d) 

        bits_geom = args.bits_geom
        bits_sh_dc = args.bits_sh_dc
        bits_sh_ac = args.bits_sh_ac
        bits_opacity = args.bits_opacity
        bits_scale = args.bits_scale
        bits_rot = args.bits_rot

        q_geom, min_geom, max_geom = quantize_scalar(geom, bits_geom)
        q_sh_dc, min_sh_dc, max_sh_dc = quantize_scalar(sh_dc, bits_sh_dc)
        q_sh_ac, min_sh_ac, max_sh_ac = quantize_scalar(sh_ac, bits_sh_ac)
        q_sh = np.concatenate([q_sh_dc, q_sh_ac], axis=1)
        q_opacity, min_opacity, max_opacity = quantize_scalar(opacity, bits_opacity)
        q_scale, min_scale, max_scale = quantize_scalar(scale, bits_scale)
        rot_normal = normalize_rot(rot)
        q_rot, min_rot, max_rot = quantize_scalar(rot_normal, bits_rot)

        quantized_data = np.hstack([
            q_geom,
            q_sh,
            q_opacity,
            q_scale,
            q_rot,
        ])
        write_ply(quantized_data, args.output_path, 'PCRM')
        meta_dict = {
            'bits_geom': bits_geom, 'min_geom': min_geom, 'max_geom': max_geom,
            'bits_sh_dc': bits_sh_dc, 'min_sh_dc': min_sh_dc, 'max_sh_dc': max_sh_dc,
            'bits_sh_ac': bits_sh_ac, 'min_sh_ac': min_sh_ac, 'max_sh_ac': max_sh_ac,
            'bits_opacity': bits_opacity, 'min_opacity': min_opacity, 'max_opacity': max_opacity,
            'bits_scale': bits_scale, 'min_scale': min_scale, 'max_scale': max_scale,
            'bits_rot': bits_rot, 'min_rot': min_rot, 'max_rot': max_rot,
            
        }
        np.savez(args.meta_path, **meta_dict)
        print(f"quantized data saved to {args.output_path}")
        print(f"meta saved to {args.meta_path}, percentiles={percentiles}")

    if args.dequantize:
       
        print("-( dequant min-max )-") 
        data = read_ply(args.input_path, 'PCRM')
        npz = np.load(args.meta_path)

      
        b_pos = int(npz['bits_geom'])
        pmin = npz['min_geom']
        pmax = npz['max_geom']
        
        b_sdc = int(npz['bits_sh_dc'])
        dc_min = npz['min_sh_dc']
        dc_max = npz['max_sh_dc']
        
        b_sac = int(npz['bits_sh_ac'])
        ac_min = npz['min_sh_ac']
        ac_max = npz['max_sh_ac']
        
        b_op = int(npz['bits_opacity'])
        omin = npz['min_opacity']
        omax = npz['max_opacity']
        
        b_sc = int(npz['bits_scale'])
        smin = npz['min_scale']
        smax = npz['max_scale']
        
        b_rt = int(npz['bits_rot'])
        rmin = npz['min_rot']
        rmax = npz['max_rot']


        q_pos = data[:, 0:3]  
        q_sh_dc = data[:, 3:6]
        q_sh_ac = data[:, 6:51]
        q_opacity = data[:, 51:52]
        q_scale = data[:, 52:55]
        q_rot = data[:, 55:] 

       
        r_pos = dequantize_scalar(q_pos, b_pos, pmin, pmax)
        sh_dc = dequantize_scalar(q_sh_dc, b_sdc, dc_min, dc_max)
        sh_ac = dequantize_scalar(q_sh_ac, b_sac, ac_min, ac_max)
        r_opacity = dequantize_scalar(q_opacity, b_op, omin, omax)
        r_scale = dequantize_scalar(q_scale, b_sc, smin, smax)
        r_rot = dequantize_scalar(q_rot, b_rt, rmin, rmax)

        dequant_sh_DC_yuv_3d = dc_2d_to_3d(sh_dc)  
        dequant_sh_DC_rgb_3d = yuv2rgb_bt709(dequant_sh_DC_yuv_3d)     
        dequant_sh_DC = dc_3d_to_2d(dequant_sh_DC_rgb_3d)  
        
        dequant_sh_AC_yuv_3d = ac_2d_to_3d(sh_ac)  
        dequant_sh_AC_rgb_3d = yuv2rgb_bt709(dequant_sh_AC_yuv_3d)     
        dequant_sh_AC = ac_3d_to_2d(dequant_sh_AC_rgb_3d) 
               
        r_rot = normalize_rot(r_rot) 

        normal = np.zeros(r_pos.shape, dtype=float)

        dequant_data = np.hstack([r_pos, normal, dequant_sh_DC,dequant_sh_AC, r_opacity, r_scale, r_rot])
        
        write_ply(dequant_data, args.output_path, 'INRIA')
        print(f"dequantized data saved to {args.output_path}")

if __name__ == "__main__":
    main()
