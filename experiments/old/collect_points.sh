#!/usr/bin/env bash

read -d '' script <<EOF
local cr = metrics['size:compression_ratio'];
local psnr = metrics['error_stat:psnr'];
local threshold = 60.0;
local objective = 0;
if (psnr == nil or psnr < threshold) then
  objective = 0;
else
  objective = cr;
end
return "objective", objective
EOF


#record a trace of k random points
mpiexec pressio \
  -a settings -a compress -a decompress \
  -t float -d 500 -d 500 -d 100 -i ~/git/datasets/hurricane/100x500x500/CLOUDf48.bin.f32 \
  -m error_stat -m size -m time \
  -M "composite:objective" -M "error_stat:psnr" -M "size:compression_ratio" \
  -b opt:compressor=sz \
  -b opt:search=random_search \
  -b opt:search_metrics=composite_search \
  -b composite_search:search_metrics=progress_printer \
  -b composite_search:search_metrics=record_search \
  -b sz:metric="composite" \
  -b composite:plugins="size" \
  -b composite:plugins="time" \
  -b composite:plugins="error_stat" \
  -o composite:scripts="$script" \
  -o io:path=/tmp/validation.csv \
  -o csv:headers="sz:quantization_intervals" \
  -o csv:headers="sz:rel_err_bound" \
  -o csv:headers="composite:objective" \
  -o csv:headers="error_stat:psnr" \
  -o csv:headers="size:compression_ratio" \
  -o opt:inputs="sz:quantization_intervals" \
  -o opt:inputs="sz:rel_err_bound" \
  -o opt:output="composite:objective" \
  -o opt:output="error_stat:psnr" \
  -o opt:output="size:compression_ratio" \
  -o opt:lower_bound=1 \
  -o opt:lower_bound=0 \
  -o opt:upper_bound=65536 \
  -o opt:upper_bound=.01 \
  -o opt:max_iterations=10 \
  -o opt:objective_mode_name="none" \
  -o sz:error_bound_mode_str="rel" \
  -O all \
  opt

#record a grid of 100x100 points
mpiexec pressio \
  -a compress  \
  -t float -d 500 -d 500 -d 100 -i ~/git/datasets/hurricane/100x500x500/CLOUDf48.bin.f32 \
  -m error_stat -m size -m time \
  -M "composite:objective" -M "error_stat:psnr" -M "size:compression_ratio" \
  -b opt:compressor=sz \
  -b opt:search=dist_gridsearch \
  -b opt:search_metrics=composite_search \
  -b composite_search:search_metrics=progress_printer \
  -b composite_search:search_metrics=record_search \
  -b dist_gridsearch:search=guess_midpoint \
  -b sz:metric=composite \
  -o composite:scripts="$script" \
  -o io:path=/tmp/training.csv \
  -o csv:headers="sz:quantization_intervals" \
  -o csv:headers="sz:rel_err_bound" \
  -o csv:headers="composite:objective" \
  -o csv:headers="error_stat:psnr" \
  -o csv:headers="size:compression_ratio" \
  -o opt:inputs="sz:quantization_intervals" \
  -o opt:inputs="sz:rel_err_bound" \
  -o opt:output="composite:objective" \
  -o opt:output="error_stat:psnr" \
  -o opt:output="size:compression_ratio" \
  -o dist_gridsearch:num_bins=100 \
  -o dist_gridsearch:num_bins=100 \
  -o dist_gridsearch:overlap_percentage=0 \
  -o dist_gridsearch:overlap_percentage=0 \
  -o opt:lower_bound=1 \
  -o opt:lower_bound=0 \
  -o opt:upper_bound=65536 \
  -o opt:upper_bound=.01 \
  -o opt:objective_mode_name="none" \
  -o sz:error_bound_mode_str="rel" \
  -O all \
  opt


