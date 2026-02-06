import os
import argparse
import shutil
import numpy as np
from plyfile import PlyData
from icecream import ic
ic.disable()
import hashlib
from functools import partial

# BT.709 conversion matrix
RGB2YUV_MAT = np.array([
    [0.212600, -0.114572, 0.500000],
    [0.715200, -0.385428, -0.454153],
    [0.072200, 0.500000, -0.045847]
])

YUV2RGB_MAT = np.array([
    [1.0, 1.0, 1.0],
    [0.0, -0.18733, 1.85563],
    [1.57480, -0.46813, 0.0]
])


def rgb_to_yuv_sh(rgb_data):
    """
    1. convert (M, 48) RGB PLY data to (M, 16, 3) YUV SH coefficients.
    2. rgb2yuv conversion
    3. (M, 16, 3) to (M, 48)
    """
    M = rgb_data.shape[0]
    ic(rgb_data[0, :])

    # Reshape logic
    rgb_dc = rgb_data[:, 0:3].reshape(M, 1, 3)
    ic(rgb_dc[0, :, :])
    rgb_ac = rgb_data[:, 3:48].reshape(M, 3, 15).transpose(0, 2, 1)
    ic(rgb_ac[0, :, :])
    sh_rgb = np.concatenate([rgb_dc, rgb_ac], axis=1)
    ic(sh_rgb[0, 0:2, :])

    sh_yuv = np.matmul(sh_rgb, RGB2YUV_MAT)
    ic(sh_yuv[0, 0:2, :])
    yuv_dc = sh_yuv[:, 0, :]
    ic(yuv_dc[0, :])
    yuv_ac = sh_yuv[:, 1:, :].transpose(0, 2, 1).reshape(M, 45)
    ic(yuv_ac[0, :])

    flat_yuv = np.concatenate([yuv_dc, yuv_ac], axis=1)
    ic(flat_yuv[0, :])
    return flat_yuv


def yuv_to_rgb_sh(yuv_data):
    """
    1. Convert (M, 48) YUV flat data to (M, 16, 3) YUV SH coefficients.
    2. yuv2rgb conversion (BT.709).
    3. (M, 16, 3) to (M, 48) RGB flat data.
    """
    M = yuv_data.shape[0]

    yuv_dc = yuv_data[:, 0:3].reshape(M, 1, 3)
    yuv_ac = yuv_data[:, 3:48].reshape(M, 3, 15).transpose(0, 2, 1)
    sh_yuv = np.concatenate([yuv_dc, yuv_ac], axis=1)

    sh_rgb = np.matmul(sh_yuv, YUV2RGB_MAT)
    rgb_dc = sh_rgb[:, 0, :]
    rgb_ac = sh_rgb[:, 1:, :].transpose(0, 2, 1).reshape(M, 45)

    flat_rgb = np.concatenate([rgb_dc, rgb_ac], axis=1)

    return flat_rgb

def md5sum(filename):
  with open(filename, mode='rb') as f:
    d = hashlib.md5()
    for buf in iter(partial(f.read, 128), b''):
      d.update(buf)
  return d.hexdigest()


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
# ic(PCRM_FORMATS)


def convert_binary_to_text(input_path, output_path):
    
    plydata = PlyData.read(input_path)
    plydata.text = True
    plydata.write(output_path)


def convert_text_to_binary(input_path, output_path):
    
    plydata = PlyData.read(input_path)
    plydata.text = False
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


def quantize_sigmoid(gs_data, bits, sigma_scale):
    
    ret_data = []
    ret_mu = []
    ret_sigma = []

    quant_max = 2 ** bits
    for i in range(gs_data.shape[1]):
        col_data = gs_data[:, i]

        _mu = np.mean(col_data)
        _sigma = np.std(col_data)
        _sigma *=  sigma_scale
        shaped = 1 / (1 + np.exp(-1 * (col_data - _mu) / _sigma))
        
        qed = np.round(shaped * quant_max).astype(int)
        qed = np.clip(qed, 0, quant_max - 1)

        ret_data.append(qed)
        ret_mu.append(_mu)
        ret_sigma.append(_sigma)

    ret = np.column_stack(ret_data)
    ret_mu = np.array(ret_mu)
    ret_sigma = np.array(ret_sigma)
    
    return ret, ret_mu, ret_sigma


