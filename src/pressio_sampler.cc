#include "pressio_sampler.h"

pressio_registry<std::unique_ptr<libpressio_sampler_plugin>>& sampling_plugins() {
  static pressio_registry<std::unique_ptr<libpressio_sampler_plugin>> registry;
  return registry;
}