# 3dgs_pcrm
This script is a testing tool designed for PCRM. It automates the end-to-end workflow of 3D point cloud quantization, encoding, decoding, rendering, and objective metric evaluation, with support for parallel processing to improve testing efficiency. Test results (e.g., PSNR, SSIM, bitrate, time) are automatically summarized into an Excel file for easy comparison and analysis.
Project File Structure
The project's root directory contains the following key files and folders:

# Project File Structure
```
The project's root directory contains the following key files and folders:
├── cfg0/                     # **Anchor (baseline) test configuration directory**
├── cfg1/                     # **Test (target) configuration directory**
├── submodules/               # Project-dependent submodules
├── template/                 # Template file directory
├── collect_results.py        # Script for collecting and summarizing results
├── my_tools.py               # Custom utility function
├── processing.py             # Main processing workflow script
├── README.md                 # This documentation
├── requirements.txt          # Python dependency list
└── run.py                    # **Main execution script containing all core configuration parameters**
```

## Core Configuration Parameters
Core configuration parameters can be adjusted in run.py.
| Parameter Name|Description  |
|--|--|
|tmc3_selected | Path to the AVS-PCC codecs to be tested |
| cameraPosition|Path to the tool for converting point clouds to camera-view PLY files  |
|mpeg_gsc_metrics | Path to the mpeg-gsc-metrics tool for point cloud rendering and objective quality metric calculation |
| template_excel| Path to the macro-enabled Excel template for result output  |
|output_excel |Name of the final Excel file for storing aggregated test results  |
|thread_num_limit | Parallel process count configuration:<br>- Index 0: Number of processes for encoding/decoding <br>- Index 1: Number of processes for metric executables. NB: Each executable renders 20 images in parallel, so the total number of processes is 20 times the number of executables.  |
|save_iamge | Toggle for saving rendered images during metric calculation:<br>- 1 = Save<br>- 0 = Do not save (save disk space) |
|save_pointCloud | Toggle for saving intermediate point cloud files (quantized/reconstructed PLY):<br>- 1 = Save<br>- 0 = Delete after test (save disk space) |
| computeSsim| Toggle for SSIM calculation:<br>- 1 = Enable<br>- 0 = Disable |
|computeIvssim | Toggle for IvSSIM calculation:<br>- 1 = Enable<br>- 0 = Disable |
| cpu| Hardware selection:<br>- 1 = Use CPU<br>- 0 = Use GPU |
| branch_selected| Test branch Single <br>Options: cfg_predict, cfg_transform |
|condition_selected |Test condition Single    |
|class_selected |Point cloud categories to be tested <br>Default includes: alley, bartender, bicycle, cinema, garden, photo, rocket, toy  |
|PCC_sequence | Path of input point cloud data |


## Data Preparation
The source data is reconstructed using COLMAP and the INRIA 3DGS code.You need to modify the source path configuration in the script according to the actual storage location of the data.The data structure is as follows:

```
/data/Sequence/AVS_data/src
├── bartender
│   ├── cameras.txt
│   ├── frame000.ply
│   └── images.txt
├── breakfast
│   ├── cameras.txt
│   ├── frame000.ply
│   └── images.txt
├── cinema
│   ├── cameras.txt
│   ├── frame000.ply
│   └── images.txt
└── fruit
    ├── 0081.ply
    ├── cameras.bin
    └── images.bin
```
## Environmental Setups
### install dependencies
Follow the instructions to install dependencies of avs-pcc-pcrm, mpeg-gsc-tools, mpeg-gsc-metrics.

### virtual env
```bash
conda create -n gscem python==3.9
conda activate gscem
```

### install the package
```bash
pip install -r requirements.txt
```
## run experiment
```bash
python run.py
```

