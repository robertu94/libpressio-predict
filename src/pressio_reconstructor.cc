#include "pressio_reconstructor.h"

pressio_registry<std::unique_ptr<libpressio_reconstructor_plugin>>& reconstructor_plugins() {
  static pressio_registry<std::unique_ptr<libpressio_reconstructor_plugin>> registry;
  return registry;
}