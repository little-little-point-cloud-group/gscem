#!/usr/bin/env bash
#  ------------------------------------------------------------
#  prep_data.sh
#
#  Brief description:  prepare data (ply, colmap related files)
#  Author:  Haiqiang Wang
#  Version: 1.0
#  Licence: Tencent
#  ------------------------------------------------------------
set -euo pipefail

downlaod_dir='/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly'
src_dir='./src'

mkdir -p $src_dir/bartender
cp $downlaod_dir/m71763_bartender_stable/track/frame000.ply $src_dir/bartender/frame000.ply
cp $downlaod_dir/m71763_bartender_stable/colmap_data/frame000/sparse/0/cameras.txt $src_dir/bartender/cameras.txt
cp $downlaod_dir/m71763_bartender_stable/colmap_data/frame000/sparse/0/images.txt $src_dir/bartender/images.txt

mkdir -p $src_dir/cinema
cp $downlaod_dir/m71763_cinema_stable/track/frame000.ply $src_dir/cinema/frame000.ply
cp $downlaod_dir/m71763_cinema_stable/colmap_data/frame000/sparse/0/cameras.txt $src_dir/cinema/cameras.txt
cp $downlaod_dir/m71763_cinema_stable/colmap_data/frame000/sparse/0/images.txt $src_dir/cinema/images.txt

mkdir -p $src_dir/breakfast
cp $downlaod_dir/m71763_breakfast_stable/track/frame000.ply $src_dir/breakfast/frame000.ply
cp $downlaod_dir/m71763_breakfast_stable/colmap_data/frame000/sparse/0/cameras.txt $src_dir/breakfast/cameras.txt
cp $downlaod_dir/m71763_breakfast_stable/colmap_data/frame000/sparse/0/images.txt $src_dir/breakfast/images.txt

mkdir -p $src_dir/fruit
cp $downlaod_dir/m71903_bust_dataset/trained_models/fruit/0081.ply $src_dir/fruit/0081.ply
cp $downlaod_dir/m71903_bust_dataset/colmap_data/fruit/000081/sparse/0/cameras.bin $src_dir/fruit/cameras.bin
cp $downlaod_dir/m71903_bust_dataset/colmap_data/fruit/000081/sparse/0/images.bin $src_dir/fruit/images.bin

mkdir -p $src_dir/bicycle
cp /data1/git/gaussian-splatting/eval/bicycle/point_cloud/iteration_30000/point_cloud.ply $src_dir/bicycle/bicycle.ply
cp /data1/git/dataset/mipnerf360/bicycle/sparse/0/cameras.bin  $src_dir/bicycle/cameras.bin
cp /data1/git/dataset/mipnerf360/bicycle/sparse/0/images.bin  $src_dir/bicycle/images.bin

mkdir -p $src_dir/garden
cp /data1/git/gaussian-splatting/eval/garden/point_cloud/iteration_30000/point_cloud.ply $src_dir/garden/garden.ply
cp /data1/git/dataset/mipnerf360/garden/sparse/0/cameras.bin  $src_dir/garden/cameras.bin
cp /data1/git/dataset/mipnerf360/garden/sparse/0/images.bin  $src_dir/garden/images.bin