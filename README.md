


# prepare src data
src data are reconstructed from colmap and INRIA 3dgs code. 
download link: https://vepcos-1257411467.cos.ap-guangzhou.myqcloud.com/gsc/src.zip
The data structure is as follows:

```
./data/src
├── alley
│   ├── cameras.bin
│   ├── frame000.ply
│   └── images.bin
├── .

```

# install dependencies
Follow the instructions to install dependencies of avs-pcc-pcrm, mpeg-gsc-tools, mpeg-gsc-metrics.
Note: If these packages are not installed in ./submodules, the paths of binaries have to be modified correspondingly.


# install gscem package

## virtual env
```
conda create -n gscem python==3.10
conda activate gscem
```
## install the package
```
pip install -r requirements.txt
```


# run experiment
1. generate cfg for PCRM
```
cd cfg
python gen_cfg.py
```
2. path related
modify the path to data, cfg, and excel template in launch_tests.sh

```
./runner.sh \
    --version v2.0_predtrans             \
    --quant_cfg_path ./cfg/cfg_quant_v2.0.yaml      \
    --codec_cfg_dir ./cfg/cfg_predtrans      \
    --metrics_cfg_path ./cfg/cfg_metrics.yaml     \
    --data_dir /data1/openi/data            \
    --output_dir /data1/openi/data           \
    --template_path /data1/openi/data/experiments/GSCRM_template_v1.0.xlsm
```
3. configure the number of parallel threads (the last number in each line) in runner.sh
current number is for machine with 64GB RAM, 24GB GPU memory

```
run python run_parallel.py "$scripts_dir/0_pre.sh" 12
run python run_parallel.py "$scripts_dir/1_enc.sh" 20
run python run_parallel.py "$scripts_dir/2_dec.sh" 20
run python run_parallel.py "$scripts_dir/3_deq.sh" 20
run python run_parallel.py "$scripts_dir/4_cam.sh" 20
run python run_parallel.py "$scripts_dir/5_metric.sh" 20
```
4. launch tests
```
bash launch_tests.sh
```