#include <algorithm>
#include <memory>
#include <sstream>
#include <iterator>
#include <mutex>
#include <condition_variable>
#include <mpi.h>
#include <vector>
#include "pressio_compressor.h"
#include "libpressio_ext/cpp/pressio.h"
#include "libpressio_ext/cpp/data.h"
#include "libpressio_ext/cpp/compressor.h"
#include "libpressio_ext/cpp/options.h"
#include "libpressio_ext/cpp/metrics.h"

#include "pressio_search.h"
#include "pressio_search_metrics.h"
#include "pressio_search_defines.h"


class pressio_proxy_search_plugin: public libpressio_compressor_plugin {

  int compress_impl(pressio_data const* input, pressio_data* compressed) override {
    (void)input;
    (void)compressed;
    *compressed = pressio_data::clone(*input);
    return 0;
  }

  int decompress_impl(pressio_data const* compressed, pressio_data* decompressed) override {
    (void)compressed;
    (void)decompressed;
    *decompressed = pressio_data::clone(*compressed);
    return 0;
  }

  pressio_options get_configuration_impl() const override{
    pressio_options opts;
    return opts;
  }

  pressio_options get_options_impl() const override{
    pressio_options opts;
    set(opts, "opt:inputs", inputs);
    set(opts, "opt:outputs", outputs);
    set(opts, "proxy_search:trace_format", trace_format);

    for (size_t i = 0; i < inputs.size(); ++i) {
      set(opts, inputs[i], input_v[i]);
    }
    auto trace_opts = trace_io->get_options();
    for (auto& trace_opt : trace_opts) {
      opts.set(trace_opt.first, trace_opt.second);
    }
    return opts;
  }


  int set_options_impl(pressio_options const& opts) override {
    if(get(opts, "opt:inputs", &inputs) == pressio_options_key_set) {
      input_v.resize(input_v.size());
    }
    if(get(opts, "opt:outputs", &outputs) == pressio_options_key_set) {
      output_v.resize(outputs.size());
    }

    std::string tmp_trace_format;
    if(get(opts, "proxy_search:trace_format", &tmp_trace_format) == pressio_options_key_set) {
      auto tmp = io_plugins().build(tmp_trace_format);
      if(tmp) {
        trace_io = std::move(tmp);
        trace_format = std::move(tmp_trace_format);
      }
    }
    trace_io->set_options(opts);

    for (size_t i = 0; i < input_v.size(); ++i) {
      get(opts, inputs[i], &input_v[i]);
    }
    return 0;
  }

  pressio_options get_metrics_results_impl() const override{
    pressio_options opts;
    for (size_t i = 0; i < outputs.size(); ++i) {
      set(opts, outputs[i], output_v[i]);
    }
    return opts;
  }


  std::shared_ptr<libpressio_compressor_plugin> clone() override {
    return compat::make_unique<pressio_proxy_search_plugin>(*this);
  }

  const char* prefix() const override {
    return "proxy_search";
  }

  const char* version() const override {
    return "0.0.0";
  }
  private:

  std::vector<std::string> inputs;
  std::vector<std::string> outputs;
  std::vector<double> input_v;
  std::vector<double> output_v;
  std::string trace_format;
  pressio_io trace_io;
};

static pressio_register X(compressor_plugins(), "proxy_search", [](){ return compat::make_unique<pressio_proxy_search_plugin>(); });
