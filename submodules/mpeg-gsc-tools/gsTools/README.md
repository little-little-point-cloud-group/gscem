# GsTools - Gaussian Splat Tools

Various tools created and used MPG WG4/WG7 for 3D Gaussian splat projects:
  - addCameraPosition: add camera information in 3DGS ply files 

# Build 


```console 
mkdir build
cd build
cmake ..
cmake --build . --config Release --parallel 20
```

or 

```console 
./build.sh 
```


# AddCameraPosition

This application loads a 3DGS object stored in a Ply file and the camera information stored in:
- cameras.txt/.bin
- images.txt/.bin
- JSON file
and adds the camera information to the header of the Ply files. This updated file can be used by mpeg-3d-renderer or mpeg-gsc-metrics software.

## Usage 

```console 
$ ./build/msvc/Release/bin/Release/cameraPosition.exe 
input parameters must be:
  -v,   --verbose=0  Verbose
  -i,   --input=""   Path to input point cloud
        --camera=""  Path to camera information:
                      - cameras.txt
                      - cameras.bin
        --image=""   Path to image information:
                      - images.txt
                      - images.bin
        --json=""    Path to json file
  -o,   --output=""  Path to output point cloud
        --ascii=0    Create ascii point cloud
```


## Examples

### Text files

```console 
./build/msvc/Release/bin/Release/cameraPosition.exe \
   --input=./bartender_stable_m71763/track/frame000.ply \
   --camera=./bartender_stable_m71763/colmap_data/frame000/sparse/cameras.txt \
   --image=./bartender_stable_m71763/colmap_data/frame000/sparse/images.txt \
   --output=frame000_pos.ply
   -v 1
```

### Bin files

```console 
./build/msvc/Release/bin/Release/cameraPosition.exe \
   --input=./choreo_dark/choreo_dark_0000.ply  \
   --camera=./choreo_dark/frame0000/colmap_data/sparse/0/cameras.bin \
   --image=./choreo_dark/frame0000/colmap_data/sparse/0/images.bin \
   --output=choreo_dark_0000_pos.ply \
   -v 1
```

### JSON file

```console 
./build/msvc/Release/bin/Release/cameraPosition.exe \
   --input=./models/bicycle.ply \
   --json=./models/bicycle/cameras.json \
   --output=bicycle_pos.ply \
   -v 1
```

### For sequences

```console 
for((i=0;i<32;i++)) ; \
do \
   ./build/msvc/Release/bin/Release/cameraPosition.exe   \
      --input=./bartender_stable_m71763/track/frame( printf "%03d" $i ).ply \
      --camera=./bartender_stable_m71763/colmap_data/frame( printf "%03d" $i )/sparse/cameras.txt \
      --image=./bartender_stable_m71763/colmap_data/frame( printf "%03d" $i )/sparse/images.txt \
      --output=frame$( printf "%03d" $i )_pos.ply; \
done
```

# Licence

The copyright in this software is being made available under the BSD
Licence, included below.  This software may be subject to other third
party and contributor rights, including patent rights, and no such
rights are granted under this licence.

Copyright (c) 2025, ISO/IEC
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

* Neither the name of the ISO/IEC nor the names of its contributors
  may be used to endorse or promote products derived from this
  software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.


# Contacts

- Julien Ricard: jricard@global.tencent.com
- Gilles Teniou: teniou@global.tencent.com