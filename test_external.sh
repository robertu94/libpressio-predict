#!/usr/bin/env bash

time pressio \
  -O all -M all -m time -m external \
  -i ~/git/datasets/hurricane/100x500x500/CLOUDf48.bin.f32  -d 500 -d 500 -d 100 -t float \
  -n external:command="julia --project=/home/runderwood/git/libraries/julia/CodarNorms.jl /home/runderwood/git/libraries/julia/CodarNorms.jl/scripts/pressio.jl  --external_metric gok_ssim" \
  -o sz:error_bound_mode_str="abs" \
  -o sz:abs_err_bound=1e-3 \
  sz
