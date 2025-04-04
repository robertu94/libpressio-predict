#include <libpressio_predict_ext/cpp/simple_scheme.h>
#include <std_compat/memory.h>



namespace libpressio { namespace predict {

using namespace std::string_literals;


class tao2019_predict_scheme_plugin : public libpressio_simple_predict_scheme_plugin {
    public:
    int set_options(pressio_options const& op) final {
        return tao->set_options(op);
    }
    pressio_options get_options() const final {
        return tao->get_options();
    }
    pressio_options get_configuration_impl() const final {
        return tao->get_configuration();
    }
    pressio_options get_documentation() const final {
        return tao->get_documentation();
    }
    void set_name(std::string const& s) final {
        if(s.empty()) tao->set_name(s);
        else tao->set_name(s + '/' + tao->prefix());
    }
    const char* prefix() const final {
        return "tao2019";
    }
    std::unique_ptr<libpressio_predict_scheme_plugin> clone() const override {
        return compat::make_unique<tao2019_predict_scheme_plugin>(*this);
    }

    private:
    std::vector<metric_usage> metrics_to_modules() override {
        return {
            {"tao2019:size:compression_ratio", "tao2019", metric_usage::feature},
            {"size:compression_ratio", "size", metric_usage::label}
        };
    }
    std::map<std::string, pressio_metrics&> module_build_override() override {
        return {
            {"tao2019", tao}
        };
    }

    pressio_metrics tao = metrics_plugins().build("tao2019");
};

static pressio_register tao2019_register_var(scheme_plugins(), "tao2019", []() {
  return compat::make_unique<tao2019_predict_scheme_plugin>();
});

} }
