#include <libpressio_predict_ext/cpp/simple_scheme.h>
#include <std_compat/memory.h>



namespace libpressio { namespace predict {

using namespace std::string_literals;


class sian2022_predict_scheme_plugin : public libpressio_simple_predict_scheme_plugin {
    public:
    int set_options(pressio_options const& op) final {
        return sian->set_options(op);
    }
    pressio_options get_options() const final {
        return sian->get_options();
    }
    pressio_options get_configuration_impl() const final {
        return sian->get_configuration();
    }
    pressio_options get_documentation() const final {
        return sian->get_documentation();
    }
    void set_name(std::string const& s) final {
        if(s.empty()) sian->set_name(s);
        else sian->set_name(s + '/' + sian->prefix());
    }
    const char* prefix() const final {
        return "sian2022";
    }
    std::unique_ptr<libpressio_predict_scheme_plugin> clone() const override {
        return compat::make_unique<sian2022_predict_scheme_plugin>(*this);
    }

    private:
    std::vector<metric_usage> metrics_to_modules() override {
        return {
            {"sian2022:size:compression_ratio", "sian2022", metric_usage::feature},
            {"size:compression_ratio", "size", metric_usage::label}
        };
    }
    std::map<std::string, pressio_metrics&> module_build_override() override {
        return {
            {"sian2022", sian}
        };
    }

    pressio_metrics sian = metrics_plugins().build("sian2022");
};

static pressio_register sian2022_register_var(scheme_plugins(), "sian2022", []() {
  return compat::make_unique<sian2022_predict_scheme_plugin>();
});

} }

