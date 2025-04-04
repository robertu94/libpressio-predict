#ifndef LIBPRESSIO_PREDICT_SIMPLE_SCHEME_CPP_H
#define LIBPRESSIO_PREDICT_SIMPLE_SCHEME_CPP_H

#include <libpressio_ext/cpp/pressio.h>
#include <libpressio_ext/cpp/configurable.h>
#include <libpressio_ext/cpp/versionable.h>
#include <libpressio_ext/cpp/compressor.h>
#include "scheme.h"
#include <set>

namespace libpressio { namespace predict {

struct metric_usage{
    std::string metric;
    std::string module;
    enum metric_type {
        feature,
        label
    } type = feature;
};

class libpressio_simple_predict_scheme_plugin : public libpressio_predict_scheme_plugin {


    std::vector<std::string> get_feature_ids() override final;
    std::vector<std::string> get_valid_feature_ids(pressio_compressor const& target_compressor, pressio_options const& invalidations) override final;
    std::vector<std::string> get_invalid_feature_ids(pressio_compressor const& target_compressor, pressio_options const& invalidations) override final;

    std::vector<std::string> get_label_ids() override final;
    std::vector<std::string> get_valid_label_ids(pressio_compressor const& target_compressor, pressio_options const& invalidations) override final;
    std::vector<std::string> get_invalid_label_ids(pressio_compressor const& target_compressor, pressio_options const& invalidations) override final;

    pressio_compressor get_metrics_collector(pressio_compressor &target_compressor, pressio_options const& invalidations) override final;
    virtual libpressio_predictor get_predictor(pressio_compressor const& compressor) override;

    virtual bool decompress_required(pressio_compressor const& target_compressor, pressio_options const& invalidations) override;
    virtual bool compress_required(pressio_compressor const& target_compressor, pressio_options const& invalidations) override;
    virtual bool training_required(pressio_compressor const& target_compressor) override;

    virtual std::unique_ptr<libpressio_predict_scheme_plugin> clone() const override=0;

    pressio_options get_configuration_impl() const override;

