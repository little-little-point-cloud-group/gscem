python3 sweep_qp_runner.py \
    --version finalv2_test \
    --quant_cfg_path ./cfg/cfg_quant_v2.0.yaml \
    --codec_cfg_dir ./cfg/cfg_predict_multiple \
    --metrics_cfg_path ./cfg/cfg_metrics.yaml \
    --data_dir /data/gly \
    --output_dir ./results \
    --template_path ./GSCRM_template_v1.1.xlsm \
    --base_attr_dir ./cfg \
    --channels  opacity\
    --deltas 0\
    --rates r1 r2 r3 r4 r5 
#channels指定要修改qp的通道，通道名称如下
# "sh0_y": [0],
# "sh1_y": [3, 6, 9],
# "sh2_y": [12, 15, 18, 21, 24],
# "sh3_y": [27, 30, 33, 36, 39, 42, 45],
# # Cb/Cr 同时修改，sh0_cb和sh0_cr等会映射到sh0_cbcr
# "sh0_cbcr": [1, 2],
# "sh1_cbcr": [4, 5, 7, 8, 10, 11],
# "sh2_cbcr": [13, 14, 16, 17, 19, 20, 22, 23, 25, 26],
# "sh3_cbcr": [28, 29, 31, 32, 34, 35, 37, 38, 40, 41, 43, 44, 46, 47],
# "opacity": [48],
# "scale": [49, 50, 51],
# "rotation": [52, 53, 54, 55],
#rates表示修改哪些码率点的qp
#deltas指定在基础qp上的偏移
#例如，--channels sh0_y sh1_y --deltas -2 2 --rates r1 r2 r3 r4 r5 
#表示在r1 r2 r3 r4 r5码率点上对sh0_y sh1_y通道的基准qp分别-2 +2，共四组：其他不变，sh0_y qp -2，sh0_y qp +2，sh1_y qp -2.sh1_y qp +2
