#!/usr/bin/env bash

gdb --args pressio \
  -i ~/git/datasets/hurricane/100x500x500/CLOUD_slice.f32 -t float -d 500 -d 100 -d 5 \
  -W /tmp/CLOUD_slice.f32.dec \
  -m error_stat -M all -O all \
  -o mgard:s=1 \
  -o mgard:tolerance=60 \
  -o mgard:qoi_use_metric=1 \
  -o mgard:qoi_metric_name="error_stat:psnr" \
  mgard

# gdb --args pressio \
#   -i ~/git/datasets/hurricane/100x500x500/CLOUD_slice.f32 -t float -d 500 -d 500 \
#   -m error_stat -M all -O all \
#   -o mgard:s=1 \
#   -o mgard:tolerance=60 \
#   -o mgard:qoi_use_metric=1 \
#   -o mgard:qoi_metric_name="error_stat:psnr" \
#   mgard
