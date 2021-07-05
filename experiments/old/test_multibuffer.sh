#!/usr/bin/env bash

mpiexec -np 4 /usr/bin/pressio \
  -Q \
  -O all \
  -t float -d 500 -d 500 -d 100 -i ${HOME}/git/datasets/hurricane/100x500x500/PRECIPf48.bin.f32 \
	-p -t float -d 500 -d 500 -d 100 -i ${HOME}/git/datasets/hurricane/100x500x500/CLOUDf48.bin.f32 \
	-p -t float -d 500 -d 500 -d 100 -i ${HOME}/git/datasets/hurricane/100x500x500/CLOUDf48.bin.f32 \
	-p -t float -d 500 -d 500 -d 100 -i ${HOME}/git/datasets/hurricane/100x500x500/CLOUDf48.bin.f32 \
	-p -t float -d 500 -d 500 -d 100 -i ${HOME}/git/datasets/hurricane/100x500x500/CLOUDf48.bin.f32 \
  -m printer -m size -m time -M all  \
  -b /pressio:many_dependent:compressor=sz \
  -b /pressio/sz:sz:metric=composite \
  -b /pressio/sz/composite:composite:names=time \
  -b /pressio/sz/composite:composite:names=size \
  -b /pressio/sz/composite:composite:plugins=time \
  -b /pressio/sz/composite:composite:plugins=size \
  -o /pressio/sz:sz:abs_err_bound=1e-6 \
  -o /pressio/sz:sz:error_bound_mode_str='ABS' \
  many_dependent

