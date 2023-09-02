#include <libpressio_predict_ext/cpp/predict.h>

namespace libpressio { namespace predict {

class libpressio_noop_predict final : public libpressio_predict_plugin {

    std::unique_ptr<libpressio_predict_plugin> clone() const override {
        return std::make_unique<libpressio_noop_predict>(*this);
    }
    int fit_impl(pressio_data const&, pressio_data const&) override {
        /* we don't do any training here, so just return immediately */
        return 0;
    }
    int predict_impl(pressio_data const& input_data, pressio_data& out) override {
        out = input_data;
        return 0;
    }

    int patch_version() const override {
        return 1;
    }

    const char* prefix() const override {
        return "noop";
    }
};

} }
