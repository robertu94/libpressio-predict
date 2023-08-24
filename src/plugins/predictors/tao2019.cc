
#include "pressio_data.h"
#include "pressio_compressor.h"
#include "pressio_options.h"
#include "libpressio_ext/cpp/metrics.h"
#include "libpressio_ext/cpp/pressio.h"
#include "libpressio_ext/cpp/options.h"
#include <libpressio_dataset_ext/loader.h>
#include "std_compat/memory.h"

namespace libpressio { namespace tao2019_metrics_ns {

    using namespace libpressio_dataset;

class tao2019_plugin : public libpressio_metrics_plugin {
  public:
    int begin_compress_impl(struct pressio_data const* input, pressio_data const*) override {
      pressio_dataset_loader sampler = dataset_loader_plugins().build("block_sampler");
      pressio_data block_size_dat;
      if(block_size.size() != 0) {
          block_size_dat = pressio_data(block_size.begin(), block_size.end());
      } else {
          switch(input->num_dimensions()) {
              case 1:
                  block_size_dat = pressio_data{uint64_t{128}};
                  break;
              case 2:
                  block_size_dat = pressio_data{uint64_t{16}, uint64_t{16}};
                  break;
              case 3:
                  block_size_dat = pressio_data{uint64_t{8}, uint64_t{8}, uint64_t{8}};
                  break;
          }
      }
      sampler->set_options({
        {"block_sampler:loader", "from_data"},
        {"block_sampler:block_size", block_size_dat},
        {"block_sampler:n", n},
        {"block_sampler:seed", seed},
        {"from_data:n", uint64_t{1}},
        {"from_data:data-0", pressio_data::nonowning(input->dtype(), input->data(), input->dimensions())}
      });

      std::vector<pressio_data> samples = sampler->load_all_data();
      cr_samples.clear();
      double total_cr = 0;
      for (auto const& s : samples) {
          pressio_data compressed;
          comp->compress(&s, &compressed);
          cr_samples.emplace_back((double)s.size_in_bytes()/compressed.size_in_bytes());
          total_cr += (cr_samples.back());
      }
      estimate = total_cr / samples.size();
      return 0;
    }

  
  struct pressio_options get_options() const override {
    pressio_options opts;
    set_meta(opts, "pressio:compressor", comp_id, comp);
    set_meta(opts, "tao2019:compressor", comp_id, comp);
    set(opts, "tao2019:n", n);
    set(opts, "tao2019:seed", seed);
    set(opts, "tao2019:block_size", pressio_data(block_size.begin(), block_size.end()));
    return opts;
  }
  int set_options(pressio_options const& opts) override {
    get_meta(opts, "pressio:compressor", compressor_plugins(), comp_id, comp);
    get_meta(opts, "tao2019:compressor", compressor_plugins(), comp_id, comp);
    get(opts, "tao2019:n", &n);
    get(opts, "tao2019:seed", &seed);
    pressio_data tmp;
    if(get(opts, "tao2019:block_size", &tmp) == pressio_options_key_set) {
        block_size = tmp.to_vector<uint64_t>();
    }

    return 0;
  }

  struct pressio_options get_configuration_impl() const override {
    pressio_options opts;
    set_meta_configuration(opts, "pressio:compressor", compressor_plugins(), comp);
    set_meta_configuration(opts, "tao2019:compressor", compressor_plugins(), comp);
    set(opts, "predictors:invalidate", std::vector<std::string>{"predictors:error_dependent"});
    set(opts, "pressio:stability", "stable");
    set(opts, "pressio:thread_safe", pressio_thread_safety_multiple);
    return opts;
  }

  struct pressio_options get_documentation_impl() const override {
    pressio_options opt;
    set_meta_docs(opt, "pressio:compressor", "compressor to predict", comp);
    set_meta_docs(opt, "tao2019:compressor", "compressor to predict", comp);
    set(opt, "pressio:description", "predicts the compressability of datasets using samples");
    set(opt, "tao2019:n", "number of samples to take");
    set(opt, "tao2019:block_size", "size of blocks to use");
    set(opt, "tao2019:seed", "seed");
    set(opt, "tao2019:size:compression_ratio", "estimate of the compression ratio");
    set(opt, "tao2019:compression_ratios", "compression ratios of the samples");
    return opt;
  }

  pressio_options get_metrics_results(pressio_options const &) override {
    pressio_options opt;
    set(opt, "tao2019:size:compression_ratio", estimate);
    set(opt, "tao2019:compression_ratios", pressio_data(cr_samples.begin(), cr_samples.end()));
    return opt;
  }

  void set_name_impl(std::string const& s) override {
      comp->set_name(s + "/" + comp->prefix());
  }

  std::unique_ptr<libpressio_metrics_plugin> clone() override {
    return compat::make_unique<tao2019_plugin>(*this);
  }
  const char* prefix() const override {
    return "tao2019";
  }

  private:
    uint64_t n = 32;
    uint64_t seed = 0;
    std::vector<double> cr_samples;
    std::vector<uint64_t> block_size;
    std::string comp_id = "noop";
    pressio_compressor comp = compressor_plugins().build(comp_id);
    compat::optional<double> estimate;
};

static pressio_register metrics_tao2019_plugin(metrics_plugins(), "tao2019", [](){ return compat::make_unique<tao2019_plugin>(); });
}}

