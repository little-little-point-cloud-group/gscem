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
    # 'nx', 'ny', 'nz',
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
ic(PCRM_FORMATS)

def convert_binary_to_text(input_path, output_path):
    plydata = PlyData.read(input_path)
    plydata.text = True
    plydata.write(output_path)

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

def sh_2d_to_3d(sh_2d):
    vertex = sh_2d.shape[0]
    f_rest_numel = 45
    Ncoef = int(f_rest_numel / 3) + 1
    sh_3d = np.zeros(shape=(vertex, Ncoef, 3))
    sh_dc = sh_2d[:, 0:3]
    sh_ac = sh_2d[:, 3:48]
    for i in range(3):
        sh_3d[:, 0, i] = sh_dc[:, i]
    for j in range(1, Ncoef):
        for i in range(3):
            sh_3d[:, j, i] = sh_ac[:, (j-1) + i*(Ncoef-1)]
    return sh_3d

def sh_3d_to_2d(sh_3d):
    vertex = sh_3d.shape[0]
    Ncoef = sh_3d.shape[1]
    sh_2d = np.zeros(shape=(vertex, 48))
    sh_dc = sh_3d[:, 0, :]
    sh_ac = np.zeros(shape=(vertex, 45))
    for j in range(1, Ncoef):
        for i in range(3):
            sh_ac[:, (j-1) + i*(Ncoef-1)] = sh_3d[:, j, i]
    sh_2d[:, 0:3] = sh_dc
    sh_2d[:, 3:48] = sh_ac
    return sh_2d

def rgb2yuv(sh_rgb):
    sh_yuv = np.zeros(sh_rgb.shape)
    for i in range(sh_rgb.shape[1]):
        sh_yuv[:,i,0] = +0.29900*sh_rgb[:,i,0] + 0.58700*sh_rgb[:,i,1] + 0.11400*sh_rgb[:,i,2]
        sh_yuv[:,i,1] = -0.14713*sh_rgb[:,i,0] - 0.28886*sh_rgb[:,i,1] + 0.43600*sh_rgb[:,i,2]
        sh_yuv[:,i,2] = +0.61500*sh_rgb[:,i,0] - 0.51498*sh_rgb[:,i,1] - 0.10001*sh_rgb[:,i,2]
    return sh_yuv

def yuv2rgb(sh_yuv):
    sh_rgb = np.zeros(sh_yuv.shape)
    for i in range(sh_yuv.shape[1]):
        sh_rgb[:,i,0] = sh_yuv[:,i,0] + 1.13983*sh_yuv[:,i,2]
        sh_rgb[:,i,1] = sh_yuv[:,i,0] - 0.39465*sh_yuv[:,i,1] - 0.58060*sh_yuv[:,i,2]
        sh_rgb[:,i,2] = sh_yuv[:,i,0] + 2.03211*sh_yuv[:,i,1]
    return sh_rgb

# ====== Unified Quantization Methods ---- BEGIN ======
def dir_interval(x, limits):
    return np.clip((x-limits[0])/(limits[1]-limits[0]), 0, 1)

def inv_interval(x, limits):
    return x*(limits[1]-limits[0])+limits[0]

def quantize(x, bits):
    q = np.round(x * (2**bits))
    q = np.clip(q, 0, (2**bits) - 1)
    return q

def dequantize(q, bits):
    x = q / (2**bits)
    return x

def dir_pos(pos, start_pos, size_pos):
    q_pos = np.zeros(pos.shape)
    for i in range(3):
        q_pos[:,i] = pos[:,i] - start_pos[i]
    q_pos = q_pos/size_pos
    return q_pos

def inv_pos(q_pos, start_pos, size_pos):
    pos = q_pos * size_pos
    for i in range(3):
        pos[:,i] = pos[:,i] + start_pos[i]
    return pos

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

def get_limits_from_data(geom, sh, opacity, scale, rot):
    # start_pos, size_pos: position
    limits_pos = [[geom[:,k].min() for k in range(3)], 256]  # start_pos=min, size_pos=256
    limits_sh = [-4, 4]
    limits_opacity = [-7, 18]
    limits_scale = [-26, 4]
    limits_rot = [-1, 1]
    return [
        limits_pos,
        limits_sh,
        limits_opacity,
        limits_scale,
        limits_rot
    ]
# ====== Unified Quantization Methods ---- END ======

