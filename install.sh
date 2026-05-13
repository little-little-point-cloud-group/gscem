#!/usr/bin/env bash
set -e  # stop on error

echo "==== Building avs-pcc-pcrm ===="

cd ./submodules/avs-pcc-pcrm
rm -rf build && mkdir -p build
cd build
cmake ../source -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5
make -j8

echo "==== Building mpeg-gsc-metrics ===="

cd ../../mpeg-gsc-metrics
rm -rf build && mkdir -p build
cd build
cmake .. -DCMAKE_CXX_COMPILER=g++-11
cmake --build . --config Release --parallel 20

echo "==== Building mpeg-gsc-tools/gsTools ===="

cd ../../mpeg-gsc-tools/gsTools
rm -rf build && mkdir -p build
cd build
cmake .. -DCMAKE_CXX_COMPILER=g++-11
cmake --build . --config Release --parallel 20

echo "==== All builds completed successfully! ===="
