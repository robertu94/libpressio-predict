#include <libpressio_predict_ext/cpp/predict.h>
#include <set>
#include <vector>
#include <sstream>
#include <std_compat/memory.h>
#include <string>



namespace libpressio { namespace predict {

using namespace std::string_literals;


class tao2019_predict_scheme_plugin : public libpressio_simple_predict_scheme_plugin {
    public:
    std::vector<metric_usage> metrics_to_modules() {
        return {
            {"tao2019:size:compression_ratio", "tao2019", false}
        };
    }
    const char* prefix() const final {
        return "tao2019";
    }
    std::unique_ptr<libpressio_predict_scheme_plugin> clone() const {
        return compat::make_unique<tao2019_predict_scheme_plugin>(*this);
    }

    virtual libpressio_predictor get_predictor(pressio_compressor const& compressor) override {
        auto pred = predictor_plugins().build("R");
        pred->set_options({{"R:code", "R()"}});
        return pred;
    }

};

pressio_registry<std::unique_ptr<libpressio_predict_scheme_plugin>>& scheme_plugins() {
    static pressio_registry<std::unique_ptr<libpressio_predict_scheme_plugin>> registry;
    return registry;
}

static pressio_register tao2019_register_var(scheme_plugins(), "tao2019", []() {
  return compat::make_unique<tao2019_predict_scheme_plugin>();
});

} }
