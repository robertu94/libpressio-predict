
#include "pressio_data.h"
#include "pressio_compressor.h"
#include "pressio_options.h"
#include "libpressio_ext/cpp/metrics.h"
#include "libpressio_ext/cpp/pressio.h"
#include "libpressio_ext/cpp/options.h"
#include "std_compat/memory.h"
#include "SZ3/frontend/SZGeneralFrontend.hpp"
#include "SZ3/quantizer/Quantizer.hpp"
#include "SZ3/quantizer/IntegerQuantizer.hpp"
#include "SZ3/predictor/LorenzoPredictor.hpp"

namespace libpressio { namespace sian2022_metrics_ns {
class sian2022_plugin : public libpressio_metrics_plugin {
  public:
    template<uint N, typename T>
    double compute_estimate(SZ::Config conf, T *data) {
      std::copy_n(conf.dims.begin(), N, global_dimensions.begin());
      int block_size = conf.blockSize;
      auto block_range = std::make_shared<SZ::multi_dimensional_range<float, N>>(data, std::begin(global_dimensions), std::end(global_dimensions), block_size, 0);
      auto element_range = std::make_shared<SZ::multi_dimensional_range<float, N>>(data, std::begin(global_dimensions), std::end(global_dimensions), 1, 0);
      std::vector<int> quant_inds(conf.num);
      size_t quant_count = 0;
      int pre_num = 0;
      double prediction = 0;
      int ii = 0;
      std::unordered_map<float, size_t> pre_freq;

      SZ::LorenzoPredictor<float,N,1> predictor;
      SZ::LinearQuantizer<float> quantizer;
      quantizer.set_eb(eb);
      predictor.precompress_data(block_range->begin());
      quantizer.precompress_data();

      auto end = block_range->end();
      for (auto block = block_range->begin(); block != end; ++block) {
        element_range->update_block_range(block, block_size);
        auto eend = element_range->end();
        for (auto element = element_range->begin(); element != eend; ++element) {
          ii++;
	  quant_inds[quant_count++] = quantizer.quantize_and_overwrite(*element, predictor.predict(element));
	  if (ii % 100 == 0) {
            pre_num++;
	    pre_freq[quant_inds[quant_count-1]]++;
          }
	}
      }

//      std::cout << pre_num << " " << pre_freq[32767] << " " << pre_freq[32768] << " " << pre_freq[32769] << std::endl;

      predictor.postcompress_data(block_range->begin());
      quantizer.postcompress_data();

      float temp_bit = 0;
      float p_0 = (float)pre_freq[32768]/pre_num;
      float P_0;
      float C_1 = 1.0;
      float pre_lossless = 1.0;
      for (int i = 1; i < 65536; i++) {
        if (pre_freq[i] != 0) {
          temp_bit = -log2((float)pre_freq[i]/pre_num);
          if (temp_bit < 32) {
            if (temp_bit < 1) {
	      if (i == 32768) prediction += ((float)pre_freq[i]/pre_num) * 1;
              else if (i == 32767) prediction += ((float)pre_freq[i]/pre_num) * 2.5;
              else if (i == 32769) prediction += ((float)pre_freq[i]/pre_num) * 2.5;
              else prediction += ((float)pre_freq[i]/pre_num) * 4;
            }
            else
              prediction += ((float)pre_freq[i]/pre_num) * temp_bit;
          }
        }
      }
      if (pre_freq[0] != 0) 
        prediction += ((float)pre_freq[0]/pre_num) * 32;
      if (pre_freq[32768] > pre_num/2)
        P_0 = (((float)pre_freq[32768]/pre_num) * 1.0)/prediction;
      else
        P_0 = -(((float)pre_freq[32768]/pre_num) * log2((float)pre_freq[32768]/pre_num))/prediction;

      pre_lossless = 1 / (C_1 * (1 - p_0) * P_0 + (1 - P_0));
      if (pre_lossless < 1) pre_lossless = 1;
      prediction = prediction / pre_lossless;
      
      return 32/prediction;
    }

    int begin_compress_impl(struct pressio_data const* input, pressio_data const*) override {
      assert(input->dtype() == pressio_float_dtype);
      assert(input->num_dimensions() < 5);
      auto dimensions = input->dimensions();
      int nn = input->num_dimensions();
      float * data = static_cast<float*>(input->data());
      switch(input->num_dimensions()) {
        case 1:
        {
          SZ::Config conf(dimensions[0]);
          estimate = compute_estimate<1>(conf, data);
	  break;
	}
	case 2:
        {
          SZ::Config conf(dimensions[1], dimensions[0]);
          estimate = compute_estimate<2>(conf, data);
	  break;
	}
        case 3:
        {
          SZ::Config conf(dimensions[2], dimensions[1], dimensions[0]);
	  estimate = compute_estimate<3>(conf, data);
	  break;
	}
        case 4:
        {
          SZ::Config conf(dimensions[3], dimensions[2], dimensions[1], dimensions[0]);
          estimate = compute_estimate<4>(conf, data);
	  break;
        }
      }
      return 0;
    }

  struct pressio_options get_options() const override {
    pressio_options opts;
    set(opts, "pressio:abs", eb);
    set(opts, "sian2022:error_bound", eb);
    return opts;
  }

  int set_options(pressio_options const& opts) override {
    get(opts, "pressio:abs", &eb);
    get(opts, "sian2022:error_bound", &eb);
    return 0;
  }
  
  struct pressio_options get_configuration_impl() const override {
    pressio_options opts;
    set(opts, "pressio:stability", "stable");
    set(opts, "pressio:thread_safe", pressio_thread_safety_multiple);
    return opts;
  }

  struct pressio_options get_documentation_impl() const override {
    pressio_options opt;
    set(opt, "pressio:description", "");
    return opt;
  }

  pressio_options get_metrics_results(pressio_options const &) override {
    pressio_options opt;
    set(opt, "sian2022:size:compression_ratio", estimate);
    return opt;
  }

  std::unique_ptr<libpressio_metrics_plugin> clone() override {
    return compat::make_unique<sian2022_plugin>(*this);
  }
  const char* prefix() const override {
    return "sian2022";
  }

  private:
    double eb = 1e-6;
    compat::optional<double> estimate;
    std::array<float, 3> global_dimensions;
};

static pressio_register metrics_sian2022_plugin(metrics_plugins(), "sian2022", [](){ return compat::make_unique<sian2022_plugin>(); });
}}

