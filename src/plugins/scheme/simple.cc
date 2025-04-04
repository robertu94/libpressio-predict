#include <libpressio_ext/cpp/metrics.h>
#include <libpressio_predict_ext/cpp/simple_scheme.h>
#include <sstream>

using namespace std::string_literals;

namespace libpressio
{
    namespace predict
    {

    

    pressio_options libpressio_simple_predict_scheme_plugin::get_configuration_impl() const  {
        pressio_options opts;
        return opts;
    }

    std::vector<std::string> libpressio_simple_predict_scheme_plugin::get_feature_ids() {
        std::vector<std::string> res;
        for (auto const& i : metrics_to_modules()) {
            if(i.type == metric_usage::feature) {
                res.emplace_back(i.metric);
            }
        }
        return res;
    }
    std::vector<std::string> libpressio_simple_predict_scheme_plugin::get_invalid_feature_ids(pressio_compressor const& comp, pressio_options const& invalidations) {
        auto invalid = discover_invalidations(comp, invalidations);
        std::vector<std::string> res;
        std::copy_if(invalid.invalid_metrics.begin(), invalid.invalid_metrics.end(), std::back_inserter(res), 
                [&](metric_name const& invalid_metric) {
                    return invalid.causes[invalid_metric].type == metric_usage::feature;
                });
        return res;
    }
    std::vector<std::string> libpressio_simple_predict_scheme_plugin::get_valid_feature_ids(pressio_compressor const& comp, pressio_options const& invalidations) {
        auto invalid = discover_invalidations(comp, invalidations);
        std::vector<std::string> res;
        std::copy_if(invalid.valid_metrics.begin(), invalid.valid_metrics.end(), std::back_inserter(res), 
                [&](metric_name const& valid_metric) {
                    return invalid.causes[valid_metric].type == metric_usage::feature;
                });
        return res;
    }

    std::vector<std::string> libpressio_simple_predict_scheme_plugin::get_label_ids() {
        std::vector<std::string> res;
        for (auto const& i : metrics_to_modules()) {
            if(i.type == metric_usage::label) {
                res.emplace_back(i.metric);
            }
        }
        return res;
    }
    std::vector<std::string> libpressio_simple_predict_scheme_plugin::get_invalid_label_ids(pressio_compressor const& comp, pressio_options const& invalidations) {
        auto invalid = discover_invalidations(comp, invalidations);
        std::vector<std::string> res;
        std::copy_if(invalid.invalid_metrics.begin(), invalid.invalid_metrics.end(), std::back_inserter(res), 
                [&](metric_name const& invalid_metric) {
                    return invalid.causes[invalid_metric].type == metric_usage::label;
                });
        return res;
    }
    std::vector<std::string> libpressio_simple_predict_scheme_plugin::get_valid_label_ids(pressio_compressor const& comp, pressio_options const& invalidations) {
        auto invalid = discover_invalidations(comp, invalidations);
        std::vector<std::string> res;
        std::copy_if(invalid.valid_metrics.begin(), invalid.valid_metrics.end(), std::back_inserter(res), 
                [&](metric_name const& valid_metric) {
                    return invalid.causes[valid_metric].type == metric_usage::label;
                });
        return res;
    }
    pressio_compressor libpressio_simple_predict_scheme_plugin::get_metrics_collector(pressio_compressor &target_compressor, pressio_options const& invalidations) {
        auto invalid = discover_invalidations(target_compressor, invalidations);
        std::vector<pressio_metrics> required_modules;
        for(auto module: invalid.invalidated_modules) {
            required_modules.emplace_back(metrics_plugins().build(module));
        }
        pressio_compressor collector = ((training_required(target_compressor) && invalid.is_training) || invalid.collect_labels) ? 
            target_compressor->clone() : compressor_plugins().build("noop");
        collector->set_metrics(make_m_composite(std::move(required_modules)));
        collector->set_options(invalidations);

        return collector;
    }

    libpressio_predictor libpressio_simple_predict_scheme_plugin::get_predictor(pressio_compressor const& compressor) {
        return predictor_plugins().build("noop");
    }
    bool libpressio_simple_predict_scheme_plugin::decompress_required(pressio_compressor const& target_compressor, pressio_options const& invalidations) {
        auto invalid = discover_invalidations(target_compressor, invalidations);
        return std::any_of(invalid.invalid_metrics.begin(), invalid.invalid_metrics.end(), [&](metric_name const& invalid_metric){
                    return invalid.causes[invalid_metric].decompress_required;
                });
    }
    bool libpressio_simple_predict_scheme_plugin::compress_required(pressio_compressor const& target_compressor, pressio_options const& invalidations) {
        auto invalid = discover_invalidations(target_compressor, invalidations);
        return std::any_of(invalid.invalid_metrics.begin(), invalid.invalid_metrics.end(), [&](metric_name const& invalid_metric){
                    return invalid.causes[invalid_metric].decompress_required;
                });
    }
    bool libpressio_simple_predict_scheme_plugin::training_required(pressio_compressor const& target_compressor) {
        return get_predictor(target_compressor)->training_required();
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
