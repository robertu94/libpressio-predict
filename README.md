# LibPressio-Predict

A framework for predicting compression ratios for lossy compressors and other key metrics in a generic way using LibPressio

## Getting Started

### Benchmarking prediction schemes on your dataset

You can use the tool `pressio_predict_bench` to evaluate your prediction method on a dataset of your choice.  Here is an example of running the tool the 13 fields of the Hurricane dataset from SDRBench with the method `average_sampled`:

```sh
./build/pressio_predict_bench  \
    -a dataset \
    -a settings \
    -a run \
    -a score \
    -L pressio:loader=folder \
    -l folder:base_dir=$HOME/git/datasets/hurricane/100x500x500/ \
    -l io_loader:dims=500 \
    -l io_loader:dims=500 \
    -l io_loader:dims=100 \
    -l io_loader:dtype=float \
    -l io_loader:use_template=true \
    -l folder:regex='.+/([A-Z]+)f(\d+).bin.f32' \
    -l folder:groups=field \
    -l folder:groups=timestep \
    -b pressio:compressor=sz3 \
    -b pressio:metric=composite \
    -b composite:plugins=size \
    -b composite:plugins=time \
    -b composite:plugins=error_stat \
    -o pressio:abs=1e-5
```

It will compute the median absolute percentage error.  If built with MPI support, this command will run in parallel on a cluster.  If your metrics are subject to interference, run in isolation mode with the `-Z isolate` flag.

### Using a prediction scheme

You can use the prediction schemes from C/C++ using the following API pattern

```
TODO EXAMPLE
```

### Adding a new prediction scheme

1. Build the code, then run the provided tool `pressio_new metric $name >
   ./src/plugins/predictors/$name.cc` from LibPressioTools to add a new
   predictor from a metrics template.


2. After the code is generated, edit the resulting file in
   `src/plugins/predictors/$name.cc` to add your prediction method.  You likely
   just need to edit the `begin_compress_impl` function to gather the metrics
   you need. You can see the example in `src/plugins/predictors/tiled_samples.cc`
   for an example that invokes the compressor to produces an estimate.

3. Next in `get_configuration` add an entry `predictors:invalidate` with a `std::vector<std::string>`
   containing the list of options to a compressor that would invalidate this predictor calculation.

   + a special value of `predictors:nondeterministic` indicates that this metric is
     is non-deterministic (e.g. Randomized SVD) even if the underlying compressor is.
   + a special value of `predictors:runtime` indicates that this metric is
     is dependent on performance related characteristics (e.g. time:compress)
   + a special value of `predictors:error_dependent` indicates that this metric is
     invalidated whenever whenever any setting that effects the quality of the
     data after compression is changed (e.g. the quantized entropy)
   + a special value of `predictors:error_agnostic` indicates that this metric is
     independent of the configuration of any compressor (e.g. the standard deviation
     of the input data)
   + if any other compressor setting (e.g. `pressio:abs` or `sz:quantization_intervals`)
     is provided in this list, the compressor for which predictions are being
     made MUST implement this metric for this predictor to use this predictor.
     If the compressor does not implement the metric MAY return an
     appropriate error.

4. After you have the predictors used to produce the estimates, run `pressio_new estimator`
   to generate a estimator class from a template that combines these estimates
   into an estimation of the metric of interest.

```
TODO EXAMPLE
```

2. Edit the CMakeLists.txt code to add dependencies for your new predictor/estimator.

3. Build your new predictor and/or estimator modules, and ensure that the automated unit tests pass.


## Installation

`libpressio-predict` is best installed via spack.


```sh
git clone https://github.com/spack/spack
git clone https://github.com/robertu94/spack_packages robertu94_packages
source ./spack/share/spack/setup-env.sh
spack repo add ./robertu94_packages

spack install libpressio-predict+bin
```

## Development Builds

The easiest way to do a development build of libpressio is to use Spack envionments.

```bash
# one time setup: create an envionment
spack env create -d mydevenviroment
spack env activate mydevenvionment

# one time setup: tell spack to set LD_LIBRARY_PATH with the spack envionment's library paths
spack config add modules:prefix_inspections:lib64:[LD_LIBRARY_PATH]
spack config add modules:prefix_inspections:lib:[LD_LIBRARY_PATH]

# one time setup: install libpressio-tools and checkout 
# libpressio for development
spack add libpressio-predict+bin
spack develop libpressio-predict@git.master

# compile and install (repeat as needed)
spack install 
```


## Manual Installation

Libpressio-Predict unconditionally requires:

+ `cmake`
+ `pkg-config`
+ `libpressio`

Dependency versions and optional dependencies are documented [in the spack package](https://github.com/spack/spack/blob/develop/var/spack/repos/builtin/packages/libpressio-predict/package.py).

# API Stability

Please refer to [docs/stability.md](docs/stability.md).

# How to Contribute

Please refer to [CONTRIBUTORS.md](CONTRIBUTORS.md) for a list of contributors, sponsors, and contribution guidelines.

# Bug Reports

Please files bugs to the Github Issues page on the robertu94 libpressio-predict repository.

Please read this post on [how to file a good bug report](https://codingnest.com/how-to-file-a-good-bug-report/).  After reading this post, please provide the following information specific to libpressio-predict:

+ Your OS version and distribution information, usually this can be found in `/etc/os-release`
+ the output of `cmake -L $BUILD_DIR`
+ the version of each of libpressio-predicts's dependencies listed in the README that you have installed. Where possible, please provide the commit hashes.

# Citing LibPressioPredict

We hope to publish a paper on LibPressioPredict soon.  Until then please cite LibPressio and the underlying prediction method which should have a citation in the `pressio:description`

```
@inproceedings{underwood2021productive,
  title={Productive and Performant Generic Lossy Data Compression with LibPressio},
  author={Underwood, Robert and Malvoso, Victoriana and Calhoun, Jon C and Di, Sheng and Cappello, Franck},
  booktitle={2021 7th International Workshop on Data Analysis and Reduction for Big Scientific Data (DRBSD-7)},
  pages={1--10},
  year={2021},
  organization={IEEE}
}
```

# Reproducability

We've achieved reproducabilty materials for our prior efforts in the reproduceability folder by the first author name and year.
