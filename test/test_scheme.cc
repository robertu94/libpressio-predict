#include "libpressio_predict_ext/cpp/predict.h"
#include "libpressio_predict_ext/cpp/scheme.h"
#include "libpressio_predict_ext/cpp/simple_scheme.h"
#include <cmath>
#include <set>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <algorithm>

using namespace libpressio::predict;
using namespace std::string_literals;

#include "std_compat/memory.h"
#include "libpressio_ext/cpp/compressor.h"
#include "libpressio_ext/cpp/data.h"
#include "libpressio_ext/cpp/options.h"
#include "libpressio_ext/cpp/pressio.h"

template <class T>
struct format_range {
    T const& rng;
};
template <class CharT, class Traits, class T>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& out, format_range<T> const& range) {
    out << '(';
    auto beg = std::begin(range.rng);
    auto end = std::end(range.rng);
    while(beg != end) {
        out << std::quoted(*beg) << ", ";
        ++beg;
    }
    return out << ')';
}

template <class Rng1, class Rng2>
std::pair<bool, std::vector<typename Rng1::value_type>> set_not_equals(Rng1 actual, Rng2 expected) {
    std::set lhs_s(actual.begin(), actual.end());
    std::set rhs_s(expected.begin(), expected.end());
    std::vector<typename Rng1::value_type> intersection;
    std::vector<typename Rng1::value_type> differences;
    std::set_intersection(lhs_s.begin(), lhs_s.end(),
            rhs_s.begin(), rhs_s.end(), std::back_inserter(intersection));
    std::set_difference(lhs_s.begin(), lhs_s.end(),
            rhs_s.begin(), rhs_s.end(), std::back_inserter(differences));
    return std::make_pair(
            intersection.size() != std::max(actual.size(), expected.size()),
            differences
            );
}

namespace libpressio { namespace mock_compressor_ns {

class libpressio_mock_predictor_predict final : public libpressio_predict_plugin {

    std::unique_ptr<libpressio_predict_plugin> clone() const override {
        return std::make_unique<libpressio_mock_predictor_predict>(*this);
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
        return "mock_predictor";
    }

    bool training_required() const override {
        return true;
    }
};

static pressio_register compressor_many_fields_plugin(predictor_plugins(), "mock_predictor", []() {
  return compat::make_unique<libpressio_mock_predictor_predict>();
});


class mock_compressor_compressor_plugin : public libpressio_compressor_plugin {
public:
  struct pressio_options get_options_impl() const override
  {
    struct pressio_options options;
    set(options, "mock_compressor:error_agnostic", a);
    set(options, "mock_compressor:error_dependent", b);
    set(options, "mock_compressor:runtime", c);
    return options;
  }

  struct pressio_options get_configuration_impl() const override
  {
    struct pressio_options options;
    set(options, "pressio:thread_safe", pressio_thread_safety_multiple);
    set(options, "pressio:stability", "experimental");
    set(options, "predictors:error_agnostic", std::vector<std::string>{"mock_compressor:error_agnostic"});
    set(options, "predictors:error_dependent", std::vector<std::string>{"mock_compressor:error_dependent"});
    set(options, "predictors:runtime", std::vector<std::string>{"mock_compressor:runtime"});
    return options;
  }

  struct pressio_options get_documentation_impl() const override
  {
    struct pressio_options options;
    set(options, "pressio:description", R"(a fake compressor intended for testing)");
    set(options, "mock_compressor:error_agnostic", R"(a setting that does not effect errors)");
    set(options, "mock_compressor:error_dependent", R"(a setting that does effect errors)");
    set(options, "mock_compressor:runtime", R"(a setting that only effects runtime)");

    set(options, "mock_compressor:compress_calls", R"(number of calls to compress)");
    set(options, "mock_compressor:decompress_calls", R"(number of calls to decompress)");
    return options;
  }


  int set_options_impl(struct pressio_options const& options) override
  {
    get(options, "mock_compressor:error_agnostic", &a);
    get(options, "mock_compressor:error_dependent", &b);
    get(options, "mock_compressor:runtime", &c);
    return 0;
  }

  int compress_impl(const pressio_data* input,
                    struct pressio_data* output) override
  {
      calls.compress += 1;
      *output = *input;
      return 0;
  }

