#include <libpressio_predict_ext/cpp/scheme.h>
#include <sstream>
#include <iomanip>

using namespace std::string_literals;

namespace libpressio
{
    namespace predict
    {
        pressio_compressor libpressio_predict_scheme_plugin::run_collector(pressio_compressor& comp, pressio_options const& invalidations, pressio_data& input, pressio_data& compressed, pressio_data& output) {
            auto metrics_collector = get_metrics_collector(comp, invalidations);
            const bool compress_required = this->compress_required(comp, invalidations);
            const bool decompress_required = this->decompress_required(comp, invalidations);
            if(compress_required) {
                int ec = metrics_collector->compress(&input, &compressed);
                if(ec < 0) {
                    throw std::runtime_error("compression failed in the proxy_fn "s + metrics_collector->error_msg());
                }
            }
            if(decompress_required) {
                pressio_data output;
                int ec = metrics_collector->decompress(&compressed, &output);
                if(ec < 0) {
                    throw std::runtime_error("decompression failed in the proxy_fn "s + metrics_collector->error_msg());
                }
            }
            return metrics_collector;
        }

        pressio_options libpressio_predict_scheme_plugin::get_configuration() const  {
            return get_configuration_impl();
        }

        void libpressio_predict_scheme_plugin::copy_metric_results(pressio_data& entry, pressio_options const& metric_results, std::vector<std::string> const& keys, pressio_options const& cache) const {
            if(keys.size() != entry.num_elements()) {
                throw std::runtime_error("invalid size of entry");
            }
            double* entry_p = static_cast<double*>(entry.data());
            for(size_t i = 0; i<keys.size(); ++i) {
                if(metric_results.key_status(keys[i]) == pressio_options_key_set) {
                    auto op = metric_results.get(keys[i]).as(pressio_option_double_type, pressio_conversion_special);
                    if(op.has_value() && op.type() == pressio_option_double_type) {
                        entry_p[i] = op.get_value<double>();
                    } else {
                        std::stringstream ss;
                        ss << "required key " << std::quoted(keys[i]) << " could not be converted from metrics_results to double";
                        throw std::runtime_error(ss.str());
                    }
                } else if(cache.key_status(keys[i]) == pressio_options_key_set) {
                    auto op = cache.get(keys[i]).as(pressio_option_double_type, pressio_conversion_special);
                    if(op.has_value() && op.type() == pressio_option_double_type) {
                        entry_p[i] = op.get_value<double>();
                    } else {
                        std::stringstream ss;
                        ss << "required key " << std::quoted(keys[i]) << " could not be converted from cache to double";
                        throw std::runtime_error(ss.str());
                    }
                } else {
                    std::stringstream ss;
                    ss << "required key " << std::quoted(keys[i]) << " was not collected or in the cache";
                    throw std::runtime_error(ss.str());
                }
            }
        }

        std::pair<pressio_data, pressio_data> libpressio_predict_scheme_plugin::get_features_and_labels(pressio_compressor const& comp, pressio_options const& invalidations, pressio_options const & cache)  {
            pressio_data features = pressio_data::owning(pressio_double_dtype, {get_feature_ids().size()});
            pressio_data labels = pressio_data::owning(pressio_double_dtype, {get_label_ids().size()});
            pressio_options results = comp->get_metrics_results();
            copy_metric_results(features, results, get_feature_ids(), cache);
            copy_metric_results(labels, results, get_label_ids(), cache);
            return std::make_pair(features, labels);
        }

    
    pressio_data libpressio_predict_scheme_plugin::get_labels(pressio_compressor const& comp, pressio_options const& invalidations, pressio_options const & cache)  {
        auto label_ids = get_label_ids();
        pressio_data results = pressio_data::owning(pressio_double_dtype, {label_ids.size()});
        copy_metric_results(results, comp->get_metrics_results(), label_ids, cache);
        return results;
    }

    pressio_data libpressio_predict_scheme_plugin::get_features(pressio_compressor const& comp, pressio_options const& invalidations, pressio_options const & cache)  {
        auto feature_ids = get_feature_ids();
        pressio_data results = pressio_data::owning(pressio_double_dtype, {feature_ids.size()});
        copy_metric_results(results, comp->get_metrics_results(), feature_ids, cache);
        return results;
    }

    std::pair<pressio_data, pressio_data> libpressio_predict_scheme_plugin::get_features_and_labels(pressio_compressor& comp, pressio_options const& invalidations, pressio_data& input, pressio_data& compressed, pressio_data& output,  pressio_options const& cache) {
        return get_features_and_labels(run_collector(comp, invalidations, input, compressed, output), invalidations, cache);
    }

    pressio_data libpressio_predict_scheme_plugin::get_labels(pressio_compressor& comp, pressio_options const& invalidations, pressio_data& input, pressio_data& compressed, pressio_data& output,  pressio_options const& cache) {
        return get_labels(run_collector(comp, invalidations, input, compressed, output), invalidations, cache);
    }

    pressio_data libpressio_predict_scheme_plugin::get_features(pressio_compressor& comp, pressio_options const& invalidations, pressio_data& input, pressio_data& compressed, pressio_data& output,  pressio_options const& cache) {
        return get_features(run_collector(comp, invalidations, input, compressed, output), invalidations, cache);
    }

    pressio_registry<std::unique_ptr<libpressio_predict_scheme_plugin>>& scheme_plugins() {
        static pressio_registry<std::unique_ptr<libpressio_predict_scheme_plugin>> registry;
        return registry;
    }

    } /* predict */ 
} /* libpressio */ 
