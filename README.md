


# prepare src data
src data are reconstructed from colmap and INRIA 3dgs code. 
A bash script is available (./scripts/prep_data.sh) to copy files into this project. The source paths should be modified based on where the data is stored. 
The data structure is as follows:

```
./data/src
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

# install dependencies
Follow the instructions to install dependencies of avs-pcc-pcrm, mpeg-gsc-tools, mpeg-gsc-metrics.


# install gscem package

## virtual env
```
conda create -n gscem python==3.9
conda activate gscem
```
## install the package
```
pip install -r requirements.txt
```

# run experiment

```
bash launch.sh
```
# qp
The current baseline QP value is


Rate point	SH0_Y(DC)	SH0_Cb_Cr(DC)	SH1_Y	SH2_Y	SH3_Y	SH1_Cb_Cr	SH2_Cb_Cr	SH3_Cb_Cr	Opacity	Scale	Rotation
R5	               6	           12	   14	   13	   21	       13	        8	       18	     24	   12	      12
R4	               8	           15	   18	   17	   25	       17	       12	       22	     28	   16	      16
R3	              10	           18	   22	   21	   29	       21	       16	       26	     32	   20	      20
R2	              18	           26	   30	   29	   37	       29	       24	       34	     40	   28	      28
R1	              26	           34	   38	   37	   45	       37	       32	       42	     48	   36	      36


1.To modify the baseline QP value, first run python3 gen_cfg_multil.py to generate cfg/cfg_predict_multiple, then run the following command.
2.Enable RGB-to-YUV conversion in gscem_all/cfg/cfg_quant/cfg_quant_v0.1.json (already enabled).
3.The command to run is as follows:
python3 scripts/sweep_qp.py \
  --base_cfg_dir ./cfg/cfg_predict_multiple \
  --base_quant_cfg ./cfg/cfg_quant/cfg_quant_v0.1.json \
  --runner_dir scripts \
  --data_dir ./data/src \
  --results_root ../gscem_all/results \
  --excel_template ./template/template.xlsm \
  --version_prefix run \
  --rates r1 r2 r3 r4 r5 \
  --channels sh1_y\
  --deltas 0 

  The following are optional parameters:
  --keep_cfg \
  --keep_artifacts  
  
"""