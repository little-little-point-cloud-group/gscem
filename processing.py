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


def quantize(gs_data, bits=18):
    
    _max, _min = np.max(gs_data, axis=0), np.min(gs_data, axis=0)
    scale = (2**bits - 1) / (_max - _min)
    ret = np.round((gs_data - _min) * scale).astype(int)
    
    return ret, _min, scale


def dequantize(ply_ata, _min, scale):
    
    ret = ply_ata / scale + _min
    
    return ret


def main():
    parser = argparse.ArgumentParser(description='3DGS PLY file preprocessing tool')
    parser.add_argument('--input_path', help='input PLY file path')
    parser.add_argument('--output_path', help='output PLY file path')
    parser.add_argument('--meta_path', help='min and scale of quantization')
    parser.add_argument('--quantize', action='store_true', help='quantize data to integers')
    parser.add_argument('--dequantize', action='store_true', help='dequantize data to floats')
    parser.add_argument('--bits_geom', type=int, default=18, help='no. of bits for geom')
    parser.add_argument('--bits_attr', type=int, default=12, help='no. of bits for attr')
    parser.add_argument('--header_format', type=str, default='inria')
    
    args = parser.parse_args()
    
    if args.quantize:
        data = read_ply(args.input_path, 'INRIA')
        ic(data)
        ic(len(data))
        ic(len(data[0]))
        geom, attr = np.hsplit(data, [3])
        ic(geom.shape)
        ic(attr.shape)
        quantized_geom, min_geom, scale_geom = quantize(geom, args.bits_geom)
        quantized_attr, min_attr, scale_attr = quantize(attr, args.bits_attr)
        quantized_data = np.hstack([quantized_geom, quantized_attr])
        write_ply(quantized_data, args.output_path, 'PCRM')
        np.savez(args.meta_path, min_geom, scale_geom, min_attr, scale_attr)

    
    if args.dequantize:
        data = read_ply(args.input_path, 'PCRM')
        geom, attr = np.hsplit(data, [3])
        
        npz = np.load(args.meta_path)
        min_geom = npz['arr_0']
        scale_geom = npz['arr_1']
        min_attr = npz['arr_2']
        scale_attr = npz['arr_3']

        ic(min_geom, scale_geom, min_attr, scale_attr)

        deqaunt_geom = dequantize(geom, min_geom, scale_geom)
        dequant_attr = dequantize(attr, min_attr, scale_attr)
        normal = np.zeros(deqaunt_geom.shape, dtype=float)
        dequant_data = np.hstack([deqaunt_geom, normal, dequant_attr])
        write_ply(dequant_data, args.output_path, 'INRIA')
        

if __name__ == "__main__":
    main()
