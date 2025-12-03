# MPEG-GSC-TOOLS

# Gsplat rendering

The following command line can be used to generate rendering using gsplat :

```
python gs_render_frames.py \
  -m gs/mpeg/bartender_stable_m71763/content.mpeg.expert/colmap_data/frame000/sparse/  \
  -i gs/mpeg/bartender_stable_m71763/content.mpeg.expert/track/frame000.ply \
  -o ./test/ \ 
  -f 1
```
