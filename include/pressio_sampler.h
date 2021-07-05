#include <std_compat/span.h>
#include <libpressio_ext/cpp/data.h>
#include <libpressio_ext/cpp/pressio.h>

class libpressio_sampler_plugin: public pressio_versionable, public pressio_configurable, public pressio_errorable {
public:
  std::vector<pressio_data> sample(compat::span<const pressio_data* const> const&);
  virtual std::unique_ptr<libpressio_sampler_plugin> clone()=0;
protected:
  virtual std::vector<pressio_data> sample_impl(compat::span<const pressio_data* const> const&)=0;
};

struct pressio_sampler {
  pressio_sampler(std::unique_ptr<libpressio_sampler_plugin>&& i): plugin(std::move(i)) {}
  pressio_sampler()=default;
  pressio_sampler(pressio_sampler const& o): plugin(o->clone()) {}
  pressio_sampler(pressio_sampler && o)=default;
  pressio_sampler& operator=(pressio_sampler const& o) {
    if(this == &o) {
      return *this;
    }
    this->plugin = o->clone();
    return *this;
  }
  pressio_sampler& operator=(pressio_sampler && o)=default;
  libpressio_sampler_plugin& operator*() const noexcept {
    return plugin.operator*();
  }
  libpressio_sampler_plugin* operator->() const noexcept {
    return plugin.operator->();
  }
  operator bool() const {
    return static_cast<bool>(plugin);
  }
  std::unique_ptr<libpressio_sampler_plugin> plugin;
};

pressio_registry<std::unique_ptr<libpressio_sampler_plugin>> & sampling_plugins();

