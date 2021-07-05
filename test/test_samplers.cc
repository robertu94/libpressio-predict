#include "gtest/gtest.h"
#include "pressio_sampler.h"

class PressioSamplingIntegrationConfigOnly: public testing::TestWithParam<std::string>{
  void SetUp() {
    sampler = sampling_plugins().build(GetParam());
  }
protected:
  pressio_sampler sampler;
};
