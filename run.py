import os
import sys
from pathlib import Path
import filecmp
from tqdm import tqdm
import subprocess
import numpy as np
import openpyxl
import shutil
import multiprocessing
from my_tools import File
import shutil
import pandas as pd
from collect_results import parse_enc_log, parse_dec_log,seq_information,PSNR_columns,Bitstream_columns


# Codecs to be tested; 
# str(tmc3_selected) + '_tmc3' corresponds to the executable file path
tmc3_selected = {0:"/data/lym/3dgs_pcrm/submodules/avs-pcc-pcrm",
                 #1:"/data/lym/3dgs_pcrm/submodules/avs-pcc-pcrm"
                 }

cameraPosition="./cameraPosition"
mpeg_gsc_metrics = "./mpeg-gsc-metrics"


# Input file paths

template_excel = f"template/template.xlsm"  # Excel template with macros
output_excel="PCRM-transform__vs__PCRM-transform.xlsm"
thread_num_limit=[40,5]                     # Number of processes 

computeMetrics=1
save_iamge=0
save_pointCloud=0
computeSsim=1
computeIvssim=1
cpu=1


# Test branch
# Please select ONLY ONE
branch_selected = [
    # "cfg_predict",
     "cfg_transform",
]

# Test condition
# Please select ONLY ONE
condition_selected = {
     "C1": "C1-losslessG-lossyA-ai",
}

# Point cloud categories

class_selected =(
        "alley",
        "bartender",
        "bicycle",
        "cinema",
        "garden",
        "photo",
        "rocket",
        "toy",
)

PCC_sequence="/data/Sequence/AVS_data/src"

file_lock=multiprocessing.Lock()

def pre_process(output,pointCloud):
    cmd = [sys.executable, 'processing.py', 
           '--input_path', pointCloud,
           '--meta_path', output+"/"+"quantize.npz",
           '--output_path', output+"/quantized.ply",
           "--quantize"]
    
    subprocess.run(cmd, check=True)

def post_process(output):
    cmd = [sys.executable, 'processing.py', 
           '--input_path', output+"/decoder.ply",
           '--meta_path', output+"/"+"quantize.npz",
           '--output_path', output+"/dequantized.ply",
           "--dequantize"]
    
    subprocess.run(cmd, check=True)

def encoder(output,pointCloud,exe,tmc,isEncoder,branch_selecte):
    def parse_time_output(time_output):
        """Parse output from /usr/bin/time -v to extract MaxRSS"""
        maxrss = None
        for line in time_output.split('\n'):
            if 'Maximum resident set size (kbytes):' in line:
                try:
                    maxrss = int(line.split(':')[1].strip())
                    break
                except (ValueError, IndexError):
                    continue
        return maxrss

    if not os.path.exists(exe):
        print("No executable file found")
    frame=output.split("/")[-1]
    
    rate_point=output.split("/")[-2]
    os.makedirs(str(Path(output).parent)+"/txt", exist_ok=True)
    main = exe
    condition_selecte = condition_selected[list(condition_selected.keys())[0]]
    cfg_path = os.getcwd() + "/cfg" + str(tmc) + "/" + branch_selecte[0] + "/" + condition_selecte + "/" +pointCloud.split("/")[-2]+"/"+ rate_point
    para_encfg = "-c " + cfg_path + "/encoder.cfg"
    para_decfg = "-c " + cfg_path + "/decoder.cfg"

    para2 = "-i " + output + "/" + "quantized.ply"
    para3 = "-b " + output + "/" + "compress.bin"
    para4 = "-r " + output + "/" + "encoder.ply"
    para5="-m 1"
    para = "%s %s %s %s %s %s" % (main, para_encfg, para2, para3, para4,para5)  # Avoid warning output
    para=f'/usr/bin/time -v {para}'

    if isEncoder:
        _file =str(Path(output).parent)+"/txt/"+frame+"__Bitbream__encoder.txt"
        with open(_file, "w") as f:

            #para = "%s %s %s %s %s" % (main, para_encfg, para2, para3, para4)
            #usage=resource.getrusage(resource.RUSAGE_SELF)

            process = subprocess.Popen(
                para,
                shell=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )

            # Wait for process completion
            out, err = process.communicate()


            # Get peak memory of the subprocess
            peak_mem_kb = parse_time_output(err)
            print(out, file=f)
            print(f"Peak Memory: {peak_mem_kb} KB",file=f)

            if not os.path.exists(output + "/" + "encoder.ply"):
                print("Encoder reconstruction failed")
                print("Running configuration: " + para)

    else:
        _file = str(Path(output).parent)+"/txt/"+frame+ "__Bitbream__decoder.txt"
        with open(_file, "w") as f:
            para4 = "-r " + output + "/" + "decoder.ply"
            para = "%s %s %s %s %s" % (main, para_decfg, para3, para4,para5)
            para = f'/usr/bin/time -v {para}'

            process = subprocess.Popen(
                para,
                shell=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )

            # Wait for process completion
            out, err = process.communicate()

            # Get peak memory of the subprocess

            peak_mem_kb = parse_time_output(err)
            print(out, file=f)
            print(f"Peak Memory: {peak_mem_kb} KB", file=f)


            if not os.path.exists(output + "/" + "decoder.ply"):
                print("Decoder reconstruction failed")
                print("Running configuration: " + para)
                #print(r.stdout)
        if not filecmp.cmp(output + "/" + "encoder.ply", output + "/" + "decoder.ply", shallow=False):
            print("Encoder and decoder output mismatch")

