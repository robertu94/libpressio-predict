
#include "pressio_data.h"
#include "pressio_compressor.h"
#include "pressio_options.h"
#include "libpressio_ext/cpp/metrics.h"
#include "libpressio_ext/cpp/pressio.h"
#include "libpressio_ext/cpp/options.h"
#include "std_compat/memory.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <type_traits>
#include <cmath>
#include <numeric>
#include <stdio.h>
#include <libpressio.h>
#include <libpressio_ext/cpp/libpressio.h>
#include <libpressio_ext/io/posix.h>
#include <zfp.h>
#include <chrono>
#include <cmath>
#include <cfloat>
#include <sys/stat.h>


/*
This is a block sampling algorithm, I used a derivation of an algorithm 
used to interpolate colors in graphics to allow us to get more blocks, i.e.
get closer to the target sample rate. But in principle uniform sampling
is fine too. For ZFP we just want blocks of size 4^n so this conceptually attempts to
sample blocks rather than individual data points which is why the indices in the loops
are the way that they are - the inner most loop is just copying whatever blocks we ended up sampling
to the output buffer for the sampled dataset.
*/
// 4^n block sampling
template<class T>
std::vector<T>
sampling_1d(T *data, std::vector<size_t> dims, size_t &sample_num, std::vector<size_t> &sample_dims, float sample_ratio) {
	size_t nbEle = dims[0];

	// get num elements to sample sample_ratio, floor to nearest multiple of 4
	size_t temp = nbEle * sample_ratio;
	sample_num = temp - (temp % 4);
	sample_dims.push_back(sample_num);

	size_t nbBlocks = sample_num / 4;

	// compute stride for blocks
	size_t totalBlocks = nbEle/4;
	// totalBlocks = (totalBlocks - (totalBlocks%4)) / 4;
	size_t stride_0 = roundf(((float)totalBlocks / (float)nbBlocks)) * 4;

	std::vector<T> sample_data(sample_num, 0);
	size_t idx = 0;
	for(size_t i = 0; i + 4 < nbEle;){

		for(int j = 0; j < 4; j++)
			sample_data[idx++] = data[i+j];

		i += stride_0;

		if(idx == sample_num) break;

	}

	return sample_data;

}

template<class T>
std::vector<T>
sampling_2d(T *data, std::vector<size_t> dims, size_t &sample_num, std::vector<size_t> &sample_dims, float sample_ratio) {
	size_t nbEle = std::accumulate(dims.begin(), dims.end(), (size_t) 1, std::multiplies<size_t>());
	// compute number of blocks to sample in order to roughly maintain dimension ratio
	size_t blocksX = dims[0] / 4;
	size_t blocksY = dims[1] / 4;
	size_t nbBlocksX = sqrt(sample_ratio)*blocksX;
	size_t nbBlocksY = sqrt(sample_ratio)*blocksY;

	sample_num = nbBlocksX*nbBlocksY*16;
	sample_dims.push_back(nbBlocksX*4);
	sample_dims.push_back(nbBlocksY*4);

	std::vector<T> sample_data(sample_num, 0);

	for(size_t i = 0; i < nbBlocksX; i++){
		size_t x_block_ind = roundf(((float)i/(nbBlocksX-1)) * (blocksX-1));
		size_t x_ind = x_block_ind * 4;
		for(size_t j = 0; j < nbBlocksY; j++){
			size_t y_block_ind = roundf(((float)j/(nbBlocksY-1)) * (blocksY-1));
			size_t y_ind = y_block_ind * 4;

			for(size_t ii = 0; ii < 4; ii++) {
				for(size_t jj = 0; jj < 4; jj++) {
					size_t data_idx = (x_ind+ii)*dims[1] + y_ind + jj;
					size_t scal_i = i * 4;
					size_t scal_j = j * 4;
					size_t sample_idx = (scal_i+ii)*sample_dims[1] + scal_j + jj;

					sample_data[sample_idx] = data[data_idx];
				}
			}
		}
	}

	return sample_data;

}	


