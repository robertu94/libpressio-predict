#ifndef LIBPRESSIO_PREDICT_SCHEME_CPP_H
#define LIBPRESSIO_PREDICT_SCHEME_CPP_H

#include <libpressio_ext/cpp/pressio.h>
#include <libpressio_ext/cpp/configurable.h>
#include <libpressio_ext/cpp/versionable.h>
#include <libpressio_ext/cpp/compressor.h>
#include "predict.h"

namespace libpressio { namespace predict {

class libpressio_predict_scheme_plugin : public pressio_configurable, public pressio_versionable {
    public:
    std::string type() const final {
        return "predict_scheme";
    }
    pressio_options get_configuration() const final;

    /**
     * \param[in] comp the metrics collector or compressor to use
     * \param[in] invalidations the list of compressor settings who's values have changed, Initially provide all settings used with the compressor
     * \param[in] input the input dataset
     * \param[in] compressed the compressed dataset
     * \param[in] output the compressed dataset
     * \param[in] cache if a metric is still valid, retrieve it from this options structure
     * \return a row vector suitable for passing to predictor->predict()
     */
    std::pair<pressio_data,pressio_data> get_features_and_labels(pressio_compressor& comp, pressio_options const& invalidations, pressio_data& input, pressio_data& compressed, pressio_data& output,  pressio_options const& cache);

    /**
     * \param[in] comp the metrics collector or compressor to use; assumes the metrics have already been collected
     * \param[in] invalidations the list of compressor settings who's values have changed, Initially provide all settings used with the compressor
     * \param[in] cache if a metric is still valid, retrieve it from this options structure
     * \return a row vector suitable for passing to predictor->predict()
     */
    std::pair<pressio_data,pressio_data> get_features_and_labels(pressio_compressor const& comp, pressio_options const& invalidations, pressio_options const& cache);

    /**
     * \param[in] comp the metrics collector or compressor to use
     * \param[in] invalidations the list of compressor settings who's values have changed, Initially provide all settings used with the compressor
     * \param[in] input input the input dataset
     * \param[in] compressed input the compressed dataset
     * \param[in] output input the compressed dataset
     * \param[in] cache if a metric is still valid, retrieve it from this options structure
     * \return a row vector suitable for passing to predictor->predict()
     */
    pressio_data get_labels(pressio_compressor& comp, pressio_options const& invalidations, pressio_data& input, pressio_data& compressed, pressio_data& output,  pressio_options const& cache);

    /**
     * \param[in] comp the metrics collector or compressor to use; assumes the metrics have already been collected
     * \param[in] invalidations the list of compressor settings who's values have changed, Initially provide all settings used with the compressor
     * \param[in] cache if a metric is still valid, retrieve it from this options structure
     * \return a row vector suitable for passing to predictor->predict()
     */
    pressio_data get_labels(pressio_compressor const& comp, pressio_options const& invalidations, pressio_options const& cache);

    /**
     * \param[in] comp the metrics collector or compressor to use
     * \param[in] invalidations the list of compressor settings who's values have changed, Initially provide all settings used with the compressor
     * \param[in] input input the input dataset
     * \param[in] compressed input the compressed dataset
     * \param[in] output input the compressed dataset
     * \param[in] cache if a metric is still valid, retrieve it from this options structure
     * \return a row vector suitable for passing to predictor->predict()
     */
    pressio_data get_features(pressio_compressor& comp, pressio_options const& invalidations, pressio_data& input, pressio_data& compressed, pressio_data& output,  pressio_options const& cache);

