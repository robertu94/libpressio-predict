#include <libpressio_predict_ext/cpp/predict.h>
#include <libpressio_ext/python/python.h>
#include <std_compat/memory.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <pybind11/pytypes.h>
#include <string>

namespace libpressio { namespace predict {

namespace py = pybind11;
using namespace py::literals;

class __attribute__((visibility ("hidden"))) libpressio_python_predict final : public libpressio_predict_plugin {
    template <class T>
    static std::vector<size_t> to_strides(std::vector<size_t> const& v) {
        std::vector<size_t> ret(v.size());
        if(!v.empty()) ret[0] = sizeof(T);
        for (size_t i = 1; i < v.size(); ++i) {
            ret[i] = (ret[i-1] * v[i-1]);
        }
        std::reverse(ret.begin(), ret.end());
        return ret;
    }
    static std::vector<size_t> to_dims(std::vector<size_t> const& v) {
        std::vector<size_t> ret(v);
        std::reverse(ret.begin(), ret.end());
        return ret;
    }

    public:
    libpressio_python_predict(std::shared_ptr<python_launch::libpressio_external_pybind_manager>&& mgr):
        pybind_manager(mgr) {}

    std::unique_ptr<libpressio_predict_plugin> clone() const override {
        return std::make_unique<libpressio_python_predict>(*this);
    }
    int fit_impl(pressio_data const& features, pressio_data const& labels) override {
        if(features.dtype() != pressio_double_dtype) return set_error(1, "features must be double for now");
        if(labels.dtype() != pressio_double_dtype) return set_error(1, "labels must be double for now");
        auto features_a = py::memoryview::from_buffer(
                static_cast<double*>(features.data()),
                to_dims(features.dimensions()),
                to_strides<double>(features.dimensions())
        );
        auto labels_a = py::memoryview::from_buffer(
                static_cast<double*>(labels.data()),
                to_dims(labels.dimensions()),
                to_strides<double>(labels.dimensions())
        );
        state["features"] = features_a;
        state["labels"] = labels_a;
        state["names"] = names;
        try {
            py::exec(program, state);
            state["state"] = py::eval("fit(features, labels)", state);
        } catch (std::exception const& ex) {
            return set_error(2, ex.what());
        }
        state["features"] = py::none{};
        state["labels"] = py::none{};
        return 0;
    }
    int predict_impl(pressio_data const& features, pressio_data& out_labels) override {
        if(features.dtype() != pressio_double_dtype) return set_error(1, "features must be double for now");
        auto features_a = py::memoryview::from_buffer(
                static_cast<double*>(features.data()),
                to_dims(features.dimensions()),
                to_strides<double>(features.dimensions())
        );
        state["features"] = features_a;
        state["names"] = names;
        try {
            py::exec(program, state);
            auto out_obj = py::eval("predict(features, state)", state);
            py::print(out_obj);
            py::array_t<double> out = std::move(out_obj);
            out_labels = pressio_data::copy(
                    pressio_double_dtype,
                    out.data(),
                    to_dims(std::vector<size_t>(out.shape(), out.shape() + out.ndim())));
        } catch (std::exception const& ex) {
            return set_error(2, ex.what());
        }
        state["features"] = py::none{};
        return 0;
    }
    int set_options(pressio_options const& opts) override {
        get(opts, "python:program", &program);
        get(opts, "python:feature_names", &names);
        return 0;
    }
    pressio_options get_options() const override {
        pressio_options opts;
        set(opts, "python:program", program);
        set(opts, "python:feature_names", names);
        return opts;
    }

    int patch_version() const override {
        return 1;
    }

    const char* prefix() const override {
        return "python";
    }

    bool training_required() const override {
        return true;
    }


    std::string program;
    std::vector<std::string> names;
    std::shared_ptr<python_launch::libpressio_external_pybind_manager> pybind_manager;
    py::dict state;
};


static pressio_register compressor_many_fields_plugin(predictor_plugins(), "python", []() {
  return compat::make_unique<libpressio_python_predict>(libpressio::python_launch::get_library());
});

} }
