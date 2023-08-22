#include "libpressio_predict_ext/cpp/predict.h"
#include <sstream>

namespace libpressio { namespace predict {
pressio_registry<std::unique_ptr<libpressio_predict_plugin>>& predictor_plugins() {
    static pressio_registry<std::unique_ptr<libpressio_predict_plugin>> registry;
    return registry;
}
pressio_registry<std::unique_ptr<libpressio_predict_quality_metrics_plugin>>& predictor_quality_metrics_plugins() {
    static pressio_registry<std::unique_ptr<libpressio_predict_quality_metrics_plugin>> registry;
    return registry;
}
pressio_options libpressio_predict_plugin::get_configuration() const {
  pressio_options ret;
  ret.copy_from(get_configuration_impl());
  set(ret, "pressio:version_epoch", epoch_version());
  set(ret, "pressio:version_major", major_version());
  set(ret, "pressio:version_minor", minor_version());
  set(ret, "pressio:version_patch", patch_version());
  set(ret, "pressio:children", children());
  set(ret, "pressio:prefix", prefix());
  set(ret, "pressio:version", version());
  set(ret, "pressio:type", type());
  return ret;
}

pressio_options libpressio_predict_quality_metrics_plugin::get_configuration() const {
  pressio_options ret;
  ret.copy_from(get_configuration_impl());
  set(ret, "pressio:version_epoch", epoch_version());
  set(ret, "pressio:version_major", major_version());
  set(ret, "pressio:version_minor", minor_version());
  set(ret, "pressio:version_patch", patch_version());
  set(ret, "pressio:children", children());
  set(ret, "pressio:prefix", prefix());
  set(ret, "pressio:version", version());
  set(ret, "pressio:type", type());
  return ret;
}


const char* libpressio_predict_plugin::version() const {
    static std::string version_str ([this]{
        std::stringstream ss;
        ss << major_version() << '.'
           << minor_version() << '.'
           << patch_version();
        return ss.str();
    }());
    return version_str.c_str();
}

const char* libpressio_predict_quality_metrics_plugin::version() const {
    static std::string version_str ([this]{
        std::stringstream ss;
        ss << major_version() << '.'
           << minor_version() << '.'
           << patch_version();
        return ss.str();
    }());
    return version_str.c_str();
}

pressio_options libpressio_predict_plugin::get_configuration_impl() const  {
    return {};
};
pressio_options libpressio_predict_plugin::get_metrics_results() const {
    return {};
};
pressio_options libpressio_predict_quality_metrics_plugin::get_configuration_impl() const  {
    return {};
};
pressio_options libpressio_predict_quality_metrics_plugin::get_metrics_results() const {
    return {};
};
void libpressio_predict_quality_metrics_plugin::clear() {
};


class libpressio_noop_predict final : public libpressio_predict_plugin {

    std::unique_ptr<libpressio_predict_plugin> clone() const override {
        return std::make_unique<libpressio_noop_predict>(*this);
    }
    int fit_impl(pressio_data const&, pressio_data const&) override {
        /* we don't do any training here, so just return immediately */
        return 0;
    }
    int predict_impl(pressio_data const& input_data, pressio_data& out) const override {
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

class libpressio_medape_predict_quality_metric final : public libpressio_predict_quality_metrics_plugin {

    std::unique_ptr<libpressio_predict_quality_metrics_plugin> clone() const override {
        return std::make_unique<libpressio_medape_predict_quality_metric>(*this);
    }

    int score_impl(pressio_data const& predicted, pressio_data const& actual) override {
        try {
            if(predicted.dimensions().at(0) == 1) return set_error(1, "too many predicted labels");
            if(actual.dimensions().at(0) == 1) return set_error(1, "too many actual labels");
            if(predicted.dimensions().at(1) != actual.dimensions().at(1)) {
                return set_error(1, "different numbers of observations predicted vs actual");
            }
            size_t N = predicted.dimensions().at(1);
            auto pred_f64 = predicted.cast(pressio_double_dtype);
            auto actual_f64 = actual.cast(pressio_double_dtype);
            double* pred_f64_ptr = static_cast<double*>(pred_f64.data());
            double* actual_f64_ptr = static_cast<double*>(actual_f64.data());

            std::vector<double> ape(N);
            for (size_t i = 0; i < N; ++i) {
                if(actual_f64_ptr[i] == 0) {
                    ape[i] = 0;
                } else {
                    ape[i] = std::abs(actual_f64_ptr[i] - pred_f64_ptr[i])/actual_f64_ptr[i];
                }
            }
            medapes.push_back(summary(ape).median);
            apes.insert(apes.end(), ape.begin(), ape.end());

        } catch(std::exception const& ex) {
            return set_error(1, std::string("exception in medape: ") + ex.what());
        }

        return 0;
    }

    pressio_options get_metrics_results() const override {
        pressio_options opts;
        auto folds = summary(medapes);
        auto overall = summary(apes);
        set(opts, "medape:folds_10ape", folds.lower_10);
        set(opts, "medape:folds_90ape", folds.upper_10);
        set(opts, "medape:folds_median", folds.median);
        set(opts, "medape:overall_10ape", overall.lower_10);
        set(opts, "medape:overall_90ape", overall.upper_10);
        set(opts, "medape:overall_median", overall.median);
        set(opts, "medape:apes", pressio_data(apes.begin(), apes.end()));
        set(opts, "medape:medapes", pressio_data(medapes.begin(), medapes.end()));
        return opts;
    }

    const char* prefix() const override {
        return "medape";
    }

    void clear() override {
        medapes.clear();
        apes.clear();
    }

    struct summary_t {
        double lower_10;
        double upper_10;
        double median;
    };
    static summary_t summary(std::vector<double> d) {
        std::sort(d.begin(), d.end());
        size_t N = d.size();
        summary_t out;
        
        out.lower_10 = d[size_t(N*.1)];
        out.upper_10 = d[size_t(N*.9)];
        if(N % 2 == 0) {
             out.median = (d[N/2] + d[N/2+1])/2.0;
        } else {
            out.median =  d[N/2];
        }
        return out;
    }
    std::vector<double> medapes;
    std::vector<double> apes;

};


} }
