#!/usr/bin/env bash

pressio \
  -i ~/git/datasets/hurricane/100x500x500/CLOUDf48.bin.f32 \
  -d 500 -d 500 -d 100 -t float \
  -m time -m size -M all\
  -b opt:compressor=zfp \
  -b opt:search=fraz \
  -b zfp:metric=time \
  -o opt:inputs="zfp:omp_threads" \
  -o opt:inputs="zfp:omp_chunk_size" \
  -o opt:output="time:compress_many" \
  -o opt:is_integral=1 \
  -o opt:lower_bound=1 \
  -o opt:upper_bound=13 \
  -o opt:is_integral=1 \
  -o opt:lower_bound=0 \
  -o opt:upper_bound=512 \
  -o opt:objective_mode_name=min \
  -o opt:do_decompress=0 \
  -o zfp:accuracy=1e-6 \
  -o zfp:execution_name=omp \
  -o opt:max_iterations=30 \
  opt
