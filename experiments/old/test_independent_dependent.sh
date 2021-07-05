#!/usr/bin/env bash


read -d '' script <<EOF
local cr = metrics['/pressio/many_dependent/opt/sz/composite/size:size:compression_ratio'];
local psnr = metrics['/pressio/many_dependent/opt/sz/composite/error_stat:error_stat:psnr'];
local threshold = 60.0;
local objective = 0;
if psnr ~= nil and psnr < threshold then
  objective = 0;
else
  objective = cr;
end
return "objective", objective
EOF

mpiexec -np 7 --oversubscribe pressio \
  -O all \
  -Q  \
  -t float -d 500 -d 500 -d 100 -i ${HOME}/git/datasets/hurricane/100x500x500/PRECIPf48.bin.f32 \
	-p -t float -d 500 -d 500 -d 100 -i ${HOME}/git/datasets/hurricane/100x500x500/PRECIPf48.bin.f32 \
	-p -t float -d 500 -d 500 -d 100 -i ${HOME}/git/datasets/hurricane/100x500x500/PRECIPf48.bin.f32 \
	-p -t float -d 500 -d 500 -d 100 -i ${HOME}/git/datasets/hurricane/100x500x500/CLOUDf48.bin.f32 \
	-p -t float -d 500 -d 500 -d 100 -i ${HOME}/git/datasets/hurricane/100x500x500/CLOUDf48.bin.f32 \
	-p -t float -d 500 -d 500 -d 100 -i ${HOME}/git/datasets/hurricane/100x500x500/CLOUDf48.bin.f32 \
  -m time -m size -M all \
  -b /pressio:many_independent:compressor=many_dependent \
  -b /pressio/many_dependent:many_dependent:compressor=opt \
  -b /pressio/many_dependent/opt:opt:compressor=sz \
  -b /pressio/many_dependent/opt:opt:metric=composite \
  -b /pressio/many_dependent/opt/composite:composite:names=time \
  -b /pressio/many_dependent/opt/composite:composite:names=size \
  -b /pressio/many_dependent/opt/composite:composite:names=error_stat \
  -b /pressio/many_dependent/opt/composite:composite:plugins=time \
  -b /pressio/many_dependent/opt/composite:composite:plugins=size \
  -b /pressio/many_dependent/opt/composite:composite:plugins=error_stat \
  -b /pressio/many_dependent/opt:opt:search=fraz \
  -o /pressio/many_dependent/opt:opt:inputs='/pressio/many_dependent/opt/sz:sz:abs_err_bound' \
  -o /pressio/many_dependent/opt:opt:output='/pressio/many_dependent/opt/sz/composite:composite:objective' \
  -o /pressio/many_dependent/opt:opt:output='/pressio/many_dependent/opt/sz/composite/size:size:compression_ratio' \
  -o /pressio/many_dependent/opt:opt:output='/pressio/many_dependent/opt/sz/composite/error_stat:error_stat:psnr' \
  -o /pressio/many_dependent/opt/sz:sz:error_bound_mode_str='abs' \
  -o /pressio/many_dependent/opt:opt:objective_mode_name='max' \
  -o /pressio/many_dependent/opt/fraz:opt:lower_bound=1e-12 \
  -o /pressio/many_dependent/opt/fraz:opt:upper_bound=3e-1 \
  -o /pressio/many_dependent/opt/composite:composite:scripts="$script" \
  -o /pressio/many_dependent/opt/fraz:opt:target=100 \
  -o /pressio:subgroups:input_data_groups=0 \
  -o /pressio:subgroups:input_data_groups=0 \
  -o /pressio:subgroups:input_data_groups=0 \
  -o /pressio:subgroups:input_data_groups=1 \
  -o /pressio:subgroups:input_data_groups=1 \
  -o /pressio:subgroups:input_data_groups=1 \
  -o /pressio:subgroups:output_data_groups=0 \
  -o /pressio:subgroups:output_data_groups=0 \
  -o /pressio:subgroups:output_data_groups=0 \
  -o /pressio:subgroups:output_data_groups=1 \
  -o /pressio:subgroups:output_data_groups=1 \
  -o /pressio:subgroups:output_data_groups=1 \
  -o /pressio:distributed:n_worker_groups=2 \
  -o /pressio/many_dependent:distributed:n_worker_groups=2 \
  many_independent
