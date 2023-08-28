cmake_minimum_required(VERSION 3.18)
project(LibPressioPredict VERSION 0.0.2)

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
find_package(LibPressioTools REQUIRED)
find_package(LibPressioDataset 0.0.6 REQUIRED)
pkg_search_module(ZSTD IMPORTED_TARGET GLOBAL libzstd)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(BUILD_SHARED_LIBS "prefer shared libraries" ON)
option(LIBPRESSIO_PREDICT_USE_MPI "enable MPI support" ON)
option(LIBPRESSIO_PREDICT_BUILD_TOOLS "build tools" ON)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libpressio_predict.h.in 
    ${CMAKE_CURRENT_BINARY_DIR}/include/libpressio_predict.h
)

add_library(libpressio_predict 
    src/libpressio_predict.cc
    src/predict.cc
    src/plugins/predictors/tao2019.cc
    src/plugins/predictors/rahman2023.cc
    )
target_include_directories(libpressio_predict PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include>
    )
target_link_libraries(libpressio_predict PUBLIC LibPressioTools::libpressio_meta LibPressioDataset::libpressio_dataset)
install(TARGETS libpressio_predict EXPORT LibPressioPredictTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

option(LIBPRESSIO_PREDICT_HAS_KHAN2023 "build support for khan 2023" OFF)
if(LIBPRESSIO_PREDICT_HAS_KHAN2023)
    find_package(PkgConfig REQUIRED)
    find_package(SZ3 REQUIRED)
    find_package(ZFP REQUIRED)
    target_link_libraries(libpressio_predict PRIVATE SZ3::SZ3)
    target_link_libraries(libpressio_predict PRIVATE zfp::zfp)
    target_sources(libpressio_predict PRIVATE
        src/plugins/predictors/khan2023_sz3.cc
        src/plugins/predictors/khan2023_zfp.cc)
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
    target_link_libraries(pressio_predict_bench PRIVATE libpressio_predict LibPressioDataset::libpressio_dataset LibDistributed::libdistributed SQLite::SQLite3)
    install(TARGETS pressio_predict_bench EXPORT LibPressioPredictTargets
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
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