template<class T>
std::vector<T>
sampling_3d(T *data, std::vector<size_t> dims, size_t &sample_num, std::vector<size_t> &sample_dims, float sample_ratio) {
	size_t nbEle = std::accumulate(dims.begin(), dims.end(), (size_t) 1, std::multiplies<size_t>());
	// compute number of blocks to sample in order to roughly maintain dimension ratio
	size_t blocksX = dims[0] / 4;
	size_t blocksY = dims[1] / 4;
	size_t blocksZ = dims[2] / 4;
	size_t nbBlocksX = std::cbrt(sample_ratio)*blocksX;
	size_t nbBlocksY = std::cbrt(sample_ratio)*blocksY;
	size_t nbBlocksZ = std::cbrt(sample_ratio)*blocksZ;

	sample_num = nbBlocksX*nbBlocksY*nbBlocksZ*64;
	sample_dims.push_back(nbBlocksX*4);
	sample_dims.push_back(nbBlocksY*4);
	sample_dims.push_back(nbBlocksZ*4);


	std::vector<T> sample_data(sample_num, 0);

	for(size_t i = 0; i < nbBlocksX; i++){
		size_t x_block_ind = roundf(((float)i/(nbBlocksX-1)) * (blocksX-1));
		size_t x_ind = x_block_ind * 4;

		for(size_t j = 0; j < nbBlocksY; j++){
			size_t y_block_ind = roundf(((float)j/(nbBlocksY-1)) * (blocksY-1));
			size_t y_ind = y_block_ind * 4;

			for(size_t k = 0; k < nbBlocksZ; k++){
				size_t z_block_ind = roundf(((float)k/(nbBlocksZ-1)) * (blocksZ-1));
				size_t z_ind = z_block_ind * 4;

				for(size_t ii = 0; ii < 4; ii++) {
					for(size_t jj = 0; jj < 4; jj++) {
						for(size_t kk = 0; kk < 4; kk++){
							size_t data_idx = (x_ind+ii)*dims[1]*dims[2] + (y_ind+jj)*dims[2] + z_ind + kk;
							size_t scal_i = i * 4;
							size_t scal_j = j * 4;
							size_t scal_k = k * 4;
							size_t sample_idx = 
								(scal_i+ii)*sample_dims[1]*sample_dims[2] + (scal_j+jj)*sample_dims[2] + scal_k + kk;

							sample_data[sample_idx] = data[data_idx];
						}
					}
				}

			}
			
		}
	}

	return sample_data;

}	


namespace libpressio { namespace khan2023_zfp_metrics_ns {

class khan2023_zfp_plugin : public libpressio_metrics_plugin {
  public:
    int begin_compress_impl(struct pressio_data const* input, pressio_data const*) override {
      assert(input->dtype() == pressio_float_dtype);
      assert(input->num_dimensions() < 4);
      std::vector<size_t> dims = input->dimensions();
      size_t sample_num;
      std::vector<size_t> sample_dims;
      std::vector<float> sample_data;
      float * data_ = static_cast<float*>(input->data());
      switch(input->num_dimensions()){
			case 1:
				sample_data = sampling_1d<float>(data_, dims, sample_num, sample_dims, sample_ratio);
				break;
			case 2:
				sample_data = sampling_2d<float>(data_, dims, sample_num, sample_dims, sample_ratio);
				break;
			case 3:
				sample_data = sampling_3d<float>(data_, dims, sample_num, sample_dims, sample_ratio);
				break;
			default:
				return set_error(1, "Dims must be between size 1 and 3");
		}

      float* sampled_dataset = sample_data.data();
      struct pressio_data* input_data =
				pressio_data_new_move(pressio_float_dtype, sampled_dataset, sample_dims.size(), sample_dims.data(),
          pressio_data_libc_free_fn, NULL);
      struct pressio_data* compressed_data =
    			pressio_data_new_empty(pressio_byte_dtype, 0, NULL);

      struct pressio_options* cmp_options = pressio_compressor_get_options(&comp);
      pressio_options_set_double(cmp_options, "zfp:accuracy", accuracy);
      pressio_compressor_set_options(&comp, cmp_options);
      comp->compress(input_data, compressed_data);
      estimate = (double)input_data->size_in_bytes()/compressed_data->size_in_bytes();

      return 0;
    }

  
  struct pressio_options get_options() const override {
    pressio_options opts;
    set(opts, "khan2023_zfp:sample_ratio", sample_ratio);
    set(opts, "pressio:abs", accuracy);
    return opts;
  }
  int set_options(pressio_options const& opts) override {
    get(opts, "khan2023_zfp:sample_ratio", &sample_ratio);
    get(opts, "pressio:abs", &accuracy);
    return 0;
  }

  struct pressio_options get_configuration_impl() const override {
    pressio_options opts;
    set(opts, "predictors:invalidate", std::vector<std::string>{"predictors:error_dependent"});
    set(opts, "predictors:requires_decompress", false);
    set(opts, "pressio:stability", "stable");
    set(opts, "pressio:thread_safe", pressio_thread_safety_multiple);
    return opts;
  }

  struct pressio_options get_documentation_impl() const override {
    pressio_options opt;
    set(opt, "pressio:description", "estimates compression ratio for zfp given a sample_ratio in [0,1]");
    set(opt, "khan2023_zfp:sample_ratio", "ratio of the input dataset used for estimation, values should be in [0,1]");
    set(opt, "khan2023_zfp:accuracy", "accuracy setting for zfp estimation");
    set(opt, "pressio:abs", "accuracy setting for zfp estimation");
    set(opt, "khan2023_zfp:size:compression_ratio", "estimate of the compresssion ratio");
    return opt;
  }

  pressio_options get_metrics_results(pressio_options const &) override {
    pressio_options opt;
    set(opt, "khan2023_zfp:size:compression_ratio", estimate);
    return opt;
  }

  std::unique_ptr<libpressio_metrics_plugin> clone() override {
    return compat::make_unique<khan2023_zfp_plugin>(*this);
  }
  const char* prefix() const override {
    return "khan2023_zfp";
  }

  private:
    float sample_ratio = 0.01;
    double accuracy = 0.01;
    compat::optional<double> estimate;
    pressio_compressor comp = compressor_plugins().build("zfp");

};

static pressio_register metrics_khan2023_zfp_plugin(metrics_plugins(), "khan2023_zfp", [](){ return compat::make_unique<khan2023_zfp_plugin>(); });
}}