    char const* version() const override;
    private:
    virtual std::vector<metric_usage> metrics_to_modules() =0;
    virtual std::map<std::string,pressio_metrics&> module_build_override() {
        return {};
    }
    pressio_metrics build_metric_module(std::string const& module_id) {
        auto overrides = module_build_override();
        if(auto it = overrides.find(module_id); it != overrides.end()) {
            return it->second->clone();
        } else {
            return metrics_plugins().build(module_id);
        }
    }
    std::set<std::string> invalidated_metrics_types(pressio_compressor const& target_compressor, pressio_options const& invalidations) {
        std::set<std::string> invalid;
        auto settings = target_compressor->get_options();
        auto configuration = target_compressor->get_configuration();

        std::vector<std::string> all_settings;
        std::transform(settings.begin(), settings.end(), std::back_inserter(all_settings), [](auto i) {return i.first; });

        std::set<std::string> runtime, error_dependent, error_agnostic;
        auto get_invalidation_classes = [&](std::set<std::string> & out, const char* key) {
            std::vector<std::string> out_v;
            if(configuration.get(key, &out_v) == pressio_options_key_set) {
                out.insert(std::make_move_iterator(out_v.begin()), std::make_move_iterator(out_v.end())); 
            } else {
                out.insert(all_settings.begin(), all_settings.end());
            }
        };
        get_invalidation_classes(runtime, "predictors:runtime");
        get_invalidation_classes(error_dependent, "predictors:error_dependent");
        get_invalidation_classes(error_agnostic, "predictors:error_agnostic");

        invalid.emplace("predictors:nondeterministic"); //non deterministic metrics are always invalidated
        if(invalidations.key_status("predictors:data") == pressio_options_key_set) {
            invalid.emplace("predictors:data");
        }
        if(invalidations.key_status("predictors:all") == pressio_options_key_set) {
            invalid.emplace("predictors:runtime");
            invalid.emplace("predictors:error_dependent");
            invalid.emplace("predictors:error_agnostic");
            invalid.emplace("predictors:data");
        } else {
            for(auto const& [key, value]: invalidations) {
                if(runtime.contains(key)) invalid.emplace("predictors:runtime");
                if(error_dependent.contains(key)) invalid.emplace("predictors:error_dependent");
                if(error_agnostic.contains(key)) invalid.emplace("predictors:error_agnostic");
                invalid.emplace(key);
            }
        }
        return invalid;
    }
    struct cause {
        std::string module;
        std::set<std::string> causes;
        metric_usage::metric_type type;
        bool compress_required = true;
        bool decompress_required = true;
    };
    using module_name = std::string;
    using metric_name = std::string;
    std::map<module_name, cause> metric_invalidation_causes() {
        std::map<module_name, std::set<std::string>> modules_to_metrics;
        std::map<metric_name, cause> metric_to_causes;

        auto modules_map = metrics_to_modules();
        for(const auto& module: modules_map) {
            modules_to_metrics[module.module].emplace(module.metric);
            metric_to_causes[module.metric].module = module.module;
            metric_to_causes[module.metric].type = module.type;
        }

        for(const auto& module: modules_to_metrics) {
            auto module_plugin = build_metric_module(module.first);
            auto module_config = module_plugin->get_configuration();
            auto metrics_results = module_plugin->get_metrics_results({});

            auto check_requires = [&](const char* key, auto get_required) {
                bool all_requires = true; // if neither all_requires are present, assume they are required
                std::vector<std::string> metric_requires;
                if(module_config.get("predictors:requires_compress", &all_requires) == pressio_options_key_set) {
                    //intentional no-op, included to prevent the call to check for the vector<string> version
                } else if(module_config.get("predictors:requires_compress", & metric_requires) == pressio_options_key_set) {
                    all_requires = false;
                    std::set metric_requires_s(metric_requires.begin(), metric_requires.end());
                    for(auto const& m: module.second) {
                        get_required(metric_to_causes[m]) = metric_requires_s.contains(m);
                    }
                }
                if(all_requires) {
                    for(auto const& m: module.second) {
                        get_required(metric_to_causes[m]) = true;
                    }
                }
            };
            check_requires("predictors:requires_compress", [](auto& i) -> bool& {return i.compress_required;});
            check_requires("predictors:requires_decompress", [](auto& i) -> bool& {return i.decompress_required;});


            std::vector<std::string> vec;
            if(module_config.get("predictors:invalidate", &vec) == pressio_options_key_set) {
                for (const auto& metric: module.second) {
                    metric_to_causes[metric].causes.insert(vec.begin(), vec.end());
                }
            } else {
                std::map<std::string, std::set<std::string>> invalidation_causes;
                constexpr std::array<const char*, 5> cause_categories {"predictors:runtime", "predictors:error_dependent", "predictors:error_agnostic",
                    "predictors:data", "predictors:nondeterministic"};
                const bool any_cause = std::any_of(cause_categories.begin(), cause_categories.end(), [&](const char* cat){
                        return module_config.key_status(cat) == pressio_options_key_set;
                        });
                for (auto const &cause : cause_categories
                    ) {
                    if(module_config.get(cause, &vec) == pressio_options_key_set) {
                        for(auto const& metric: vec) {
                            if(module.second.contains(metric)) {
                                metric_to_causes[metric].causes.emplace(cause);
                            }
                        }
                    } else {
                        if(!any_cause) {
                            for(auto const& metric: module.second) {
                                metric_to_causes[metric].causes.emplace(cause);
                            }
                        }
                    }
                }
            }
        }
        return metric_to_causes;
    }

        struct invalidation_result {
            std::map<metric_name, cause> causes;
            std::set<metric_name> valid_metrics;
            std::set<metric_name> invalid_metrics;
            std::set<metric_name> all_metrics;
            std::set<module_name> invalidated_modules;
            bool is_training;
            bool collect_labels;
        };
        invalidation_result discover_invalidations(pressio_compressor const& target_compressor, pressio_options const& invalidations) {
            invalidation_result res;
            auto invalidation_types = invalidated_metrics_types(target_compressor, invalidations);
            res.causes = metric_invalidation_causes();
            res.is_training = invalidations.key_status("predictors:training") == pressio_options_key_set;
            res.collect_labels = invalidations.key_status("predictors:collect_labels") == pressio_options_key_set;
            for(auto const& [metric, cause]: res.causes) {
                res.all_metrics.emplace(metric);
                std::vector<std::string> intersection;
                std::set_intersection(invalidation_types.begin(), invalidation_types.end(),
                        cause.causes.begin(), cause.causes.end(), std::back_inserter(intersection)
                        );
                if ((cause.type == metric_usage::label)
                        ? (!intersection.empty() && res.is_training)
                        : (!intersection.empty())) {
                  res.invalidated_modules.emplace(cause.module);
                  res.invalid_metrics.emplace(metric);
                } else {
                  res.valid_metrics.emplace(metric);
                }
            }
            return res;
        }
};

}}

#endif
