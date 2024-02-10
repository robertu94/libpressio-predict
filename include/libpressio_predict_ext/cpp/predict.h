#ifndef LIBPRESSIO_PREDICT_CPP_H
#define LIBPRESSIO_PREDICT_CPP_H

#include <set>
#include <libpressio_ext/cpp/pressio.h>
#include <libpressio_ext/cpp/configurable.h>

namespace libpressio { namespace predict {

class libpressio_predict_plugin : public pressio_configurable, pressio_versionable {
    public:
    int fit(pressio_data const& features, pressio_data const& labels) {
        //TODO implement add hooks to implement timing metrics
        return fit_impl(features, labels);
    }
    int predict(pressio_data const& features, pressio_data& predicted_labels) {
        //TODO implement add hooks to implement timing metrics
        return predict_impl(features, predicted_labels);
    }
    std::string type() const {
        return "predict";
    }
    pressio_options get_configuration() const final;


    virtual std::unique_ptr<libpressio_predict_plugin> clone() const=0;
    virtual int fit_impl(pressio_data const& features, pressio_data const& labels) = 0;
    virtual int predict_impl(pressio_data const& features, pressio_data& predicted_labels) = 0;

    virtual pressio_options get_configuration_impl() const;
    virtual pressio_options get_metrics_results() const;
    virtual char const* version() const;
};

class libpressio_predict_quality_metrics_plugin : public pressio_configurable, pressio_versionable {
    public:
    int score(pressio_data const& actual, pressio_data const& predicted) {
        //TODO implement add hooks to implement timing metrics
        return score_impl(actual, predicted);
    }
    std::string type() const final {
        return "predict_quality_metrics";
    }
    pressio_options get_configuration() const final;


    virtual std::unique_ptr<libpressio_predict_quality_metrics_plugin> clone() const=0;
    virtual int score_impl(pressio_data const& actual, pressio_data const& predicted) = 0;
    virtual pressio_options get_metrics_results() const =0;

    virtual pressio_options get_configuration_impl() const;
    virtual char const* version() const;
    virtual void clear();
};

class libpressio_predictor_metrics {

    public:
    libpressio_predictor_metrics(std::unique_ptr<libpressio_predict_quality_metrics_plugin>&& rhs): ptr(std::move(rhs)) {}
    libpressio_predictor_metrics()=default;
    libpressio_predictor_metrics(libpressio_predictor_metrics const& rhs) : ptr(rhs.ptr->clone()) {}
    libpressio_predictor_metrics(libpressio_predictor_metrics && rhs) : ptr(std::move(rhs.ptr)) {}
    libpressio_predictor_metrics& operator=(libpressio_predictor_metrics&& rhs) {
        if(&rhs == this) return *this;
        ptr = std::move(rhs.ptr);
        return *this;
    }
    libpressio_predictor_metrics& operator=(libpressio_predictor_metrics const& rhs) {
        if(&rhs == this) return *this;
        ptr = rhs.ptr->clone();
        return *this;
    }

    operator bool() {
        return (bool)ptr;
    }

    libpressio_predict_quality_metrics_plugin* operator->() {
        return ptr.operator->();
    }

    libpressio_predict_quality_metrics_plugin& operator*() {
        return *ptr;
    }

    libpressio_predict_quality_metrics_plugin* get() {
        return ptr.get();
    }

    private:
    std::unique_ptr<libpressio_predict_quality_metrics_plugin> ptr;
};

class libpressio_predictor {

    public:
    libpressio_predictor(std::unique_ptr<libpressio_predict_plugin>&& rhs): ptr(std::move(rhs)) {}
    libpressio_predictor()=default;
    libpressio_predictor(libpressio_predictor const& rhs) : ptr(rhs.ptr->clone()) {}
    libpressio_predictor(libpressio_predictor && rhs) : ptr(std::move(rhs.ptr)) {}
    libpressio_predictor& operator=(libpressio_predictor&& rhs) {
        if(&rhs == this) return *this;
        ptr = std::move(rhs.ptr);
        return *this;
    }
    libpressio_predictor& operator=(libpressio_predictor const& rhs) {
        if(&rhs == this) return *this;
        ptr = rhs.ptr->clone();
        return *this;
    }

    operator bool() {
        return (bool)ptr;
    }

    libpressio_predict_plugin* operator->() {
        return ptr.operator->();
    }

    libpressio_predict_plugin& operator*() {
        return *ptr;
    }

    libpressio_predict_plugin* get() {
        return ptr.get();
    }

