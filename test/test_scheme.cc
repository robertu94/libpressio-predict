#include "libpressio_predict_ext/cpp/predict.h"
#include <cmath>

using namespace libpressio::predict;
using namespace std::string_literals;

#include "std_compat/memory.h"
#include "libpressio_ext/cpp/compressor.h"
#include "libpressio_ext/cpp/data.h"
#include "libpressio_ext/cpp/options.h"
#include "libpressio_ext/cpp/pressio.h"

namespace libpressio { namespace mock_compressor_ns {

class mock_compressor_compressor_plugin : public libpressio_compressor_plugin {
public:
  struct pressio_options get_options_impl() const override
  {
    struct pressio_options options;
    return options;
  }

  struct pressio_options get_configuration_impl() const override
  {
    struct pressio_options options;
    set(options, "pressio:thread_safe", pressio_thread_safety_multiple);
    set(options, "pressio:stability", "experimental");
    return options;
  }

  struct pressio_options get_documentation_impl() const override
  {
    struct pressio_options options;
    set(options, "pressio:description", R"()");
    return options;
  }


  int set_options_impl(struct pressio_options const& options) override
  {
    return 0;
  }

  int compress_impl(const pressio_data* input,
                    struct pressio_data* output) override
  {
      return 0;
  }

  int decompress_impl(const pressio_data* input,
                      struct pressio_data* output) override
  {
      return 0;
  }

  int major_version() const override { return 0; }
  int minor_version() const override { return 0; }
  int patch_version() const override { return 1; }
  const char* version() const override { return "0.0.1"; }
  const char* prefix() const override { return "mock_compressor"; }

  pressio_options get_metrics_results_impl() const override {
    return {};
  }

  std::shared_ptr<libpressio_compressor_plugin> clone() override
  {
    return compat::make_unique<mock_compressor_compressor_plugin>(*this);
  }

};

static pressio_register compressor_many_fields_plugin(compressor_plugins(), "mock", []() {
  return compat::make_unique<mock_compressor_compressor_plugin>();
});

} }

#include "pressio_data.h"
#include "pressio_compressor.h"
#include "pressio_options.h"
#include "libpressio_ext/cpp/metrics.h"
#include "libpressio_ext/cpp/pressio.h"
#include "libpressio_ext/cpp/options.h"
#include "std_compat/memory.h"

namespace libpressio { namespace mock_metrics_ns {

class mock_plugin : public libpressio_metrics_plugin {
  public:
    int end_compress_impl(struct pressio_data const* input, pressio_data const* output, int) override {
      return 0;
    }

    int end_decompress_impl(struct pressio_data const* , pressio_data const* output, int) override {
      return 0;
    }

    int end_compress_many_impl(compat::span<const pressio_data* const> const& inputs,
                                   compat::span<const pressio_data* const> const& outputs, int ) override {
      return 0;
  }

  int end_decompress_many_impl(compat::span<const pressio_data* const> const& ,
                                   compat::span<const pressio_data* const> const& outputs, int ) override {
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
    return opt;
  }

  std::unique_ptr<libpressio_metrics_plugin> clone() override {
    return compat::make_unique<mock_plugin>(*this);
  }
  const char* prefix() const override {
    return "mock";
  }

  private:

};

static pressio_register metrics_mock_plugin(metrics_plugins(), "mock", [](){ return compat::make_unique<mock_plugin>(); });
}}


class mock_predict_scheme_plugin : public libpressio_simple_predict_scheme_plugin {
    public:
    std::vector<metric_usage> metrics_to_modules() {
        return {
            {"mock:size:compression_ratio", "mock", false}
        };
    }
    const char* prefix() const final {
        return "mock";
    }
    std::unique_ptr<libpressio_predict_scheme_plugin> clone() const {
        return compat::make_unique<mock_predict_scheme_plugin>(*this);
    }
};

int main(int argc, char *argv[])
{
    std::vector<std::string> inputs {"mock:a_data", "mock:b_training", "mock:c_errordep"};
    auto input = pressio_data::owning(pressio_float_dtype, {100, 100});
    auto input_ptr = static_cast<float*>(input.data());
    auto const stride = input.get_dimension(1);
    for (int i = 0; i < input.get_dimension(0); ++i) {
        for (int j = 0; j < input.get_dimension(1); ++j) {
            input_ptr[j+stride*i] = std::log(std::pow(pow(sin(.1*i+.2*j),2)/cos(.1*j),2.0));
            
        }
        
    }

    std::string comp_id = "mock";
    pressio_compressor comp = compressor_plugins().build(comp_id);

    std::string scheme_id = "mock";
    libpressio_scheme scheme = scheme_plugins().build(scheme_id);
    
    const auto do_compress = scheme->do_compress(comp, inputs);
    const auto do_decompress = scheme->do_decompress(comp, inputs);
    auto predictor = scheme->get_predictor(comp);
    auto metrics = scheme->req_metrics_opts(comp, inputs);
    auto metrics_ids = scheme->req_metrics();
    if(do_compress || do_decompress) {
      pressio_data compressed;
      std::vector<pressio_data*> compressed_ptrs;
      if(do_compress) {
          int ec = metrics->compress(&input, &compressed);
          if(ec < 0) {
              throw std::runtime_error("compression failed in the proxy_fn "s + metrics->error_msg());
          }
      }
      if(do_decompress) {
          pressio_data output;
          int ec = metrics->decompress(&compressed, &output);
          if(ec < 0) {
              throw std::runtime_error("decompression failed in the proxy_fn "s + metrics->error_msg());
          }
      }
    }
    auto metrics_results = metrics->get_metrics_results();
    std::vector<double> features_v;
    for (auto const& i : metrics_ids) {
      auto status = metrics_results.key_status(i);
      if(status == pressio_options_key_exists || status == pressio_options_key_set) {
          auto const& opt = metrics_results.get(i).as(pressio_option_double_type, pressio_conversion_special);
          if(opt.has_value() && opt.type() == pressio_option_double_type) {
              features_v.emplace_back(opt.get_value<double>());
          } else {
              throw std::runtime_error("converting metric key " + i + " failed");
          }
      } else {
          throw std::runtime_error("metric key " + i + " does not exist");
      }
    }

    pressio_data features = pressio_data(features_v.begin(), features_v.end());
    pressio_data labels;

    int ec = predictor->predict(features, labels);
    if(ec > 0) {
          throw std::runtime_error("prediction failed "s + predictor->error_msg());
    }
}