  int decompress_impl(const pressio_data* input,
                      struct pressio_data* output) override
  {
      calls.decompress += 1;
      *output = *input;
      return 0;
  }

  int major_version() const override { return 0; }
  int minor_version() const override { return 0; }
  int patch_version() const override { return 1; }
  const char* version() const override { return "0.0.1"; }
  const char* prefix() const override { return "mock_compressor"; }

  struct {
      int compress = 0;
      int decompress = 0;
  } calls;
  double a=1.0,b=2.0,c=3.0;

  pressio_options get_metrics_results_impl() const override {
    return {
        {"mock_compressor:compress_calls", calls.compress},
        {"mock_compressor:decompress_calls", calls.decompress},
    };
  }

  std::shared_ptr<libpressio_compressor_plugin> clone() override
  {
    return compat::make_unique<mock_compressor_compressor_plugin>(*this);
  }

};

static pressio_register mock_compressor_register(compressor_plugins(), "mock_compressor", []() {
  return compat::make_unique<mock_compressor_compressor_plugin>();
});

} }

#include "pressio_data.h"
#include "pressio_compressor.h"
#include "pressio_options.h"
#include "libpressio_ext/cpp/metrics.h"
#include "libpressio_ext/cpp/pressio.h"
#include "libpressio_ext/cpp/options.h"
#include "std_compat/memory.h"

namespace libpressio { namespace mock_metrics_ns {

class mock_data_plugin : public libpressio_metrics_plugin {
  public:
    int end_compress_impl(struct pressio_data const* input, pressio_data const* output, int) override {
      calls.end_compress += 1;
      return 0;
    }

    int end_decompress_impl(struct pressio_data const* , pressio_data const* output, int) override {
      calls.end_decompress += 1;
      return 0;
    }

    int end_compress_many_impl(compat::span<const pressio_data* const> const& inputs,
                                   compat::span<const pressio_data* const> const& outputs, int ) override {
      calls.end_compress_many += 1;
      return 0;
  }

  int end_decompress_many_impl(compat::span<const pressio_data* const> const& ,
                                   compat::span<const pressio_data* const> const& outputs, int ) override {
      calls.end_decompress_many += 1;
    return 0;
  }

  
  struct pressio_options get_configuration_impl() const override {
    pressio_options opts;
    set(opts, "pressio:stability", "stable");
    set(opts, "pressio:thread_safe", pressio_thread_safety_multiple);
    set(opts, "predictors:requires_decompress", std::vector<std::string>{});
    set(opts, "predictors:error_agnostic", std::vector<std::string>{"mock_data:error_agnostic"});
    set(opts, "predictors:data",  std::vector<std::string>{"mock_data:data"});
    return opts;
  }

  struct pressio_options get_documentation_impl() const override {
    pressio_options opt;
    set(opt, "pressio:description", "a mock prediction feature implemented as a metric");
    set(opt, "mock_data:error_agnostic", "a metric that is agnostic to error bound related settings");
    set(opt, "mock_data:data", "a metric that only depends on the the data");
    set(opt, "mock_data:end_compress_calls", "number of calls to end_compress");
    set(opt, "mock_data:end_decompress_calls", "number of calls to end_decompress");
    set(opt, "mock_data:end_compress_many_calls", "number of calls to end_compress_many");
    set(opt, "mock_data:end_decompress_many_calls", "number of calls to end_decompress_many");
    return opt;
  }

  pressio_options get_metrics_results(pressio_options const &) override {
    pressio_options opt;
    set(opt, "mock_data:error_agnostic", 3.0);
    set(opt, "mock_data:data", 4.0);
    set(opt, "mock_data:end_compress_calls", calls.end_compress);
    set(opt, "mock_data:end_decompress_calls", calls.end_decompress);
    set(opt, "mock_data:end_compress_many_calls", calls.end_compress_many);
    set(opt, "mock_data:end_decompress_many_calls", calls.end_decompress_many);
    return opt;
  }

  std::unique_ptr<libpressio_metrics_plugin> clone() override {
    return compat::make_unique<mock_data_plugin>(*this);
  }
  const char* prefix() const override {
    return "mock_data";
  }

