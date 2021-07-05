#include <libpressio_ext/cpp/compressor.h>
#include "pressio_sampler.h"
#include <std_compat/memory.h>
#include <sstream>
#include <pressio_reconstructor.h>

class pressio_sampling final: public libpressio_compressor_plugin {
public:
  std::shared_ptr<libpressio_compressor_plugin> clone() override {
    return compat::make_unique<pressio_sampling>(*this);
  }

  const char *prefix() const override {
    return "sampling";
  }
  const char *version() const override {
    const static std::string version_str = [this]{
      std::stringstream ss;
      ss << this->major_version() << '.' << this->minor_version() << '.' << this->patch_version();
      return ss.str();
    }();
    return version_str.c_str();
  }

  void set_name_impl(const std::string &new_name) override {

  }
  int major_version() const override {
    return 1;
  }
  int minor_version() const override {
    return 0;
  }
  int patch_version() const override {
    return 0;
  }

  pressio_options get_options_impl() const override {
    pressio_options opts;
    set_meta(opts, "sampling:sampler", sampler_id, sampler);
    set_meta(opts, "sampling:reconstructor", reconstructor_id, reconstructor);
    return opts;
  }
  pressio_options get_documentation_impl() const override {
    pressio_options opts;
    set_meta_docs(opts, "sampling:sampler", "sampling method to use", sampler);
    set_meta_docs(opts, "sampling:reconstructor", "reconstruction method to use", reconstructor);
    set(opts, "pressio:description", "meta-compressor for sampling methods for libpressio");
    return opts;
  }
  pressio_options get_configuration_impl() const override {
    pressio_options opts;
    opts.copy_from(sampler->get_configuration());
    opts.copy_from(reconstructor->get_configuration());
    return opts;
  }
  int set_options_impl(const pressio_options &options) override {
    get_meta(options, "sampling:sampler", sampling_plugins(), sampler_id, sampler);
    get_meta(options, "sampling:reconstructor", reconstructor_plugins(), reconstructor_id, reconstructor);
    return 0;
  }

protected:
  int decompress_many_impl(const compat::span<const pressio_data *const> &inputs,
                           compat::span<pressio_data *> &outputs) override {
    return set_error(1, "not implemented yet");
  }
  int compress_many_impl(const compat::span<const pressio_data *const> &inputs,
                         compat::span<pressio_data *> &outputs) override {
    return set_error(1, "not implemented yet");
  }

  int compress_impl(const pressio_data *input, struct pressio_data *output) override {
    return compress_many(
        &input, 1,
        &output, 1
        );
  }
  int decompress_impl(const pressio_data *input, struct pressio_data *output) override {
    return decompress_many(
        &input, 1,
        &output, 1
    );
  }

  static std::string version_str;
  std::string sampler_id="noop", reconstructor_id="noop";
  pressio_sampler sampler = sampling_plugins().build("noop");
  pressio_reconstructor reconstructor = reconstructor_plugins().build("noop");
};

static pressio_register pressio_sampling_register(compressor_plugins(), "sampling", []{
  return compat::make_unique<pressio_sampling>();
});