def dequantize_sigmoid(ply_ata, bits, _mu, _sigma):

    quant_max = 2**bits
    ret = ply_ata / quant_max
    ret = np.clip(ret, 1e-10, 1 - 1e-10)
    ic([np.min(ret, axis=0), np.max(ret, axis=0)])
    ret = _mu - _sigma * np.log(1 / ret - 1)
    # ret = _mu + _sigma * np.log(ret - 1)
    
    return ret


def quantize_linear(gs_data, bits):
    quant_max = 2**bits
    ret_data = []
    ret_min = []
    ret_r = []
    for i in range(gs_data.shape[1]):
        col_data = gs_data[:, i]
        
        _min, _max = np.min(col_data), np.max(col_data)
        _r = _max - _min
        ic([_min, _max, _r])
        
        ret = np.round((col_data - _min) * quant_max / _r).astype(int)
        ret = np.clip(ret, 0, quant_max - 1)

        ret_data.append(ret)
        ret_min.append(_min)
        ret_r.append(_r)

    ret = np.column_stack(ret_data)
    ret_min = np.array(ret_min)
    ret_r = np.array(ret_r)

    return ret, ret_min, ret_r

def quantize_linear_shared(gs_data, bits):
    quant_max = 2**bits
    _min = np.min(gs_data)
    _max = np.max(gs_data)
    _r = _max - _min
    ic([_min, _max, _r])

    ret = np.round((gs_data - _min) * quant_max / _r).astype(int)
    ret = np.clip(ret, 0, quant_max - 1)

    return ret, _min, _r
    
def dequantize_linear(ply_ata, bits, _low, _r):

    quant_max = 2**bits
    ret = ply_ata / quant_max * _r + _low
    
    return ret


def build_parser():
    parser = argparse.ArgumentParser(description='3DGS PLY file preprocessing tool')
    # parser.add_argument('--header_format', type=str, default='inria')

    subparsers = parser.add_subparsers(dest='command', required=True)
    p_quantize = subparsers.add_parser('quantize')
    p_quantize.add_argument('--meta_path', required=True, help='min and scale of quantization')
    p_quantize.add_argument('--input_path', required=True, help='input PLY file path')
    p_quantize.add_argument('--output_path', required=True, help='output PLY file path')
    p_quantize.add_argument('--bits_geom', type=int, required=True, help='no. of bits for geom')
    p_quantize.add_argument('--fn_geom', type=str, default='linear', help='no. of bits for geom')
    p_quantize.add_argument('--bits_dc', type=int, required=True, help='no. of bits for sh dc')
    p_quantize.add_argument('--fn_dc', type=str, default='linear', help='no. of bits for geom')
    p_quantize.add_argument('--bits_ac', type=int, required=True, help='no. of bits for sh ac')
    p_quantize.add_argument('--fn_ac', type=str, default='linear', help='no. of bits for geom')
    p_quantize.add_argument('--bits_opacity', type=int, required=True, help='no. of bits for opacity')
    p_quantize.add_argument('--fn_opacity', type=str, default='linear', help='no. of bits for geom')
    p_quantize.add_argument('--bits_scale', type=int, required=True, help='no. of bits for scale')
    p_quantize.add_argument('--fn_scale', type=str, default='linear', help='no. of bits for geom')
    p_quantize.add_argument('--bits_rotation', type=int, required=True, help='no. of bits for rotation')
    p_quantize.add_argument('--fn_rotation', type=str, default='linear', help='no. of bits for geom')
    p_quantize.add_argument('--color_space_conversion', action='store_true',
                            help='enable RGB to YUV conversion for SH dc/ac')

    p_dequantize = subparsers.add_parser('dequantize')
    p_dequantize.add_argument('--meta_path', required=True, help='min and scale of quantization')
    p_dequantize.add_argument('--input_path', required=True, help='input PLY file path')
    p_dequantize.add_argument('--output_path', required=True, help='output PLY file path')
    p_dequantize.add_argument('--bits_geom', type=int, required=True, help='no. of bits for geom')
    p_dequantize.add_argument('--fn_geom', type=str, default='linear', help='no. of bits for geom')
    p_dequantize.add_argument('--bits_dc', type=int, required=True, help='no. of bits for sh dc')
    p_dequantize.add_argument('--fn_dc', type=str, default='linear', help='no. of bits for geom')
    p_dequantize.add_argument('--bits_ac', type=int, required=True, help='no. of bits for sh ac')
    p_dequantize.add_argument('--fn_ac', type=str, default='linear', help='no. of bits for geom')
    p_dequantize.add_argument('--bits_opacity', type=int, required=True, help='no. of bits for opacity')
    p_dequantize.add_argument('--fn_opacity', type=str, default='linear', help='no. of bits for geom')
    p_dequantize.add_argument('--bits_scale', type=int, required=True, help='no. of bits for scale')
    p_dequantize.add_argument('--fn_scale', type=str, default='linear', help='no. of bits for geom')
    p_dequantize.add_argument('--bits_rotation', type=int, required=True, help='no. of bits for rotation')
    p_dequantize.add_argument('--fn_rotation', type=str, default='linear', help='no. of bits for geom')
    p_dequantize.add_argument('--color_space_conversion', action='store_true',
                              help='enable YUV to RGB conversion for SH dc/ac')

    p_t2b = subparsers.add_parser('text2bin')
    p_t2b.add_argument('--input_path', required=True, help='input PLY file path')
    p_t2b.add_argument('--output_path', required=True, help='output PLY file path')
    
    p_b2t = subparsers.add_parser('bin2text')
    p_b2t.add_argument('--input_path', required=True, help='input PLY file path')
    p_b2t.add_argument('--output_path', required=True, help='output PLY file path')

    return parser