  private:
    struct {
        int end_compress = 0;
        int end_decompress = 0;
        int end_compress_many = 0;
        int end_decompress_many = 0;
    } calls;

};
static pressio_register metrics_mock_data_plugin(metrics_plugins(), "mock_data", [](){ return compat::make_unique<mock_data_plugin>(); });


class mock_plugin : public libpressio_metrics_plugin {
  public:
    int end_compress_impl(struct pressio_data const* input, pressio_data const* output, int) override {
      calls.end_compress += 1;
      return 0;
    }

    int end_decompress_impl(struct pressio_data const* , pressio_data const* output, int) override {
      calls.end_decompress += 1;
      return 0;
    }

    int end_compress_many_impl(compat::span<const pressio_data* const> const& inputs,
                                   compat::span<const pressio_data* const> const& outputs, int ) override {
      calls.end_compress_many += 1;
      return 0;
  }

  int end_decompress_many_impl(compat::span<const pressio_data* const> const& ,
                                   compat::span<const pressio_data* const> const& outputs, int ) override {
      calls.end_decompress_many += 1;
    return 0;
  }

  
  struct pressio_options get_configuration_impl() const override {
    pressio_options opts;
    set(opts, "pressio:stability", "stable");
    set(opts, "pressio:thread_safe", pressio_thread_safety_multiple);
    set(opts, "predictors:requires_decompress", std::vector<std::string>{"mock:error_dependent", "mock:nondeterministic"});
    set(opts, "predictors:runtime", std::vector<std::string>{"mock:runtime"});
    set(opts, "predictors:error_dependent", std::vector<std::string>{"mock:error_dependent"});
    set(opts, "predictors:error_agnostic", std::vector<std::string>{"mock:error_agnostic"});
    set(opts, "predictors:data",  std::vector<std::string>{"mock:data"});
    set(opts, "predictors:nondeterministic",  std::vector<std::string>{"mock:nondeterministic"});
    return opts;
  }

  struct pressio_options get_documentation_impl() const override {
    pressio_options opt;
    set(opt, "pressio:description", "a mock prediction feature implemented as a metric");
    set(opt, "mock:end_compress_calls", "number of calls to end_compress");
    set(opt, "mock:end_decompress_calls", "number of calls to end_decompress");
    set(opt, "mock:end_compress_many_calls", "number of calls to end_compress_many");
    set(opt, "mock:end_decompress_many_calls", "number of calls to end_decompress_many");
    return opt;
  }

  pressio_options get_metrics_results(pressio_options const &) override {
    pressio_options opt;
    set(opt, "mock:nondeterministic", 0.0);
    set(opt, "mock:runtime", 1.0);
    set(opt, "mock:error_dependent", 2.0);
    set(opt, "mock:error_agnostic", 3.0);
    set(opt, "mock:data", 4.0);
    set(opt, "mock:end_compress_calls", calls.end_compress);
    set(opt, "mock:end_decompress_calls", calls.end_decompress);
    set(opt, "mock:end_compress_many_calls", calls.end_compress_many);
    set(opt, "mock:end_decompress_many_calls", calls.end_decompress_many);
    return opt;
  }

  std::unique_ptr<libpressio_metrics_plugin> clone() override {
    return compat::make_unique<mock_plugin>(*this);
  }
  const char* prefix() const override {
    return "mock";
  }

  private:
    struct {
        int end_compress = 0;
        int end_decompress = 0;
        int end_compress_many = 0;
        int end_decompress_many = 0;
    } calls;

};

static pressio_register metrics_mock_plugin(metrics_plugins(), "mock", [](){ return compat::make_unique<mock_plugin>(); });
}}


class mock_predict_scheme_plugin : public libpressio_simple_predict_scheme_plugin {
    public:
    std::vector<metric_usage> metrics_to_modules() override {
        return {
            {"mock:nondeterministic", "mock"},
            {"mock:error_dependent", "mock"},
            {"mock_data:error_agnostic", "mock_data"},
            {"mock_data:data", "mock_data"},
            {"size:compression_ratio", "size", metric_usage::label},
        };
    }

    libpressio_predictor get_predictor(pressio_compressor const&) override {
        return predictor_plugins().build("mock_predictor");
    }

    const char* prefix() const final {
        return "mock";
    }
    std::unique_ptr<libpressio_predict_scheme_plugin> clone() const override  {
        return compat::make_unique<mock_predict_scheme_plugin>(*this);
    }
};

static pressio_register scheme_mock_plugin(scheme_plugins(), "mock", [](){ return compat::make_unique<mock_predict_scheme_plugin>(); });

