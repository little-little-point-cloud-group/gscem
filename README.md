


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
Current baseline QP value:
Rate point	SH0(Y) SH0(CbCr)	SH1Y	SH2Y	SH3Y	SH1(CbCr)	SH2(CbCr) SH3(CbCr)	 Opacity  Scale	  Rotation        												
R5	         4	    8	        18	   14	   22	    22	      12	       18	 	    16	      8   	  8
R4	         6	    12	      22	   22	   29	    26	      20	       25	 	    24	      12	    12
R3	         10	    18	      30	   30	   37	    34	      28	       33	 	    32	      20	    20
R2	         18	    26	      38	   38	   45	    42	      36	       41	 	    40	      28	    28
R1	         26	    34	      46	   46	   53	    50	      44	       49	 	    48	      36	    36



1. To modify QP values, first need to use python3 gen_cfg_multil.py to generate cfg/cfg_predict_multiple, then run the following command.
2. In gscem_all/cfg/cfg_quant/cfg_quant_v0.1.json, the RGB to YUV conversion has been enabled (already turned on).
3. python3 scripts/sweep_qp.py \
  --base_cfg_dir ./cfg/cfg_predict_multiple \
  --base_quant_cfg ./cfg/cfg_quant/cfg_quant_v0.1.json \
  --runner_dir scripts \
  --data_dir ./data/src \
  --results_root ../gscem_all/results \
  --excel_template ./template/template.xlsm \
  --version_prefix qpSweep \
  --rates r1 r2 r3 r4 r5
  
"""