#include <libpressio_ext/cpp/configurable.h>
#include <libpressio_ext/cpp/errorable.h>
#include <memory>
#include <vector>

struct observation {
  const std::vector<double> inputs;
  const std::vector<double> outputs;
};

class point_database {
  std::vector<observation> observations;
};

class libpressio_predictor_plugin: public pressio_errorable, public pressio_configurable {
  public:

  virtual std::vector<double> predict(std::vector<double> const&) const=0;

  virtual std::unique_ptr<libpressio_predictor_plugin> clone()=0;
};