def main():
    
    parser = build_parser()
    args = parser.parse_args()
    print(args)
    
    if args.command == 'quantize':
        data = read_ply(args.input_path, 'INRIA')
        ic(data)
        ic(len(data))
        ic(len(data[0]))
        geom, dc, ac, opacity, scale, rotation = np.hsplit(data, [3, 6, 51, 52, 55])
        ic([geom.shape, dc.shape, ac.shape, opacity.shape, scale.shape, rotation.shape])

        #rgb2yuv
        if args.color_space_conversion:
            dc_ac = np.concatenate([dc, ac], axis=1)
            dc_ac_yuv = rgb_to_yuv_sh(dc_ac)
            dc, ac = np.hsplit(dc_ac_yuv, [3])
            ic([dc_ac.shape])
            ic([dc_ac_yuv.shape])
            ic(np.max(dc_ac_yuv))
            ic(np.min(dc_ac_yuv))
            ic([dc.shape, ac.shape])

        if args.fn_geom == 'linear':
            q_geom, geom_low, geom_range = quantize_linear(geom, args.bits_geom)
        else:
            q_geom, geom_low, geom_range = quantize_sigmoid(geom, args.bits_geom)
        
        if args.fn_dc == 'linear':
            # q_sh_dc, sh_dc_low, sh_dc_range = quantize_linear_shared(dc, args.bits_dc)
            q_sh_dc, sh_dc_low, sh_dc_range = quantize_linear(dc, args.bits_dc)
        else:
            q_sh_dc, sh_dc_low, sh_dc_range = quantize_sigmoid(dc, args.bits_dc)

        if args.fn_ac == 'linear':
            # q_sh_ac, sh_ac_low, sh_ac_range = quantize_linear(ac, args.bits_ac, 20)
            # q_sh_ac, sh_ac_low, sh_ac_range = quantize_linear_shared(ac, args.bits_ac)
            q_sh_ac, sh_ac_low, sh_ac_range = quantize_linear(ac, args.bits_ac)
        else:
            q_sh_ac, sh_ac_low, sh_ac_range = quantize_sigmoid(ac, args.bits_ac, 20)
        
        if args.fn_opacity == 'linear':
            q_opacity, opacity_low, opacity_range = quantize_linear(opacity, args.bits_opacity)
        else:
            q_opacity, opacity_low, opacity_range = quantize_sigmoid(opacity, args.bits_opacity)

        if args.fn_scale == 'linear':
            q_scale, scale_low, scale_range = quantize_linear(scale, args.bits_scale)
        else:
            q_scale, scale_low, scale_range = quantize_sigmoid(scale, args.bits_scale)

        if args.fn_rotation == 'linear':
            q_rotation, rotation_low, rotation_range = quantize_linear(rotation, args.bits_rotation)
        else:
            q_rotation, rotation_low, rotation_range = quantize_sigmoid(rotation, args.bits_rotation)
        
        ic([q_geom.shape, q_sh_dc.shape, q_sh_ac.shape, q_opacity.shape, q_scale.shape, q_rotation.shape])
        ret = np.column_stack([q_geom, q_sh_dc, q_sh_ac, q_opacity, q_scale, q_rotation])#一行代表一个对象的完整参数向量，一列代表一个属性

        write_ply(ret, args.output_path, 'PCRM')
        np.savez(args.meta_path, geom_low, geom_range, sh_dc_low, sh_dc_range,
                 sh_ac_low, sh_ac_range, opacity_low, opacity_range,
                 scale_low, scale_range, rotation_low, rotation_range)
        
        print(f"quantized file {args.output_path} md5: {md5sum(args.output_path)}")
        print(f"mins and maxs {args.meta_path} md5: {md5sum(args.meta_path)}")

    
    if args.command == 'dequantize':
        data = read_ply(args.input_path, 'PCRM')
        geom, dc, ac, opacity, scale, rotation = np.hsplit(data, [3, 6, 51, 52, 55])
        
        npz = np.load(args.meta_path)

        geom_low, geom_range = npz['arr_0'], npz['arr_1']
        if args.fn_geom == 'linear':
            dq_geom = dequantize_linear(geom, args.bits_geom, geom_low, geom_range)
        else:
            dq_geom = dequantize_sigmoid(geom, args.bits_geom, geom_low, geom_range)
        
        sh_dc_low, sh_dc_range = npz['arr_2'], npz['arr_3']
        if args.fn_dc == 'linear':
            dq_sh_dc = dequantize_linear(dc, args.bits_dc, sh_dc_low, sh_dc_range)
        else:
            dq_sh_dc = dequantize_sigmoid(dc, args.bits_dc, sh_dc_low, sh_dc_range)
        
        sh_ac_low, sh_ac_range = npz['arr_4'], npz['arr_5']
        if args.fn_ac == 'linear':
            dq_sh_ac = dequantize_linear(ac, args.bits_ac, sh_ac_low, sh_ac_range)
        else:
            dq_sh_ac = dequantize_sigmoid(ac, args.bits_ac, sh_ac_low, sh_ac_range)
        
        opacity_low, opacity_range = npz['arr_6'], npz['arr_7']
        if args.fn_opacity == 'linear':
            dq_opacity = dequantize_linear(opacity, args.bits_opacity, opacity_low, opacity_range)
        else:
            dq_opacity = dequantize_sigmoid(opacity, args.bits_opacity, opacity_low, opacity_range)

        scale_low, scale_range = npz['arr_8'], npz['arr_9']
        if args.fn_scale == 'linear':
            dq_scale = dequantize_linear(scale, args.bits_scale, scale_low, scale_range)
        else:
            dq_scale = dequantize_sigmoid(scale, args.bits_scale, scale_low, scale_range)
            
        rotation_low, rotation_range = npz['arr_10'], npz['arr_11']
        if args.fn_rotation == 'linear':
            dq_rotation = dequantize_linear(rotation, args.bits_rotation, rotation_low, rotation_range)
        else:
            dq_rotation = dequantize_sigmoid(rotation, args.bits_rotation, rotation_low, rotation_range)
            
        if args.color_space_conversion:
            dc_ac = np.concatenate([dq_sh_dc, dq_sh_ac], axis=1)
            dc_ac_rgb = yuv_to_rgb_sh(dc_ac)
            dq_sh_dc, dq_sh_ac = np.hsplit(dc_ac_rgb, [3])
            ic([dc_ac.shape])
            ic([dc_ac_rgb.shape])
            ic(np.max(dc_ac_rgb))
            ic(np.min(dc_ac_rgb))
            ic([dc.shape, ac.shape])

        normal = np.zeros(dq_geom.shape, dtype=float)
        dq_data = np.column_stack([dq_geom, normal, dq_sh_dc, dq_sh_ac, dq_opacity, dq_scale, dq_rotation])
        
        write_ply(dq_data, args.output_path, 'INRIA')
        print(f"dequantized data saved to {args.output_path}")

    if args.command == 'text2bin':
        convert_text_to_binary(args.input_path, args.output_path)
    
    if args.command == 'bin2text':
        convert_binary_to_text(args.input_path, args.output_path)


if __name__ == "__main__":
    main()