def main():
    parser = argparse.ArgumentParser(description='3DGS PLY file preprocessing tool')
    parser.add_argument('--input_path', help='input PLY file path')
    parser.add_argument('--output_path', help='output PLY file path')
    parser.add_argument('--meta_path', help='min and scale/limits of quantization')
    parser.add_argument('--quantize', action='store_true', help='quantize data to integers')
    parser.add_argument('--dequantize', action='store_true', help='dequantize data to floats')
    parser.add_argument('--bits_geom', type=int, default=18, help='no. of bits for geom')
    parser.add_argument('--bits_sh', type=int, default=12, help='no. of bits for sh')
    parser.add_argument('--bits_opacity', type=int, default=12, help='no. of bits for opacity')
    parser.add_argument('--bits_scale', type=int, default=12, help='no. of bits for scale')
    parser.add_argument('--bits_rot', type=int, default=12, help='no. of bits for rot')
    parser.add_argument('--header_format', type=str, default='inria')
    args = parser.parse_args()

    bits = [args.bits_geom, args.bits_sh, args.bits_opacity, args.bits_scale, args.bits_rot]

    if args.quantize:
        data = read_ply(args.input_path, 'INRIA')
        geom = data[:, 0:3]
        sh_rgb_2d = data[:, 3:51]
        opacity = data[:, 51:52]
        scale = data[:, 52:55]
        rot = data[:, 55:]

        sh_rgb_3d = sh_2d_to_3d(sh_rgb_2d)
        sh_yuv_3d = rgb2yuv(sh_rgb_3d)
        # sh shape = (N, M, 3), need to flatten to (N, M*3)
        sh_flat = sh_yuv_3d.reshape(sh_yuv_3d.shape[0], -1)

        # get limits
        limits = get_limits_from_data(geom, sh_flat, opacity, scale, rot)
        bits_geom, bits_sh, bits_opac, bits_scale, bits_rot = bits
        limits_pos, limits_sh, limits_opacity, limits_scale, limits_rot = limits
        start_pos = limits_pos[0]
        size_pos = limits_pos[1]

        # positions
        q_geom = quantize(dir_pos(geom, start_pos, size_pos), bits_geom)
        # harmonics
        q_sh = quantize(dir_interval(sh_flat, limits_sh), bits_sh)
        # opacity
        q_opacity = quantize(dir_interval(opacity, limits_opacity), bits_opac)
        # scale
        q_scale = quantize(dir_interval(scale, limits_scale), bits_scale)
        # rotation
        rot_normalized = normalize_rot(rot)
        q_rot = quantize(dir_interval(rot_normalized, limits_rot), bits_rot)

        quantized_data = np.hstack([q_geom, q_sh, q_opacity, q_scale, q_rot])
        write_ply(quantized_data, args.output_path, 'PCRM')
        # Save bits and limits arrays for dequantization (like config)
        np.savez(args.meta_path, bits=np.array(bits), limits=np.array(limits, dtype=object), start_pos=np.array(start_pos), size_pos=np.array(size_pos))
        print(f"quantized data saved to {args.output_path}")
        print(f"bits and limits saved to {args.meta_path}")

    if args.dequantize:
        data = read_ply(args.input_path, 'PCRM')
        q_geom = data[:, 0:3]
        q_sh = data[:, 3:3+48]
        q_opacity = data[:, 3+48:3+48+1]
        q_scale = data[:, 3+48+1:3+48+1+3]
        q_rot = data[:, 3+48+1+3:3+48+1+3+4]

        npz = np.load(args.meta_path, allow_pickle=True)
        bits = npz['bits']
        limits = npz['limits']
        start_pos = npz['start_pos']
        size_pos = npz['size_pos']
        bits_geom, bits_sh, bits_opac, bits_scale, bits_rot = bits
        limits_pos, limits_sh, limits_opacity, limits_scale, limits_rot = limits

        # positions
        r_geom = inv_pos(dequantize(q_geom, bits_geom), start_pos, size_pos)
        # harmonics
        r_sh_flat = inv_interval(dequantize(q_sh, bits_sh), limits_sh)
        # opacity
        r_opacity = inv_interval(dequantize(q_opacity, bits_opac), limits_opacity)
        # scale
        r_scale = inv_interval(dequantize(q_scale, bits_scale), limits_scale)
        # rotation
        r_rot = inv_interval(dequantize(q_rot, bits_rot), limits_rot)

        # rot normalization (just like quantization did)
        r_rot = normalize_rot(r_rot)

        # sh shape recovery
        N = r_sh_flat.shape[0]
        M = r_sh_flat.shape[1] // 3
        r_sh_yuv_3d = r_sh_flat.reshape(N, M, 3)
        r_sh_rgb_3d = yuv2rgb(r_sh_yuv_3d)
        # flatten to (N, 48)
        r_sh_2d = sh_3d_to_2d(r_sh_rgb_3d)

        # add dummy normal (0) after geom (to match INRIA format)
        normal = np.zeros(r_geom.shape, dtype=float)
        dequant_data = np.hstack([r_geom, normal, r_sh_2d, r_opacity, r_scale, r_rot])
        write_ply(dequant_data, args.output_path, 'INRIA')
        print(f"dequantized data saved to {args.output_path}")

if __name__ == "__main__":
    main()