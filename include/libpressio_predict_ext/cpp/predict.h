#ifndef LIBPRESSIO_PREDICT_CPP_H
#define LIBPRESSIO_PREDICT_CPP_H

#include <libpressio_ext/cpp/pressio.h>
#include <libpressio_ext/cpp/configurable.h>

namespace libpressio { namespace predict {

class libpressio_predict_plugin : public pressio_configurable, pressio_versionable {
    public:
    int fit(pressio_data const& features, pressio_data const& labels) {
        //TODO implement add hooks to implement timing metrics
        return fit_impl(features, labels);
    }
    int predict(pressio_data const& features, pressio_data& predicted_labels) const {
        //TODO implement add hooks to implement timing metrics
        return predict_impl(features, predicted_labels);
    }
    std::string type() const {
        return "predict";
    }
    pressio_options get_configuration() const final;


    virtual std::unique_ptr<libpressio_predict_plugin> clone() const=0;
    virtual int fit_impl(pressio_data const& features, pressio_data const& labels) = 0;
    virtual int predict_impl(pressio_data const& features, pressio_data& predicted_labels) const = 0;

    virtual pressio_options get_configuration_impl() const;
    virtual pressio_options get_metrics_results() const;
    virtual char const* version() const;
};

class libpressio_predict_quality_metrics_plugin : public pressio_configurable, pressio_versionable {
    public:
    int score(pressio_data const& features, pressio_data const& labels) {
        //TODO implement add hooks to implement timing metrics
        return score_impl(features, labels);
    }
    std::string type() const final {
        return "predict_quality_metrics";
    }
    pressio_options get_configuration() const final;


    virtual std::unique_ptr<libpressio_predict_quality_metrics_plugin> clone() const=0;
    virtual int score_impl(pressio_data const& features, pressio_data const& labels) = 0;
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

pressio_registry<std::unique_ptr<libpressio_predict_quality_metrics_plugin>>& predictor_quality_metrics_plugins();
pressio_registry<std::unique_ptr<libpressio_predict_plugin>>& predictor_plugins();

} }


#endif /* end of include guard: LIBPRESSIO_PREDICT_CPP_H */