    private:
    std::unique_ptr<libpressio_predict_plugin> ptr;
};

class libpressio_predict_scheme_plugin : public pressio_configurable, public pressio_versionable {
    public:
    std::string type() const final {
        return "predict_scheme";
    }
    pressio_options get_configuration() const final;

    virtual std::vector<std::string> req_metrics()=0;
    virtual pressio_compressor req_metrics_opts(pressio_compressor& comp, std::vector<std::string> const& invalidations)=0;
    virtual libpressio_predictor get_predictor(pressio_compressor const& compressor)=0;
    virtual bool do_decompress(pressio_compressor const& compressor, std::vector<std::string> const& invalidations)=0;
    virtual bool do_compress(pressio_compressor const& compressor, std::vector<std::string> const& invalidations)=0;

    virtual pressio_options get_configuration_impl() const=0;
    virtual char const* version() const=0;
    virtual std::unique_ptr<libpressio_predict_scheme_plugin> clone() const=0;
};

struct metric_usage{
    std::string metric;
    std::string module;
    bool training_only;
};

class libpressio_simple_predict_scheme_plugin : public libpressio_predict_scheme_plugin {

    virtual std::vector<metric_usage> metrics_to_modules() =0;

    std::vector<std::string> req_metrics() override final;
    pressio_compressor req_metrics_opts(pressio_compressor &target_compressor, std::vector<std::string> const& invalidations) final;
    virtual libpressio_predictor get_predictor(pressio_compressor const& compressor) override;

    virtual bool do_decompress(pressio_compressor const& target_compressor, std::vector<std::string> const& invalidations) override;
    virtual bool do_compress(pressio_compressor const& target_compressor, std::vector<std::string> const& invalidations) override;

    virtual std::unique_ptr<libpressio_predict_scheme_plugin> clone() const override=0;
    pressio_options get_configuration_impl() const final;
    char const* version() const override;
    private:
    bool requires_compressor(pressio_compressor const& target_compressor, std::vector<std::string> const& invalidations, const char* key) {
        pressio library;
        std::set<std::string> checked;
        auto const& metrics_info = metrics_to_modules();
        pressio_options target_options = target_compressor->get_options();
        for (auto const& metric_info : metrics_info) {
            if (checked.find(metric_info.module) != checked.end()) continue;

            pressio_metrics metric = library.get_metric(metric_info.module);
            metric->set_options(target_options);
            pressio_options metric_config = metric->get_configuration();

            bool requires_decompress;
            std::vector<std::string> metrics_that_require_decompress;
            if(metric_config.get("predictors:requires_decompress", &requires_decompress) == pressio_options_key_set) {
                if(requires_decompress) return true;
            }
            if(metric_config.get("predictors:requires_decompress", &metrics_that_require_decompress) == pressio_options_key_set) {
                if(std::find(metrics_that_require_decompress.begin(), metrics_that_require_decompress.end(), metric_info.metric) != metrics_that_require_decompress.end()) return true;
            }
        }
        return false;
    }
};



class libpressio_scheme {

    public:
    libpressio_scheme(std::unique_ptr<libpressio_predict_scheme_plugin>&& rhs): ptr(std::move(rhs)) {}
    libpressio_scheme()=default;
    libpressio_scheme(libpressio_scheme const& rhs) : ptr(rhs.ptr->clone()) {}
    libpressio_scheme(libpressio_scheme && rhs) : ptr(std::move(rhs.ptr)) {}
    libpressio_scheme& operator=(libpressio_scheme&& rhs) {
        if(&rhs == this) return *this;
        ptr = std::move(rhs.ptr);
        return *this;
    }
    libpressio_scheme& operator=(libpressio_scheme const& rhs) {
        if(&rhs == this) return *this;
        ptr = rhs.ptr->clone();
        return *this;
    }

    operator bool() {
        return (bool)ptr;
    }

    libpressio_predict_scheme_plugin* operator->() const {
        return ptr.operator->();
    }

    libpressio_predict_scheme_plugin& operator*() const {
        return *ptr;
    }

    libpressio_predict_scheme_plugin* get() const {
        return ptr.get();
    }

    private:
    std::unique_ptr<libpressio_predict_scheme_plugin> ptr;
};

pressio_registry<std::unique_ptr<libpressio_predict_quality_metrics_plugin>>& predictor_quality_metrics_plugins();
pressio_registry<std::unique_ptr<libpressio_predict_plugin>>& predictor_plugins();
pressio_registry<std::unique_ptr<libpressio_predict_scheme_plugin>>& scheme_plugins();

} }


#endif /* end of include guard: LIBPRESSIO_PREDICT_CPP_H */
