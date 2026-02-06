import os
import re
import shutil
import random
import numpy as np
import argparse
from argparse import ArgumentParser
from icecream import ic
# ic.disable()
from cfg.cfg_quant import load_config


def enc_cmd(encoder_path, cfg_path, ref_path, bin_path, mdf_path, log_path):
    command = [
        f"{encoder_path}",
        "-c",
        f"{cfg_path}",
        "-m",
        1,
        "-i",
        f"{ref_path}",
        "-b",
        f"{bin_path}",
        "-mdf",
        f"{mdf_path}",
        ">",
        f"{log_path}"
        ]
    command = ' '.join(map(str, command))
        
    return command


def dec_cmd(decoder_path, cfg_path, bin_path, mdf_path, rec_path, log_path):
    command = [
        f"{decoder_path}",
        "-c",
        f"{cfg_path}",
        "-b",
        f"{bin_path}",
        "-mdf",
        f"{mdf_path}",
        "-r",
        f"{rec_path}",
        ">",
        f"{log_path}"
    ]
    command = ' '.join(map(str, command))
    
    return command


def quant_cmd(input_path, meta_path, output_path, quant_para, color_space_conversion=False):
    command = [
        "python3 processing.py",
        "quantize",
        "--input_path",
        f"{input_path}",
        "--meta_path",
        f"{meta_path}",
        "--output_path",
        f"{output_path}",
        "--bits_geom",
        f"{quant_para['bits_geom']}",
        "--fn_geom",
        f"{quant_para['fn_geom']}",
        "--bits_dc",
        f"{quant_para['bits_dc']}",
        "--fn_dc",
        f"{quant_para['fn_dc']}",
        "--bits_ac",
        f"{quant_para['bits_ac']}",
        "--fn_ac",
        f"{quant_para['fn_ac']}",
        "--bits_opacity",
        f"{quant_para['bits_opacity']}",
        "--fn_opacity",
        f"{quant_para['fn_opacity']}",
        "--bits_scale",
        f"{quant_para['bits_scale']}",
        "--fn_scale",
        f"{quant_para['fn_scale']}",
        "--bits_rotation",
        f"{quant_para['bits_rotation']}",
        "--fn_rotation",
        f"{quant_para['fn_rotation']}"
        ]
    if color_space_conversion:
        command.append("--color_space_conversion")
    
    command = ' '.join(map(str, command))
        
    return command


def deq_cmd(input_path, meta_path, output_path, quant_para, color_space_conversion=False):
    command = [
        "python3 processing.py",
        "dequantize",
        "--input_path",
        f"{input_path}",
        "--meta_path",
        f"{meta_path}",
        "--output_path",
        f"{output_path}",
        "--bits_geom",
        f"{quant_para['bits_geom']}",
        "--fn_geom",
        f"{quant_para['fn_geom']}",
        "--bits_dc",
        f"{quant_para['bits_dc']}",
        "--fn_dc",
        f"{quant_para['fn_dc']}",
        "--bits_ac",
        f"{quant_para['bits_ac']}",
        "--fn_ac",
        f"{quant_para['fn_ac']}",
        "--bits_opacity",
        f"{quant_para['bits_opacity']}",
        "--fn_opacity",
        f"{quant_para['fn_opacity']}",
        "--bits_scale",
        f"{quant_para['bits_scale']}",
        "--fn_scale",
        f"{quant_para['fn_scale']}",
        "--bits_rotation",
        f"{quant_para['bits_rotation']}",
        "--fn_rotation",
        f"{quant_para['fn_rotation']}"
        ]
    if color_space_conversion:
        command.append("--color_space_conversion")
    command = ' '.join(map(str, command))
        
    return command


def cam_cmd(cam_binary_path, ply_path, cam_path, image_path, output_path):
    command = [
        f"{cam_binary_path}",
        f"--input={ply_path}",
        f"--camera={cam_path}",
        f"--image={image_path}",
        f"--output={output_path}"
        ]
    command = ' '.join(map(str, command))
        
    return command


