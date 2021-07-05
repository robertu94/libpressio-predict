#include <std_compat/memory.h>
#include "pressio_sampler.h"

class noop_sampler final : public libpressio_sampler_plugin {
public:
  std::unique_ptr<libpressio_sampler_plugin> clone() override {
    return compat::make_unique<noop_sampler>(*this);
  }
  const char *prefix() const override {
    return "noop";
  }
  const char *version() const override {
    return "0.0.0";
  }
protected:
  std::vector<pressio_data> sample_impl(const compat::span<const pressio_data *const> &span) override {
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
};

static pressio_register noop_sampler_register (sampling_plugins(), "noop", []{
  return compat::make_unique<noop_sampler>();
});