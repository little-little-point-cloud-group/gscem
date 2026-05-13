import os
import re
import shutil
import random
import numpy as np
import argparse
from argparse import ArgumentParser
from icecream import ic
# ic.disable()
from cfg.get_cfg import load_yaml_config


def enc_cmd(encoder_path, cfg_path, ref_path, bin_path, mdf_path, log_path):
    command = [
        f"{encoder_path}",
        "-c",
        f"{cfg_path}",
        "-m",
        0,
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


def quant_cmd(scene, input_path, meta_path, output_path, quant_cfg_path):
    command = [
        "python processing.py",
        "quantize",
        "--scene",
        f"{scene}",
        "--input_path",
        f"{input_path}",
        "--output_path",
        f"{output_path}",
        "--meta_path",
        f"{meta_path}",
        "--quant_cfg_path",
        f"{quant_cfg_path}"
        ]
    
    command = ' '.join(map(str, command))
        
    return command


def deq_cmd(scene, input_path, meta_path, output_path, quant_cfg_path):
    command = [
        "python processing.py",
        "dequantize",
        "--scene",
        f"{scene}",
        "--input_path",
        f"{input_path}",
        "--output_path",
        f"{output_path}",
        "--meta_path",
        f"{meta_path}",
        "--quant_cfg_path",
        f"{quant_cfg_path}"
        ]
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


def metric_cmd(metric_binary_path, ref_path, rec_path, frame_index, output_path, metrics_cfg, width=1920, height=1080):
    command = [
        f"OMP_NUM_THREADS={metrics_cfg['OMP_NUM_THREADS']}",
        f"{metric_binary_path}",
        "-a",
        f"{ref_path}",
        "-b",
        f"{rec_path}",
        f"--onlyViewpoint={frame_index}",
        "-f 1",
        "-i 0",
        f"-s {metrics_cfg['save']}",
        f"--cpu={metrics_cfg['cpu']}",
        "--useCameraPosition=1",
        f"--width={width}",
        f"--height={height}",
        f"-o {output_path}",
        f"-v {metrics_cfg['verbose']}",
        f"--threads=1"
        ]
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
    parser.add_argument("--gstools", default="./submodules/mpeg-gsc-tools/gsTools/build/msvc/Release/bin/cameraPosition")
    parser.add_argument("--metric_bin_path", default="./submodules/mpeg-gsc-metrics/build/Release/bin/mpeg-gsc-metrics")
    parser.add_argument("--reset_dir", action='store_true')
    # force to pass
    parser.add_argument("--codec_cfg_dir", required=True)
    parser.add_argument("--data_dir", required=True)
    parser.add_argument("--scripts_dir", required=True)
    parser.add_argument("--bitstreams_dir", required=True)
    parser.add_argument("--quant_cfg_path", required=True)
    parser.add_argument("--metrics_cfg_path", required=True)

    args, _ = parser.parse_known_args()
    quant_cfg = load_yaml_config(args.quant_cfg_path)
    metrics_cfg = load_yaml_config(args.metrics_cfg_path)
    ratepoint_per_cond = quant_cfg['ratepoint_per_cond']
    
    args.src_dir = os.path.join(args.data_dir, 'src')
    bitstreams_dir = args.bitstreams_dir
    scripts_dir = args.scripts_dir

    if args.reset_dir:
        reset_folder(bitstreams_dir)
        # reset_folder(scripts_dir)
    
    quant_commands = []
    enc_commands = []
    dec_commands = []
    deq_commands = []
    cam_commands = []
    metric_commands = []

    scenes = quant_cfg['scenes']
    for scene in scenes.keys():
        category = scenes[scene]['category']

        frame = scenes[scene]['frame']
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
        
        src_cam_path = os.path.join(bitstreams_dir, scene + f'_cam.ply')
        cmd = cam_cmd(args.gstools, src_path, cam_path, image_path, src_cam_path)
        cam_commands.append(cmd)

        # quantization src
        quant_name = '_'.join([scene, frame_name, 'quant.ply'])
        quant_path = os.path.join(bitstreams_dir, quant_name)
        meta_name = '_'.join([scene, frame_name, 'meta.npz'])
        meta_path = os.path.join(bitstreams_dir, meta_name)

        cmd = quant_cmd(scene, src_path, meta_path, quant_path, args.quant_cfg_path)
        quant_commands.append(cmd)
        
        for cond in ratepoint_per_cond.keys():
            for rate in ratepoint_per_cond[cond]:
                identifier = '_'.join([scene, cond[:2], rate, 'frame'])
                
                enc_cfg_path = os.path.join(args.codec_cfg_dir, cond, scene, rate, 'encoder.cfg')
                dec_cfg_path = os.path.join(args.codec_cfg_dir, cond, scene, rate, 'decoder.cfg')
                
                bin_path = os.path.join(bitstreams_dir, f'{identifier}.bin')
                rec_path = os.path.join(*[bitstreams_dir, f'{identifier}.ply'])
                mdf_path = os.path.join(bitstreams_dir, f'{identifier}.txt')
                
                enc_log_path = os.path.join(bitstreams_dir, f'{identifier}_enc.log')
                dec_log_path = os.path.join(bitstreams_dir, f'{identifier}_dec.log')

                deq_path = os.path.join(bitstreams_dir, f'{identifier}_deq.ply')
                deq_cam_path = os.path.join(bitstreams_dir, f'{identifier}_cam.ply')
                metric_log_path = os.path.join(bitstreams_dir, f'{identifier}_metric.log')
                
                cmd = enc_cmd(args.encoder, enc_cfg_path, quant_path, bin_path, mdf_path, enc_log_path)
                enc_commands.append(cmd)

                cmd = dec_cmd(args.decoder, dec_cfg_path, bin_path, mdf_path, rec_path, dec_log_path)
                dec_commands.append(cmd)

                cmd = deq_cmd(scene, rec_path, meta_path, deq_path, args.quant_cfg_path)
                deq_commands.append(cmd)

                cmd = cam_cmd(args.gstools, deq_path, cam_path, image_path, deq_cam_path)
                cam_commands.append(cmd)
                
                rendering_info = scenes[scene]['rendering']
                total_frames = rendering_info['num_viewpoints']
                width = rendering_info['width']
                height = rendering_info['height']
                frame_indices = list(range(0, total_frames, rendering_info['viewport_sample_period']))
                for frame_idx in frame_indices:
                    metric_log_path = os.path.join(bitstreams_dir, f'{identifier}_{frame_idx}_metric.log')

                    cmd = metric_cmd(args.metric_bin_path, src_cam_path, deq_cam_path, frame_idx, metric_log_path, metrics_cfg, width, height)
                    metric_commands.append(cmd)
    
    random.shuffle(quant_commands)
    with open(f"{scripts_dir}/0_pre.sh", "w", encoding="utf-8") as f:
        for item in quant_commands:
            f.write(f"{item}\n")

    random.shuffle(enc_commands)
    with open(f"{scripts_dir}/1_enc.sh", "w", encoding="utf-8") as f:
        for item in enc_commands:
            f.write(f"{item}\n")

    random.shuffle(dec_commands)
    with open(f"{scripts_dir}/2_dec.sh", "w", encoding="utf-8") as f:
        for item in dec_commands:
            f.write(f"{item}\n")

    random.shuffle(deq_commands)
    with open(f"{scripts_dir}/3_deq.sh", "w", encoding="utf-8") as f:
        for item in deq_commands:
            f.write(f"{item}\n")

    random.shuffle(cam_commands)
    with open(f"{scripts_dir}/4_cam.sh", "w", encoding="utf-8") as f:
        for item in cam_commands:
            f.write(f"{item}\n")

    random.shuffle(metric_commands)
    with open(f"{scripts_dir}/5_metric.sh", "w", encoding="utf-8") as f:
        for item in metric_commands:
            f.write(f"{item}\n")