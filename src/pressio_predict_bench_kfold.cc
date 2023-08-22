#include "pressio_predict_bench_kfold.h"

namespace libpressio { namespace predict {
fold_t kfold(std::vector<pressio_options> inputs, size_t k) {
    if(k < 1) {
        throw std::runtime_error("invalid k fold " + std::to_string(k));
    }
    return fold_t{k, inputs};
}
} }
