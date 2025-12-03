# AVS-PCC


## Build
**cmake** <http://cmake.org/> is required to build the project across platforms.

### create build directory
- mkdir build
- cd build

### cmake

#### make file
- cmake ../source -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5

#### VS2019
- cmake ../source -G "Visual Studio 16 2019" -A x64

#### VS2017
- cmake ../source -G "Visual Studio 15 2017 Win64"

#### VS2015
- cmake ../source -G "Visual Studio 14 2015 Win64"

#### VS2013
- cmake ../source -G "Visual Studio 12 2013 Win64"

### make

#### Linux
- make

#### Windows
- open the generated visual studio solution and build it

## Run

### Windows 
To run encoder, the following command can be used: 

- avs-pcc-encoder.exe -c ../cfg/.../encode.cfg (-option)

To run decoder in Windows, the following commmand can be used:

- avs-pcc-decoder.exe -c ../cfg/.../decode.cfg (-option)

Documentation of options is provided via the `--help` command line option

### Linux

To run encoder, the following command can be used: 

- avs-pcc-encoder -c ../cfg/.../encode.cfg (-option)

To run decoder in Windows, the following commmand can be used:

- avs-pcc-decoder -c ../cfg/.../decode.cfg (-option)

Documentation of options is provided via the `--help` command line option

## sample configuration file 

### Comfiguration files
All command line parameters may be specified in a configuration file. 
A set of configuration file templates compliant with the current Common Test Conditions
 is provided in the cfg/directory.  

To generate the configuration files, run the `gen_cfg.py` script:
```console
avs-pcc-pcem/cfg/script$ python ./gen_cfg.py
```

## MultiAttribute coding function
If the user wants to enable multiAttribute coding, then can be in the code\source\lib\common\TComVector.h, set the MULTI_ATTRIBUTE is 1,
then will use the multiAttribute coding function. Otherwise, the MULTI_ATTRIBUTE default is zero.


### encoder configuration 
```console
geom_only_flag         　: 0       # geometry-only coding mode  
ascii_write_flag        : 1       # recon ply write mode. 1: ascii, 0: binary  
geom_quant_step         : 1       # geometry quantization step  (>0)  
attr_quant_step         : 1       # attribute quantization step (>0, integer)  
color_transform_flag    : 1       # apply color transform method. 1: on, 0: off  
geom_remove_dup_flag    : 1       # manipulating duplicate points. 1: remove, 0: keep  
geom_quant_ste          : 16      # geometry quantization step  
geom_im_qtbt_k          : 0       # max num of OT before implicit QTBT  
geom_im_qtbt_m          : 0       # min size of implicit QTBT  
attr_quant_param        : 48      # attribute quantization Parameter  
metrics_enable          : 1       # calculate metrics flag. 1: on, 0: off  
symmetry                : 1       # if calculate symmetry metrics  
cal_color               : 1       # if calculate color metrics. 1: on, 0 : off  
cal_reflectance         : 1       # if calculate reflectance metrics. 1: on, 0 : off  
duplicate_mode          : 1       # process duplicated points. 1: average 0 : no process  
multineighbour_mode     : 1       # process same distance neighbours. 1: average 0 : no process  
show_hausdorff          : 1       # if show hausdorff and hausdorffPSNR. 1: on, 0: off  
peakvalue               : 2047    # peak value of Geometry PSNR(default = 0)  
```

### decoder configuration
```console
ascii_write_flag        : 1       # recon ply write mode. 1: ascii, 0: binary  
transform_color         : 0       # apply color transfor. 1: yes  2: no  
```
Note that:
1. When PCEM is supposed to encode/decode multi-frames, the file name of input ply for encoder and bitstream file for decoder should be set in the following format  
   input　　　:　　*basename_input* + *start_number* + ".ply"  
   In order to facilitate getting the file number, the last character of the *basename_input* should not be the number among '0'~'9'.  
   Moreover, the only requirement is that the *start_number* should be the first frame number wanted to be encoded or decoded.  

2. The reconstructed files will be named as follow:

　　-1) Single frame  (*frames_to_be_coded* <= 1):

　　　recon　　　　:"xxx.ply"　　　-->　　　"xxx.ply"
　　　bitstream　　:"yyy.bin"　　　-->　　　"yyy.bin"

　　-2) Multi-frames  (*frames_to_be_coded* > 1):

　　　recon　　　　:"xxx.ply"　　　-->　　　"xxx" + "-" + *frame_number* + ".ply"
　　　bitstream　　:"yyy.bin"　　　-->　　　"yyy" + "-" + *frame_number* + ".bin"

　　　where 

　　　*frame_number* = *start_number* + *iteration_index*, 

　　　with 0 <= *iteration_index* < *frames_to_be_coded*

## Example
The following example encodes an decodes frame 0000 of the sequence `Livox_01_all_1mm`, 
making use of the configuration file `cfg/C1-limitlossyG-lossyA-ai/Livox_01_all_in_one_1mm/r1/encoder(decoder).cfg`, 
and storing the intermediate results in the output directory `output/`.

```console
avs-pcc-pcem$ ./avs-pcc-encoder -c cfg/C1-limitlossyG-lossyA-ai/Livox_01_all_in_one_1mm/r1/encoder.cfg \
    -i dataset/Livox_01_all_1mm-0000.ply \
    -b output/Livox_01_all_1mm-0000.bin \
    -r output/C1-limitlossyG-lossyA-ai_r1_Livox_01_all_1mm_enc.ply \
    -ftbc 1 \
    -mdf output/Livox_01_all_1mm.txt \
    > output/C1-limitlossyG-lossyA-ai_r1_Livox_01_all_1mm_enc.log

avs-pcc-pcem$ ./avs-pcc-decoder -c cfg/C1-limitlossyG-lossyA-ai/Livox_01_all_in_one_1mm/r1/decoder.cfg \
    -b output/Livox_01_all_1mm-0000.bin \
    -r output/C1-limitlossyG-lossyA-ai_r1_Livox_01_all_1mm_dec.ply \
    -ftbc 1 \
    -mdf output/Livox_01_all_1mm.txt \
    > output/C1-limitlossyG-lossyA-ai_r1_Livox_01_all_1mm_dec.log

avs-pcc-pcem$ ./avs-pcc-pc_evalue -c cfg/C1-limitlossyG-lossyA-ai/Livox_01_all_in_one_1mm/r1/pcerror.cfg \
    -f1 dataset/Livox_01_all_1mm-0000.ply \
    -f2 output/C1-limitlossyG-lossyA-ai_r1_Livox_01_all_1mm_dec.ply\
    -ftbc 1 \
    > output/C1-limitlossyG-lossyA-ai_r1_Livox_01_all_1mm_pcerror.log
```