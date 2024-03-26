#include <string>
#include <iterator>
#include <libpressio_predict.h>
#include "pressio_search.h"
#include "pressio_search_results.h"
#include <std_compat/memory.h>
#include "libpressio_predict_ext/cpp/predict.h"
#include "libpressio_predict_version.h"



namespace libpressio { namespace predict {
    using namespace std::string_literals;


struct predict_search: public pressio_search_plugin {
  public:
    pressio_search_results search(compat::span<const pressio_data *const> const &input_datas,
                                  std::function<pressio_search_results::output_type(
                                          pressio_search_results::input_type const &)> compress_fn,
                                  distributed::queue::StopToken &token) override {
      const auto do_compress = scheme->do_compress(comp, inputs);
      const auto do_decompress = scheme->do_decompress(comp, inputs);
      const auto metrics_ids = scheme->req_metrics();

      const auto data_do_compress = scheme->do_compress(comp, {"predictors:data"});
      const auto data_do_decompress = scheme->do_decompress(comp, {"predictors:data"});
      auto data_metrics = scheme->req_metrics_opts(comp, {"predictors:data"});
      //TODO implement the call for the data-only metrics
      
      auto predictor = scheme->get_predictor(comp);
      auto metrics = scheme->req_metrics_opts(comp, inputs);
      auto proxy_fn = [&](pressio_search_results::input_type const & in) -> pressio_search_results::output_type {
          if(do_compress || do_decompress) {
              std::vector<pressio_data> compressed(input_datas.size(), pressio_data::empty(pressio_byte_dtype, {}));
              std::vector<pressio_data*> compressed_ptrs;
              std::transform(compressed.begin(), compressed.end(), std::back_inserter(compressed_ptrs), [](auto& data) {
                      return &data;
              });
              if(do_compress || do_decompress) {
                  //if decompression is required, we must also compress or else we dont have the data we need
/*                  int ec = metrics->compress_many(input_datas.begin(), input_datas.end(), 
                                         compressed_ptrs.begin(), compressed_ptrs.end());
                  if(ec < 0) {
                      throw pressio_search_exception("compression failed in the proxy_fn "s + metrics->error_msg());
                  }
 */             }
              if(do_decompress) {
                  std::vector<pressio_data> outputs;
                  std::transform(input_datas.begin(), input_datas.end(), std::back_inserter(outputs), [](auto const& data) {
                          return pressio_data::clone(*data);
                  });
                  std::vector<pressio_data*> output_ptrs;
                  std::transform(outputs.begin(), outputs.end(), std::back_inserter(output_ptrs), [](auto& data) {
                          return &data;
                  });
/*
                  int ec = metrics->decompress_many(compressed_ptrs.begin(), compressed_ptrs.end(),
                                           output_ptrs.begin(), output_ptrs.end());
                  if(ec < 0) {
                      throw pressio_search_exception("decompression failed in the proxy_fn "s + metrics->error_msg());
                  }
 */             }
          }
          auto metrics_results = metrics->get_metrics_results();
          std::vector<double> features_v; 
          //TODO copy in the data dependent rigs metrics
          for (auto const& i : metrics_ids) {
              auto status = metrics_results.key_status(i);
              if(status == pressio_options_key_exists || status == pressio_options_key_set) {
                  auto const& opt = metrics_results.get(i).as(pressio_option_double_type, pressio_conversion_special);
                  if(opt.has_value() && opt.type() == pressio_option_double_type) {
                      features_v.emplace_back(opt.get_value<double>());
                  } else {
                      throw pressio_search_exception("converting metric key " + i + " failed");
                  }
              } else {
                  throw pressio_search_exception("metric key " + i + " does not exist");
              }
          }

          pressio_data features = pressio_data(features_v.begin(), features_v.end());
          pressio_data labels;

          int ec = predictor->predict(features, labels);
          if(ec > 0) {
                  throw pressio_search_exception("prediction failed "s + predictor->error_msg());
          }
          return labels.to_vector<double>();
      };
      auto results = proxy_search->search(input_datas, proxy_fn, token);
      return results;
    }

    //configuration
    pressio_options get_options() const override {
      pressio_options opts;
      set_meta(opts, "predict:search", proxy_search_id, proxy_search);
      set_meta(opts, "opt:compressor", comp_id, comp);
      set_meta(opts, "predict:scheme", scheme_id, scheme);
      return opts;
    }
    pressio_options get_configuration_impl() const override {
      pressio_options opts;
      set_meta_configuration(opts, "predict:search", search_plugins(), proxy_search);
      set_meta_configuration(opts, "opt:compressor", compressor_plugins(), comp);
      set_meta_configuration(opts, "predict:scheme", scheme_plugins(), scheme);
      return opts;
    }
    pressio_options get_documentation() const override {
      pressio_options opts;
      set_meta_docs(opts, "predict:search", "search method to use with proxy", proxy_search);
      set_meta_docs(opts, "opt:compressor", "compressor to proxy", comp);
      set_meta_docs(opts, "predict:scheme", "prediction scheme to build proxy", scheme);
      return opts;
    }


    int set_options(pressio_options const& options) override {
      get_meta(options, "predict:search", search_plugins(), proxy_search_id, proxy_search);
      get_meta(options, "opt:compressor", compressor_plugins(), comp_id, comp);
      get_meta(options, "predict:scheme", scheme_plugins(), scheme_id, scheme);
      return 0;
    }
    
    const char* prefix() const override {
      return "predict";
    }
    const char* version() const override {
      return "0.0.1";
    }
    void set_name_impl(std::string const& new_name) override {
      proxy_search->set_name(new_name + "/search");
      comp->set_name(new_name + "/compressor");
      scheme->set_name(new_name + "/scheme");
    }


    int major_version() const override { return LIBPRESSIO_PREDICT_MAJOR_VERSION; }
    int minor_version() const override { return LIBPRESSIO_PREDICT_MINOR_VERSION; }
    int patch_version() const override { return LIBPRESSIO_PREDICT_PATCH_VERSION; }

    std::shared_ptr<pressio_search_plugin> clone() override {
      return compat::make_unique<predict_search>(*this);
    }
private:
    std::vector<std::string> inputs;
    std::string scheme_id = "noop";
    libpressio_scheme scheme = scheme_plugins().build(scheme_id);

    std::string proxy_search_id = "guess";
    pressio_search proxy_search = search_plugins().build(proxy_search_id);

    std::string comp_id = "noop";
    pressio_compressor comp = compressor_plugins().build(comp_id);
};


static pressio_register predict_register(search_plugins(), "predict", [](){ return compat::make_unique<predict_search>();});

}}