def metric_cmd(
    metric_binary_path,
    ref_path,
    rec_path,
    output_path,
    width=1920,
    height=1080,
    omp_threads=None,
    save_images=True,
):
    prefix = []
    if omp_threads:
        prefix.append(f"OMP_NUM_THREADS={omp_threads}")
    command = [
        *prefix,
        f"{metric_binary_path}",
        "-a",
        f"{ref_path}",
        "-b",
        f"{rec_path}",
        "-f 1",
        "-i 0",
        f"--cpu=1",
        "--useCameraPosition=1",
        f"--width={width}",
        f"--height={height}",
        f"-o {output_path}",
        "-v 1"
        ]
    if save_images:
        command.insert(-2, "-s 1")  # optional video dump; default disabled
    command = ' '.join(map(str, command))
        
    return command


def reset_folder(folder_path: str) -> None:
    if os.path.exists(folder_path):
        shutil.rmtree(folder_path)
    os.makedirs(folder_path, exist_ok=True)
 

if __name__ == "__main__":
    parser = ArgumentParser(description="generate all cmds to enc and dec")
    parser.add_argument("--encoder", default="./submodules/avs-pcc-pcrm/build/avs-pcc-encoder")
    parser.add_argument("--decoder", default="./submodules/avs-pcc-pcrm/build/avs-pcc-decoder")
    parser.add_argument("--gstools", default="/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/execution/cameraPosition")
    parser.add_argument("--metric_bin_path", default="/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/execution/mpeg-gsc-metrics")
    parser.add_argument("--cfg_dir", default='./cfg')
    parser.add_argument("--quant_cfg_path", default='./cfg/cfg_quant/cfg_quant_version.json')
    # change the dir to src 3dgs data
    parser.add_argument("--data_dir", default="/data1/openi/data")
    parser.add_argument("--script_dir", default="./scripts")
    parser.add_argument("--test_id", required=True)
    parser.add_argument("--metric_threads", type=int, default=21,
                        help="Set OMP_NUM_THREADS for mpeg-gsc-metrics (per-command).")
    parser.add_argument(
        "--save_metric_images",
        action="store_true",
        help="If set, pass -s 1 to mpeg-gsc-metrics to dump .rgb frames (disabled by default).",
    )

    args, _ = parser.parse_known_args()
    quant_cfg = load_config(args.quant_cfg_path)
    PCRM_methods = quant_cfg['PCRM_methods']
    ratepoint_per_cond = quant_cfg['ratepoint_per_cond']
    
    args.src_dir = os.path.join(args.data_dir, 'src')
    args.enc_dir = os.path.join(args.data_dir, args.test_id)
    
    reset_folder(args.enc_dir)
    
    quant_commands = []
    enc_commands = []
    dec_commands = []
    deq_commands = []
    cam_commands = []
    metric_commands = []

    scenes = quant_cfg['scenes']
    quant_bits = quant_cfg['quant_bits']
    quant_fn = quant_cfg['quant_fn']
    for scene in scenes.keys():
        scene_para = scenes[scene]
        ic(scene_para)
        category = scene_para['category']
        quant_para = quant_bits[category]
        quant_para.update(quant_fn[category])
        color_space_conversion = quant_cfg['color_space_conversion'].get(category, False)

        frame = scene_para['frame']
        frame_name = re.split('\.', frame)[0]

        # src: inria recon file (w/ normal)
        src_path = os.path.join(args.src_dir, scene, frame)

        # add cam in src
        cam_path = os.path.join(args.src_dir, scene, 'cameras.bin')
        if not os.path.exists(cam_path):
            cam_path = os.path.join(args.src_dir, scene, 'cameras.txt')

        image_path = os.path.join(args.src_dir, scene, 'images.bin')
        if not os.path.exists(image_path):           
            image_path = os.path.join(args.src_dir, scene, 'images.txt')
        
        src_cam_path = os.path.join(args.enc_dir, scene + f'_cam.ply')
        cmd = cam_cmd(args.gstools, src_path, cam_path, image_path, src_cam_path)
        cam_commands.append(cmd)

        # quantization src
        quant_name = '_'.join([scene, frame_name, 'quant.ply'])
        quant_path = os.path.join(args.enc_dir, quant_name)
        meta_name = '_'.join([scene, frame_name, 'meta.npz'])
        meta_path = os.path.join(args.enc_dir, meta_name)

        cmd = quant_cmd(src_path, meta_path, quant_path, quant_para, color_space_conversion)
        quant_commands.append(cmd)
        
        for method in PCRM_methods:
            for cond in ratepoint_per_cond.keys():
                for rate in ratepoint_per_cond[cond]:
                    idendifier = '_'.join([scene, frame_name, cond[:2], rate, method])
                    
                    enc_cfg_path = os.path.join(args.cfg_dir, f'cfg_{method}', cond, scene, rate, 'encoder.cfg')
                    dec_cfg_path = os.path.join(args.cfg_dir, f'cfg_{method}', cond, scene, rate, 'decoder.cfg')
                    
                    bin_path = os.path.join(args.enc_dir, f'{idendifier}.bin')
                    rec_path = os.path.join(args.enc_dir, f'{idendifier}.ply')
                    mdf_path = os.path.join(args.enc_dir, f'{idendifier}.txt')
                    
                    enc_log_path = os.path.join(args.enc_dir, f'{idendifier}_enc.log')
                    dec_log_path = os.path.join(args.enc_dir, f'{idendifier}_dec.log')

                    deq_path = os.path.join(args.enc_dir, f'{idendifier}_deq.ply')
                    deq_cam_path = os.path.join(args.enc_dir, f'{idendifier}_cam.ply')
                    metric_log_path = os.path.join(args.enc_dir, f'{idendifier}_metric.log')
                    
                    cmd = enc_cmd(args.encoder, enc_cfg_path, quant_path, bin_path, mdf_path, enc_log_path)
                    enc_commands.append(cmd)

                    cmd = dec_cmd(args.decoder, dec_cfg_path, bin_path, mdf_path, rec_path, dec_log_path)
                    dec_commands.append(cmd)

                    cmd = deq_cmd(rec_path, meta_path, deq_path, quant_para, color_space_conversion)
                    deq_commands.append(cmd)

                    cmd = cam_cmd(args.gstools, deq_path, cam_path, image_path, deq_cam_path)
                    cam_commands.append(cmd)

                    cmd = metric_cmd(args.metric_bin_path, src_cam_path, deq_cam_path, metric_log_path,
                                     omp_threads=args.metric_threads,
                                     save_images=args.save_metric_images)
                    metric_commands.append(cmd)
    
    ic(f"{args.script_dir}")
    random.shuffle(quant_commands)
    with open(f"{args.script_dir}/0_pre.sh", "w", encoding="utf-8") as f:
        for item in quant_commands:
            f.write(f"{item}\n")

    random.shuffle(enc_commands)
    with open(f"{args.script_dir}/1_enc.sh", "w", encoding="utf-8") as f:
        for item in enc_commands:
            f.write(f"{item}\n")

    random.shuffle(dec_commands)
    with open(f"{args.script_dir}/2_dec.sh", "w", encoding="utf-8") as f:
        for item in dec_commands:
            f.write(f"{item}\n")

    random.shuffle(deq_commands)
    with open(f"{args.script_dir}/3_deq.sh", "w", encoding="utf-8") as f:
        for item in deq_commands:
            f.write(f"{item}\n")

    random.shuffle(cam_commands)
    with open(f"{args.script_dir}/4_cam.sh", "w", encoding="utf-8") as f:
        for item in cam_commands:
            f.write(f"{item}\n")

    random.shuffle(metric_commands)
    with open(f"{args.script_dir}/5_metric.sh", "w", encoding="utf-8") as f:
        for item in metric_commands:
            f.write(f"{item}\n")
