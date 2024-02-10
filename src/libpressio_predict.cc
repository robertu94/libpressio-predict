#include "libpressio_predict.h"
extern "C" {
int libpressio_predict_version_major() {
    return LIBPRESSIO_PREDICT_MAJOR_VERSION;
}
int libpressio_predict_version_minor() {
    return LIBPRESSIO_PREDICT_MINOR_VERSION;
}
int libpressio_predict_version_patch() {
    return LIBPRESSIO_PREDICT_PATCH_VERSION;
}
}

