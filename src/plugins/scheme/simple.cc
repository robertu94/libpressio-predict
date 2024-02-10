#include <libpressio_predict_ext/cpp/predict.h>
#include <sstream>

namespace libpressio
{
    namespace predict
    {

    pressio_options libpressio_predict_scheme_plugin::get_configuration() const  {
        return get_configuration_impl();
    }
    pressio_options libpressio_simple_predict_scheme_plugin::get_configuration_impl() const  {
        pressio_options opts;
        return opts;
    }

    std::vector<std::string> libpressio_simple_predict_scheme_plugin::req_metrics() {
        std::vector<std::string> res;
        for (auto const& i : metrics_to_modules()) {
            res.emplace_back(i.metric);
        }
        return res;
    }
    pressio_compressor libpressio_simple_predict_scheme_plugin::req_metrics_opts(pressio_compressor &target_compressor, std::vector<std::string> const& invalidations) {
        return target_compressor;
    }

    libpressio_predictor libpressio_simple_predict_scheme_plugin::get_predictor(pressio_compressor const& compressor) {
        return predictor_plugins().build("noop");
    }
    bool libpressio_simple_predict_scheme_plugin::do_decompress(pressio_compressor const& target_compressor, std::vector<std::string> const& invalidations) {
        return requires_compressor(target_compressor, invalidations, "predictors:requires_decompress");
    }
    bool libpressio_simple_predict_scheme_plugin::do_compress(pressio_compressor const& target_compressor, std::vector<std::string> const& invalidations) {
        return requires_compressor(target_compressor, invalidations, "predictors:requires_compress");
    }
    char const* libpressio_simple_predict_scheme_plugin::version() const {
        static std::string str = [this]{
            std::stringstream ss;
            ss << this->major_version() << '.' << this->minor_version() << '.' << this->patch_version();
            return ss.str();
        }();
        return str.c_str();
    }
        
    } /* predict */ 
} /* libpressio */ 