    /**
     * \param[in] comp the metrics collector or compressor to use; assumes the metrics have already been collected
     * \param[in] invalidations the list of compressor settings who's values have changed, Initially provide all settings used with the compressor
     * \param[in] cache if a metric is still valid, retrieve it from this options structure
     * \return a row vector suitable for passing to predictor->predict()
     */
    pressio_data get_features(pressio_compressor const& comp, pressio_options const& invalidations, pressio_options const& cache);
    /**
     * Get a list of all metrics IDs features that need to be passed to the predictor
     */
    virtual std::vector<std::string> get_feature_ids()=0;
    /**
     * Returns the metrics IDs for the features that can be re-used from a previous run
     */
    virtual std::vector<std::string> get_valid_feature_ids(pressio_compressor const& comp, pressio_options const& invalidations)=0;
    /**
     * Returns the metrics IDs for the features that need to be re-computed with get_metrics_collector
     */
    virtual std::vector<std::string> get_invalid_feature_ids(pressio_compressor const& comp, pressio_options const& invalidations)=0;
    /**
     * Get a list of all metrics IDs labels that need to be passed to the predictor
     */
    virtual std::vector<std::string> get_label_ids()=0;
    /**
     * Returns the metrics IDs for the labels that can be re-used from a previous run
     */
    virtual std::vector<std::string> get_valid_label_ids(pressio_compressor const& comp, pressio_options const& invalidations)=0;
    /**
     * Returns the metrics IDs for the labels that need to be re-computed with get_metrics_collector
     */
    virtual std::vector<std::string> get_invalid_label_ids(pressio_compressor const& comp, pressio_options const& invalidations)=0;
    /**
     * \param[in] comp the compressor to be predicted
     * \param[in] invalidations the set of compressor settings who's values have changed.  Initially provide all settings used with the compressor
     * \returns a "compressor" object capable of computing the list of options
     * that need to be passed to the compressor to efficiently compute the required metrics
     */
    virtual pressio_compressor get_metrics_collector(pressio_compressor& comp, pressio_options const& invalidations)=0;
    /**
     * \param[in] compressor what compressor is the scheme being used with?
     * \returns a predictor class suitable for use with this prediction scheme
     */
    virtual libpressio_predictor get_predictor(pressio_compressor const& compressor)=0;
    /**
     * Implementing plugins SHOULD override this function to indication if compression is required
     *
     * \param[in] compressor what compressor is the scheme being used with?
     * \param[in] invalidations set of compressor settings who's values have changed.  Initially provide all settings provided to the compressor.
     * \returns true if decompression must be called to compute the metrics needed for this scheme
     */
    virtual bool decompress_required(pressio_compressor const& compressor, pressio_options const& invalidations)=0;
    /**
     * Implementing plugins SHOULD override this function to indication if compression is required
     *
     * \param[in] compressor what compressor is the scheme being used with?
     * \param[in] invalidations set of compressor settings who's values have changed.  Initially provide all settings provided to the compressor.
     * \returns true if decompression must be called to compute the metrics needed for this scheme
     */
    virtual bool compress_required(pressio_compressor const& compressor, pressio_options const& invalidations)=0;

    /**
     * Implementing plugins SHOULD override this function to indication if training is required
     *
     * \param[in] compressor what compressor is the scheme being used with?
     * \returns true if training should be preformed to use this scheme
     */
    virtual bool training_required(pressio_compressor const& compressor)=0;

    

    /**
     * Implementing plugins SHOULD override this function to provide configuration values
     *
     * \returns configuration options known at compile time
     */
    virtual pressio_options get_configuration_impl() const=0;
    /**
     * A human readable version string
     */
    virtual char const* version() const=0;
    /**
     * A clone function capable of returning an independent copy of this subtree
     */
    virtual std::unique_ptr<libpressio_predict_scheme_plugin> clone() const=0;

    private:
    pressio_compressor run_collector(pressio_compressor& comp, pressio_options const& invalidations, pressio_data& input, pressio_data& compressed, pressio_data& output);
    void copy_metric_results(pressio_data& output, pressio_options const& metric_results, std::vector<std::string> const& ids, pressio_options const& metrics_results_cache) const;
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

pressio_registry<std::unique_ptr<libpressio_predict_scheme_plugin>>& scheme_plugins();

}}


#endif
