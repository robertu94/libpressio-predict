#include <std_compat/memory.h>
#include "pressio_reconstructor.h"

class noop_reconstructor final : public libpressio_reconstructor_plugin {
public:

  virtual std::vector<pressio_data> reconstruct_impl(const compat::span<const pressio_data* const>& span) override {
    std::vector<pressio_data> sampled;
    std::transform(
        span.begin(),
        span.end(),
        std::back_inserter(sampled),
        [](const pressio_data* const data) {
          return pressio_data::clone(*data);
        });
    return sampled;
  }

  std::unique_ptr<libpressio_reconstructor_plugin> clone() override {
    return compat::make_unique<noop_reconstructor>(*this);
  }
  const char *prefix() const override {
    return "noop";
  }
  const char *version() const override {
    return "0.0.0";
  }
protected:
};

static pressio_register noop_reconstructor_register (reconstructor_plugins(), "noop", []{
  return compat::make_unique<noop_reconstructor>();
});