def cam_to_ply(ply,camDIR,exe,output):
    para1="--input="+ply
    para2=para3=""

    if os.path.exists(camDIR+"/cameras.txt"):
        para2="--camera="+camDIR+"/cameras.txt"
    elif os.path.exists(camDIR+"/cameras.bin"):
        para2 = "--camera=" + camDIR + "/cameras.bin"
    else:
        print("No camera file found")

    if os.path.exists(camDIR+"/images.txt"):
        para3="--image="+camDIR+"/images.txt"
    elif os.path.exists(camDIR+"/images.bin"):
        para3 = "--image=" + camDIR + "/images.bin"
    else:
        print("No image file found")

    para4="--output="+output
    para5="--verbose=1"
    para = "%s %s %s %s %s %s" % (exe, para1, para2, para3, para4, para5)
    process = subprocess.Popen(
        para,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    # Wait for process completion
    out, err = process.communicate()

def metrics(exe,src,dec,frame_start,frame_num,width,hight):

    if computeMetrics==0:
        return 0

    print("render: "+dec)
    para1="-a "+src
    para2="-b "+dec
    para3="--width="+str(width)+" --height="+str(hight)

    para4="-i "+str(frame_start)+" -f "+str(frame_num)+" --cpu="+str(cpu) #cpu
    #para4 = "-i " + str(frame_start) + " -f " + str(frame_num)

    para5 = ("--useCameraPosition=1"
                 f" -s {save_iamge}"
                 f" --computeSsim={computeSsim}"
                 f" --computeIvssim={computeIvssim}"
                 )
        

    para = "OMP_NUM_THREADS=20 __NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia %s %s %s %s %s %s" % (exe, para1, para2, para3, para4, para5)
    #print(para)
    process = subprocess.Popen(
            para,
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        # Wait for process completion
    #print(para)
    out, err = process.communicate()
    metric_path = str(Path(dec).parent.parent)+"/metrics/" +f"__metrics.txt"
    os.makedirs(str(Path(dec).parent.parent)+"/metrics", exist_ok=True)
    with file_lock:
        with open(metric_path, "a") as f:
            print(out,file=f)


    print("success render: "+metric_path)

class Gaussian:
    def __init__(self):

        self.anchor_name="MPEG152-octree"
        self.test_name="MPEG152"
# ======================================
# Variables below do not need modification in the short term

        self.PCC_sequence=PCC_sequence
        self.rate_points=["r1","r2","r3","r4","r5"]


# ======================================
#Variables below need real-time update
        self.condition_selecte = None
        self.class_selecte = class_selected[0]
        self.metrics_path = dict()
        # Load Excel template with macros
        self.out_excel=output_excel
        self.wb =openpyxl.load_workbook(template_excel, keep_vba=True,read_only=False)
        self.branch=branch_selected
        def set_name():
            self.wb["Summary"].cell(row=3, column=3).value = self.anchor_name
            self.wb["Summary"].cell(row=4, column=3).value = self.test_name

        self.set_name=set_name

    def run(self):
        #Clear branch directory
            if os.path.exists(self.branch[0]):
                shutil.rmtree(self.branch[0])

        # ======================================
        # Encode and Decode

            encoders=[True,False]
            for isEncoder in encoders:
                thread_pool = multiprocessing.Pool(thread_num_limit[0])
                for class_selecte in class_selected:
                        for rate_point in self.rate_points:


                                condition_selecte = condition_selected[list(condition_selected.keys())[0]]

                                pointCloud = File.get_all_file_from_baseCatalog(".ply",self.PCC_sequence+"/"+class_selecte)
                                cameras_path=os.path.dirname(pointCloud)

                               
                                for tmc in tmc3_selected:
                                    if isEncoder:
                                        tmc13=tmc3_selected[tmc]+"/build/avs-pcc-encoder"
                                    else:
                                        tmc13=tmc3_selected[tmc]+"/build/avs-pcc-decoder"

                                    out=self.branch[0] + "/"+str(tmc)+"/" + condition_selecte + "/" + class_selecte + "/" + rate_point+f"/frame{0:03d}"
                                    #self.sub_run(pointCloud, out, 0, tmc13, tmc, cameras_path, cameraPosition, isEncoder,self.branch)
                                    thread_pool.apply_async(self.sub_run,args=(pointCloud, out, 0, tmc13, tmc, cameras_path, cameraPosition,isEncoder, self.branch))

                thread_pool.close()  # Close the process pool entrance; no new processes will be accepted
                thread_pool.join()  # Main process blocks until all subprocesses in the pool are completed, then resumes

            thread_pool = multiprocessing.Pool(thread_num_limit[1])
            for class_selecte in class_selected:
                    for rate_point in self.rate_points:
                        condition_selecte = condition_selected[list(condition_selected.keys())[0]]

                        for tmc in tmc3_selected:
                                    DIR=f"{self.branch[0]}/{tmc}/{condition_selecte}/{class_selecte}/{rate_point}"
                                    #self.render(0, DIR, mpeg_gsc_metrics, class_selecte)
                                    thread_pool.apply_async(self.render, args=(0, DIR, mpeg_gsc_metrics, class_selecte))

            thread_pool.close()  # Close the process pool entrance; no new processes will be accepted
            thread_pool.join()  # Main process blocks until all subprocesses in the pool are completed, then resumes

            if not save_pointCloud:
                all=File.get_all_file_from_baseCatalog(".ply","./")
                for a in all:
                    os.remove(a)

            self.write_to_excel()      # Write results to Excel


    @staticmethod
    def sub_run(pointCloud,output,frame,tmc13,tmc,cameras_path,cameraPosition,isEncoder,branch_selecte):

        if isEncoder:
            print("Encoding in progress:"+output)
        else:
            print("Decoding in progress:"+output)

        os.makedirs(output, exist_ok=True)

        if isEncoder:
            pre_process(output,pointCloud)  # Pre-processing
            encoder(output,pointCloud,tmc13,tmc,isEncoder,branch_selecte)  # Encoding
        else:
            encoder(output, pointCloud, tmc13, tmc, isEncoder,branch_selecte)  # Decoding
            post_process(output)  # Post-processing

            src_DIR = os.path.dirname(output) + "/src"
            dec_DIR = os.path.dirname(output) + "/dec"
            os.makedirs(src_DIR, exist_ok=True)
            os.makedirs(dec_DIR, exist_ok=True)

            cam_to_ply(pointCloud,cameras_path,cameraPosition,src_DIR+f"/frame{frame:03d}.ply")
            cam_to_ply(output + "/dequantized.ply", cameras_path, cameraPosition, dec_DIR + f"/frame{frame:03d}.ply")

            #shutil.rmtree(output)

            print("Completed:" + output)


    @staticmethod
    def render(frame_id,DIR,exe,class_selecte):

        src=DIR+"/src/"+"frame"+"%03d" + ".ply"
        dec=DIR+"/dec/"+"frame"+"%03d" + ".ply"
        metrics(exe,src,dec,frame_id,1,seq_information[class_selecte][0],seq_information[class_selecte][1])



    def write_to_excel(self):
        # ======================================
        # Write PSNR metrics
        metrics_path=File.get_all_file_from_baseCatalog("__metrics.txt",self.branch[0])
        metrics_data=[]
        for path in metrics_path:
            metrics_data.append(self.extract_metrics(path))

        self.sub_write_to_excel(metrics_data)


        # ======================================
        # Write bitstream information
        # ======================================
        bitstream_files=File.get_all_file_from_baseCatalog("__Bitbream__encoder.txt",self.branch[0])
        for bitstream_file in bitstream_files:

            parsed_data = parse_enc_log(Path(bitstream_file))
            decoder_txt=bitstream_file.replace("encoder.txt","decoder.txt")
            parsed_data1 = parse_dec_log(Path(decoder_txt))
            parsed_data.update(parsed_data1)

            self.sub_bitstream_write_to_excel(parsed_data,bitstream_file)

        self.set_name()


        self.wb.save(self.branch[0] + "/" + self.out_excel.split("/")[-1])
        print(f"Data successfully written to {self.branch[0]}/{self.out_excel}")

# ======================================
# Step 1: Extract data from metrics.txt
# ======================================
    def extract_metrics(self,file_path):
        data=dict()
        PSNR = dict()
        with open(file_path, "r") as f:
            contents = f.readlines()

        rgb=[]
        yuv=[]
        ssim_yuv=[]

        for content in contents:
            if content.find("OM-")>=0:         # Skip OM-PSNR and OM-IVSSIM
                continue
            if content.find("Psnr RGB (avg)")>=0:
                rgb.append(float(content.split()[4]))
            elif content.find("Psnr YUV (avg)")>=0:
                yuv.append(float(content.split()[4]))
            elif content.find("SSIM (avg)")>=0:
                ssim_yuv.append(float(content.split()[3]))

        frame_num=len(rgb)
        PSNR["PSNR-RGB"]=sum(rgb)/frame_num
        PSNR["PSNR-YCbCr"]=sum(yuv)/frame_num
        PSNR["SSIM-YCbCr"]=sum(ssim_yuv)/frame_num
        PSNR["MIN_PSNR-RGB"]=min(rgb)
        PSNR["MIN_PSNR-YUV"] = min(yuv)
        PSNR["MIN_SSIM-YUV"] = min(ssim_yuv)

        PSNR["MAX_PSNR-RGB"] = max(rgb)
        PSNR["MAX_PSNR-YUV"] = max(yuv)
        PSNR["MAX_SSIM-YUV"] = max(ssim_yuv)

        data[file_path]=PSNR
        return data


# ======================================
# Step 2: Write metrics to Excel .xlsm file
# ======================================
    def sub_write_to_excel(self,metrics_data):

        # Write data in frame number order
        for path in metrics_data:
            key=list(path.keys())[0]
            labels=key.split("/")
            increase_row=int(labels[-3][1:3])-1
            tmc=int(labels[-6])
            increase_columns = 0 if tmc else 20 
            _class = labels[-4]

            row = seq_information[_class][5] +increase_row# Assume data is ordered sequentially
            data = path[key]
            ws=self.wb["C1"]
            # Write metric data (retain original precision)
            for key in PSNR_columns.keys():
                    ws.cell(row=row, column=PSNR_columns[key]+increase_columns, value=data[key])



# ======================================
# Step 3: Write bitstream information to Excel
# ======================================
    def sub_bitstream_write_to_excel(self,data,bitstream_file):
            # Column mapping
            columns = Bitstream_columns
            # Define starting position of data columns (adjust according to your Excel template)
            # Example: Assume header is in row 1, data starts from row 2
            labels = bitstream_file.split("/")
            increase_row = int(labels[-3][1:3]) - 1
            _class=labels[-4]
            row = seq_information[_class][5] + increase_row  # Assume data is ordered sequentially
            tmc=int(labels[-6])
            increase_columns = 0 if tmc else 20 
            ws = self.wb["C1"]

            for key in data.keys():
                ws.cell(row=row, column=Bitstream_columns[key]+increase_columns, value=data[key][0])



if __name__ == '__main__':
    p = "./1F-geo/"
    g = Gaussian()
    g.run()
    #g.write_to_excel()   # g.run() will automatically collect results and write to Excel



