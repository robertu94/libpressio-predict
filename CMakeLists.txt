cmake_minimum_required(VERSION 3.18)
project(LibPressioPredict VERSION 0.0.5)

#correct was to set a default build type
# https://blog.kitware.com/cmake-and-the-default-build-type/
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type was set. Setting build type to ${default_build_type}.")
  set(CMAKE_BUILD_TYPE ${default_build_type} CACHE 
    STRING "Choose the type to build" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")


include(CTest)
include(GNUInstallDirs)
find_package(LibPressio 0.96.5 REQUIRED)
find_package(LibPressioDataset 0.0.7 REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(BUILD_SHARED_LIBS "prefer shared libraries" ON)
option(LIBPRESSIO_PREDICT_USE_MPI "enable MPI support" ON)
option(LIBPRESSIO_PREDICT_BUILD_TOOLS "build tools" ON)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libpressio_predict.h.in 
    ${CMAKE_CURRENT_BINARY_DIR}/include/libpressio_predict_version.h
)

add_library(libpressio_predict 
    src/libpressio_predict.cc
    src/predict.cc
    src/plugins/predictors/noop.cc
    src/plugins/predictor_metrics/medape.cc
    src/plugins/metrics/tao2019.cc
    src/plugins/metrics/rahman2023.cc
    src/plugins/metrics/bench_metric.cc
    src/plugins/scheme/simple.cc
    src/plugins/scheme/tao2019.cc
    )
target_include_directories(libpressio_predict PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/libpressio_predict>
    )
target_link_libraries(libpressio_predict PUBLIC LibPressioDataset::libpressio_dataset)
install(TARGETS libpressio_predict EXPORT LibPressioPredictTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

option(LIBPRESSIO_PREDICT_HAS_KHAN2023 "build support for khan 2023" OFF)
if(LIBPRESSIO_PREDICT_HAS_KHAN2023)
    find_package(PkgConfig REQUIRED)
    pkg_search_module(ZSTD IMPORTED_TARGET GLOBAL libzstd)
    find_package(SZ3 REQUIRED)
    find_package(ZFP REQUIRED)
    target_link_libraries(libpressio_predict PRIVATE SZ3::SZ3)
    target_link_libraries(libpressio_predict PRIVATE zfp::zfp)
    target_sources(libpressio_predict PRIVATE
        src/plugins/metrics/khan2023_sz3.cc
        src/plugins/metrics/khan2023_zfp.cc)
endif()

option(LIBPRESSIO_PREDICT_HAS_SIAN2022 "build support for sian 2022" OFF)
if(LIBPRESSIO_PREDICT_HAS_SIAN2022)
    find_package(PkgConfig REQUIRED)
    pkg_search_module(ZSTD IMPORTED_TARGET GLOBAL libzstd)
    find_package(SZ3 REQUIRED)
    target_link_libraries(libpressio_predict PRIVATE SZ3::SZ3)
    target_sources(libpressio_predict PRIVATE
        src/plugins/metrics/sian2022.cc)
endif()

option(LIBPRESSIO_PREDICT_HAS_PYTHON "build support for predictors in python" OFF)
if(LIBPRESSIO_PREDICT_HAS_PYTHON)
  find_package(pybind11 REQUIRED)
  find_package(Python3 REQUIRED COMPONENTS Development.Embed)
  target_sources(libpressio_predict
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins/predictors/python.cc
    )
  target_link_libraries(libpressio_predict PRIVATE pybind11::embed Python3::Python)
endif()

option(LIBPRESSIO_PREDICT_HAS_OPT "build support for proxy based predictors" OFF)
if(LIBPRESSIO_PREDICT_HAS_OPT)
    find_package(LibPressioOpt REQUIRED)
    target_sources(libpressio_predict
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins/search/predict.cc
        )
    target_link_libraries(libpressio_predict PRIVATE LibPressioOpt::libpressio_opt)
endif()

if(LIBPRESSIO_PREDICT_BUILD_TOOLS)
    find_package(LibDistributed 0.4.3 REQUIRED)
    find_package(SQLite3)
    add_executable(pressio_predict_bench 
        src/pressio_predict_bench.cc
        src/pressio_predict_bench_db.cc
        src/pressio_predict_bench_cli.cc
        src/pressio_predict_bench_kfold.cc
    )
target_link_libraries(pressio_predict_bench PRIVATE libpressio_predict ${CMAKE_DL_LIBS} LibPressioDataset::libpressio_dataset LibDistributed::libdistributed SQLite::SQLite3)
    install(TARGETS pressio_predict_bench EXPORT LibPressioPredictTargets
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

if(BUILD_TESTING)
    add_subdirectory(test)
endif()



install(EXPORT LibPressioPredictTargets NAMESPACE LibPressioPredict:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LibPressioPredict/)
export(EXPORT LibPressioPredictTargets FILE ${CMAKE_CURRENT_BINARY_DIR}/cmake/LibPressioPredictTargets.cmake NAMESPACE LibPressioPredict::)
include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/LibPressioPredictConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/LibPressioPredictConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LibPressioPredict
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/LibPressioPredictConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY AnyNewerVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/LibPressioPredictConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/LibPressioPredictConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LibPressioPredict
)
install(DIRECTORY
    include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libpressio_predict
    )
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/include/libpressio_predict_version.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libpressio_predict
    )


