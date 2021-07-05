#!/usr/bin/env bash

gdb --args pressio \
  -t float -d 500 -d 500 -d 100 -i ~/git/datasets/hurricane/100x500x500/CLOUDf48.bin.f32 \
  -m size -m time \
  -M all \
  -o sz:abs_err_bound=1e-6 \
  -o sz:error_bound_mode_str="ABS" \
  sz
  
