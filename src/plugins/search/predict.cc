#include <string>
#include <libpressio_predict.h>
#include "pressio_search.h"
#include "pressio_search_results.h"
#include <std_compat/memory.h>
#include "libpressio_predict_ext/cpp/scheme.h"
#include "libpressio_predict_version.h"



namespace libpressio { namespace predict {
    using namespace std::string_literals;


struct predict_search: public pressio_search_plugin {
  public:
    pressio_search_results search(compat::span<const pressio_data *const> const &input_datas,
                                  std::function<pressio_search_results::output_type(
                                          pressio_search_results::input_type const &)> compress_fn,
                                  distributed::queue::StopToken &token) override {
        return pressio_search_results();
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
