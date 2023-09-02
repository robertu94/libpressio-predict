#include <libpressio_ext/cpp/data.h>
#include <libpressio_predict_ext/cpp/predict.h>
#include <std_compat/memory.h>
#include <cmath>

namespace libpressio { namespace predict {

class libpressio_medape_predict_quality_metric final : public libpressio_predict_quality_metrics_plugin {

    std::unique_ptr<libpressio_predict_quality_metrics_plugin> clone() const override {
        return std::make_unique<libpressio_medape_predict_quality_metric>(*this);
    }

    int score_impl(pressio_data const& actual, pressio_data const& predicted) override {
        try {
            size_t N;
            if(actual.num_dimensions() == 1) {
                if(predicted.dimensions() != actual.dimensions()) {
                    return set_error(1, "predicted and actual have different dimensions");
                }
                N = predicted.dimensions().at(0);
            } else if(actual.num_dimensions() == 2) {
                if(predicted.dimensions() != actual.dimensions()) {
                    return set_error(1, "predicted and actual have different dimensions");
                }
                if(predicted.dimensions().at(0) != 1) {
                    return set_error(1, "predicted and actual have too many labels");
                }
                N = predicted.dimensions().at(1);
            } else {
                return set_error(1, "too many dimension on the actual values");
            }

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
static pressio_register compressor_many_fields_plugin(predictor_quality_metrics_plugins(), "medape", []() {
  return compat::make_unique<libpressio_medape_predict_quality_metric>();
});

} }
