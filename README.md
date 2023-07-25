# LibPressio-Predict

A framework for predicting compression ratios for lossy compressors and other key metrics in a generic way using LibPressio

## Getting Started

### Benchmarking prediction schemes on your dataset

You can use the tool `pressio_predict_bench` to evaluate your prediction method on a dataset of your choice.  Here is an example of running the tool on 2d slices of the `CLOUDf48.bin` for the Hurricane dataset from SDRBench with the method `average_sampled`:

```
pressio_predict_bench
```

It will compute the median absolute percentage error.  If built with MPI support, this command will run in parallel on a cluster.  If your metrics are subject to interference, run in isolation mode with the `-Z isolate` flag.

### Using a prediction scheme

You can use the prediction schemes from C/C++ using the following API pattern

```
TODO EXAMPLE
```

### Adding a new prediction scheme

Build the code, then run the provided tool `pressio_predict_new $name` to add a new prediction scheme from a template.

After the template is generated: 

1. edit the resulting file in `src/plugins/predictors/$name to add your prediction method.  You will need to modify the `metrics` and `predict` functions to add both implementations of the methods and how to combine them into a prediction of the compression ratio, and a list of what invalidates them to `get_configuration`.  First the `metrics()` function:

```
TODO EXAMPLE
```

Now the `predict()` function:

```
TODO EXAMPLE
```

2. If you need dependencies other than LibPressio, edit the CMakeLists.txt code to add dependencies for your new predictor.

3. Build your new predictor module, and ensure that the automated unit tests pass.


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
