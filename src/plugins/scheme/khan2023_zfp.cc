#include <libpressio_predict_ext/cpp/simple_scheme.h>
#include <std_compat/memory.h>



namespace libpressio { namespace predict {

using namespace std::string_literals;


class khan2023_zfp_predict_scheme_plugin : public libpressio_simple_predict_scheme_plugin {
    public:
    int set_options(pressio_options const& op) final {
        return khan2023_zfp->set_options(op);
    }
    pressio_options get_options() const final {
        return khan2023_zfp->get_options();
    }
    pressio_options get_configuration_impl() const final {
        return khan2023_zfp->get_configuration();
    }
    pressio_options get_documentation() const final {
        return khan2023_zfp->get_documentation();
    }
    void set_name(std::string const& s) final {
        if(s.empty()) khan2023_zfp->set_name(s);
        else khan2023_zfp->set_name(s + '/' + khan2023_zfp->prefix());
    }
    const char* prefix() const final {
        return "khan2023_zfp";
    }
    std::unique_ptr<libpressio_predict_scheme_plugin> clone() const override {
        return compat::make_unique<khan2023_zfp_predict_scheme_plugin>(*this);
    }

    private:
    std::vector<metric_usage> metrics_to_modules() override {
        return {
            {"khan2023_zfp:size:compression_ratio", "khan2023_zfp", metric_usage::feature},
            {"size:compression_ratio", "size", metric_usage::label}
        };
    }
    std::map<std::string, pressio_metrics&> module_build_override() override {
        return {
            {"khan2023_zfp", khan2023_zfp}
        };
    }

    pressio_metrics khan2023_zfp = metrics_plugins().build("khan2023_zfp");
};

static pressio_register khan2023_zfp_register_var(scheme_plugins(), "khan2023_zfp", []() {
  return compat::make_unique<khan2023_zfp_predict_scheme_plugin>();
});

} }

