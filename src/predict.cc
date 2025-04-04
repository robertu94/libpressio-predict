#include "libpressio_predict_ext/cpp/predict.h"
#include "libpressio_predict_ext/cpp/predict_metrics.h"
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


} }
