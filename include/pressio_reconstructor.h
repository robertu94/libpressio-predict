#include <std_compat/span.h>
#include <libpressio_ext/cpp/data.h>
#include <libpressio_ext/cpp/pressio.h>

class libpressio_reconstructor_plugin: public pressio_versionable, public pressio_configurable, public pressio_errorable {
public:
  std::vector<pressio_data> reconstruct(compat::span<const pressio_data* const> const&);
  virtual std::unique_ptr<libpressio_reconstructor_plugin> clone()=0;
protected:
  virtual std::vector<pressio_data> reconstruct_impl(compat::span<const pressio_data* const> const&)=0;
};

struct pressio_reconstructor {
  pressio_reconstructor(std::unique_ptr<libpressio_reconstructor_plugin>&& i): plugin(std::move(i)) {}
  pressio_reconstructor()=default;
  pressio_reconstructor(pressio_reconstructor const& o): plugin(o->clone()) {}
  pressio_reconstructor(pressio_reconstructor && o)=default;
  pressio_reconstructor& operator=(pressio_reconstructor const& o) {
    if(this == &o) {
      return *this;
    }
    this->plugin = o->clone();
    return *this;
  }
  pressio_reconstructor& operator=(pressio_reconstructor && o)=default;
  libpressio_reconstructor_plugin& operator*() const noexcept {
    return plugin.operator*();
  }
  libpressio_reconstructor_plugin* operator->() const noexcept {
    return plugin.operator->();
  }
  operator bool() const {
    return static_cast<bool>(plugin);
  }
  std::unique_ptr<libpressio_reconstructor_plugin> plugin;
};

pressio_registry<std::unique_ptr<libpressio_reconstructor_plugin>> & reconstructor_plugins();