int main(int argc, char *argv[])
{
    auto input = pressio_data::owning(pressio_float_dtype, {100, 100});
    auto input_ptr = static_cast<float*>(input.data());
    auto const stride = input.get_dimension(1);
    for (int i = 0; i < input.get_dimension(0); ++i) {
        for (int j = 0; j < input.get_dimension(1); ++j) {
            input_ptr[j+stride*i] = std::log(std::pow(pow(sin(.1*i+.2*j),2)/cos(.1*j),2.0));
        }
    }

    struct test_case {
        pressio_options settings;
        std::set<std::string> expected_valid_features;
        std::set<std::string> expected_invalid_features;
        std::set<std::string> expected_valid_labels;
        std::set<std::string> expected_invalid_labels;
        pressio_options cache={};
        bool expected_compressed_required=true;
        bool expected_decompressed_required=true;
        bool expected_training_required=true;

        std::vector<std::string> failures={};

        std::string name() const {
            std::stringstream ss;
            ss << " for inputs=" << format_range(inputs());
            return ss.str();
        }

        std::vector<std::string> inputs() const  {
            std::vector<std::string> v;
            std::transform(settings.begin(), settings.end(), std::back_inserter(v), [](auto i){ return i.first; });
            return v;
        }

        void check_compress_required(libpressio_scheme& scheme, pressio_compressor& comp) {
            const bool compress_required = scheme->compress_required(comp, settings);
            if(compress_required != expected_compressed_required) {
                std::stringstream ss;
                ss << "expected compress_required actual=" << std::boolalpha << compress_required << " == expected=" << std::boolalpha << expected_compressed_required<< std::endl;
                failures.emplace_back(ss.str());
            }
        }
        void check_decompress_required(libpressio_scheme& scheme, pressio_compressor& comp) {
            const bool decompress_required = scheme->decompress_required(comp, settings);
            if(decompress_required != expected_compressed_required) {
                std::stringstream ss;
                ss << "expected compress_required actual=" << std::boolalpha << decompress_required << " == expected=" << std::boolalpha << expected_decompressed_required << std::endl;

                failures.emplace_back(ss.str());
            }
        }
        void check_training_required(libpressio_scheme& scheme, pressio_compressor& comp) {
            const bool training_required = scheme->training_required(comp);
            if(training_required != expected_training_required) {
                std::stringstream ss;
                ss << "expected training_required actual=" << std::boolalpha << training_required << " == expected=" << std::boolalpha << expected_training_required << std::endl;

                failures.emplace_back(ss.str());
            }
        }
        void check_feature_ids(libpressio_scheme& scheme) {
            std::vector<std::string> actual = scheme->get_feature_ids();
            std::set<std::string> actual_s(actual.begin(), actual.end());
            std::set<std::string> expected;
            expected.insert(expected_valid_features.begin(), expected_valid_features.end());
            expected.insert(expected_invalid_features.begin(), expected_invalid_features.end());
            if(auto i = set_not_equals(actual_s,expected); i.first) {
                std::stringstream ss;
                ss << "expected features ids to contain actual=" << format_range(actual_s) << "to contain expected=" << format_range(actual_s) << "differences" << format_range(i.second) << std::endl;
                failures.emplace_back(ss.str());
            }
        }

        void check_valid_feature_ids(libpressio_scheme& scheme, pressio_compressor const& comp) {
            std::vector<std::string> valid_metrics_ids = scheme->get_valid_feature_ids(comp, settings);
            if(auto i = set_not_equals(valid_metrics_ids,expected_valid_features); i.first) {
                std::stringstream ss;
                ss << "valid_feature_metrics do not match actual=" <<
                    format_range(valid_metrics_ids) << " != expected=" <<
                    format_range(expected_valid_features) <<
                    "differences" << format_range(i.second) <<
                    std::endl;
                failures.emplace_back(ss.str());
            }
        }

        void check_invalid_feature_ids(libpressio_scheme& scheme, pressio_compressor const& comp) {
            std::vector<std::string> invalid_metrics_ids = scheme->get_invalid_feature_ids(comp, settings);
            if(auto i = set_not_equals(invalid_metrics_ids,expected_invalid_features); i.first) {
                std::stringstream ss;
                ss << "invalid_feature_metrics do not match actual=" <<
                    format_range(invalid_metrics_ids) << " != expected=" <<
                    format_range(expected_invalid_features) <<
                    "differences" << format_range(i.second) <<
                    std::endl;
                failures.emplace_back(ss.str());
            }
        }

        void check_label_ids(libpressio_scheme& scheme) {
            std::vector<std::string> actual = scheme->get_label_ids();
            std::set<std::string> actual_s(actual.begin(), actual.end());
            std::set<std::string> expected;
            expected.insert(expected_valid_labels.begin(), expected_valid_labels.end());
            expected.insert(expected_invalid_labels.begin(), expected_invalid_labels.end());
            if(auto i = set_not_equals(actual_s, expected); i.first) {
                std::stringstream ss;
                ss << "expected label ids to contain actual=" << format_range(actual_s) << "to contain expected=" << format_range(actual_s) << " differences " << format_range(i.second) << std::endl;
                failures.emplace_back(ss.str());
            }
        }

        void check_valid_label_ids(libpressio_scheme& scheme, pressio_compressor const& comp) {
            std::vector<std::string> valid_metrics_ids = scheme->get_valid_label_ids(comp, settings);
            if(auto i = set_not_equals(valid_metrics_ids,expected_valid_labels); i.first) {
                std::stringstream ss;
                ss << "valid_label_metrics do not match actual=" <<
                    format_range(valid_metrics_ids) << " != expected=" <<
                    format_range(expected_valid_labels) <<
                    "differences" << format_range(i.second) <<
                    std::endl;
                failures.emplace_back(ss.str());
            }
        }

        void check_invalid_label_ids(libpressio_scheme& scheme, pressio_compressor const& comp) {
            std::vector<std::string> invalid_metrics_ids = scheme->get_invalid_label_ids(comp, settings);
            if(auto i = set_not_equals(invalid_metrics_ids, expected_invalid_labels); i.first) {
                std::stringstream ss;
                ss << "invalid_label_metrics do not match actual=" <<
                    format_range(invalid_metrics_ids) << " != expected=" <<
                    format_range(expected_invalid_labels) <<
                    "differences" << format_range(i.second) <<
                    std::endl;
                failures.emplace_back(ss.str());
            }
        }

        void throw_any_failures() {
            if(!failures.empty()){
                std::stringstream ss;
                ss << std::endl;
                for(auto const& f: failures) {
                    ss << "- \t" <<  f << std::endl;
                }
                throw std::runtime_error(ss.str());
            }
        }
    };
    std::vector<test_case> cases{
        {
            //everything that is not a label
            .settings = {{"predictors:all", true}},
            .expected_valid_features = {},
            .expected_invalid_features = {"mock:nondeterministic",
                                          "mock:error_dependent",
                                          "mock_data:error_agnostic",
                                          "mock_data:data"},
            .expected_valid_labels = {"size:compression_ratio"},
            .expected_invalid_labels = {},
            .cache = {
                {"size:compression_ratio", 2.0}
            },
        },

        {
            //everything including labels
            .settings = {{"predictors:all", true}, {"predictors:training", true}},
            .expected_valid_features = {},
            .expected_invalid_features = {"mock:nondeterministic",
                                          "mock:error_dependent",
                                          "mock_data:error_agnostic",
                                          "mock_data:data"},
            .expected_valid_labels = {},
            .expected_invalid_labels = {"size:compression_ratio"},
        },

        {
            //non-determinstic labels only
            .settings = {{"predictors:training", true}},
            .expected_valid_features = {"mock:error_dependent",
                                        "mock_data:error_agnostic",
                                        "mock_data:data"},
            .expected_invalid_features = {"mock:nondeterministic"},
            .expected_valid_labels = {"size:compression_ratio"},
            .expected_invalid_labels = {},
            .cache = {
                { "mock_data:error_agnostic", 3.0},
                {"mock_data:data", 3.0},
                {"mock:error_dependent", 3.0},
                {"size:compression_ratio", 2.0}
            },
        },

        { //data specific metrics only
         .settings = {{"predictors:data", true}},
         .expected_valid_features = {"mock:error_dependent",
                                     "mock_data:error_agnostic"},
         .expected_invalid_features = {"mock:nondeterministic",
                                       "mock_data:data"},
            .expected_valid_labels = {"size:compression_ratio"},
            .expected_invalid_labels = {},
            .cache = {
                {"mock_data:error_agnostic", 3.0},
                {"mock:error_dependent", 3.0},
                {"size:compression_ratio", 2.0}
            },
        },

        {.settings = {{"mock_compressor:runtime", 8.0}},
         .expected_valid_features = {"mock:error_dependent",
                                     "mock_data:error_agnostic",
                                     "mock_data:data"},
         .expected_invalid_features = {"mock:nondeterministic"},
         .expected_valid_labels = {"size:compression_ratio"},
         .expected_invalid_labels = {},
            .cache = {
                {"mock_data:error_agnostic", 3.0},
                {"mock:error_dependent", 3.0},
                {"mock_data:data", 3.0},
                {"size:compression_ratio", 2.0}
            },
        },

        {.settings = {{"mock_compressor:error_dependent", 9.0}},
         .expected_valid_features = {"mock_data:error_agnostic",
                                     "mock_data:data"},
         .expected_invalid_features = {"mock:nondeterministic",
                                       "mock:error_dependent"},

         .expected_valid_labels = {"size:compression_ratio"},
         .expected_invalid_labels = {},
            .cache = {
                {"mock_data:error_agnostic", 3.0},
                {"mock_data:data", 3.0},
                {"size:compression_ratio", 2.0}
            },
        },

        {.settings = {{"mock_compressor:error_agnostic", 10.0}},
         .expected_valid_features = {"mock:error_dependent", "mock_data:data"},
         .expected_invalid_features = {"mock:nondeterministic",
                                       "mock_data:error_agnostic"},
         .expected_valid_labels = {"size:compression_ratio"},
         .expected_invalid_labels = {},
            .cache = {
                {"mock:error_dependent", 3.0},
                {"mock_data:data", 3.0},
                {"size:compression_ratio", 2.0}
            },
        },

        {.settings = {{"mock_compressor:error_agnostic", 11.0}, 
                         {"mock_compressor:runtime", 12.0}},
         .expected_valid_features = {"mock:error_dependent", "mock_data:data"},
         .expected_invalid_features = {"mock:nondeterministic",
                                       "mock_data:error_agnostic"},
         .expected_valid_labels = {"size:compression_ratio"},
         .expected_invalid_labels = {},
            .cache = {
                {"mock:error_dependent", 3.0},
                {"mock_data:data", 3.0},
                {"size:compression_ratio", 2.0}
            },
        },
    };

    bool any_failure = false;
    for (auto& test_case: cases) {
        try {

            pressio_compressor comp = compressor_plugins().build("mock_compressor");
            libpressio_scheme scheme = scheme_plugins().build("mock");
            pressio_data output = pressio_data::owning(input);
            pressio_data compressed = pressio_data::empty(pressio_byte_dtype, {});

            test_case.check_compress_required(scheme, comp);
            test_case.check_decompress_required(scheme, comp);
            test_case.check_training_required(scheme, comp);
            test_case.check_feature_ids(scheme);
            test_case.check_valid_feature_ids(scheme, comp);
            test_case.check_invalid_feature_ids(scheme, comp);
            test_case.check_label_ids(scheme);
            test_case.check_valid_label_ids(scheme, comp);
            test_case.check_invalid_label_ids(scheme, comp);

            test_case.throw_any_failures();

            pressio_data features;
            pressio_data predicted_labels(pressio_data::owning(pressio_double_dtype, {scheme->get_label_ids().size()}));

            libpressio_predictor predictor = scheme->get_predictor(comp);

            if(scheme->training_required(comp)) {
                pressio_data actual_labels;
                std::tie(features, actual_labels) = scheme->get_features_and_labels(comp, test_case.settings, input, compressed, output, test_case.cache);
                pressio_data predicted_labels = pressio_data::owning(actual_labels);

                predictor->fit(features, actual_labels);
            } else {
                scheme->get_features(comp, test_case.settings, input, compressed, output, test_case.cache);
            }

            int ec = predictor->predict(features, predicted_labels);

            if(ec > 0) {
                throw std::runtime_error("prediction failed "s + predictor->error_msg());
            }
            std::cout << "passed " << test_case.name() << std::endl;
        } catch (std::exception const& ex) {
            std::cout << "test case " << format_range(test_case.inputs()) << std::endl << "Error :" <<  ex.what() << std::endl;
            any_failure = true;
        }
    }
    return any_failure?1:0;
}
