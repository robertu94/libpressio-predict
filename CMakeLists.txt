cmake_minimum_required(VERSION 3.12)
project(libpressio_predict VERSION "0.0.1" LANGUAGES C CXX)

#correct was to set a default build type
# https://blog.kitware.com/cmake-and-the-default-build-type/
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type was set. Setting build type to ${default_build_type}.")
  set(CMAKE_BUILD_TYPE ${default_build_type} CACHE 
    STRING "Choose the type to build" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

include(CTest)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(BUILD_SHARED_LIBS "build libraries as shared" ON)

set(default_cxx_standard 17)
if(NOT LIBPRESSIO_PREDICT_CXX_VERSION)
  set(LIBPRESSIO_PREDICT_CXX_VERSION ${default_cxx_standard} CACHE 
    STRING "Which version of the cxx standard to expect" FORCE)
  set_property(CACHE LIBPRESSIO_PREDICT_CXX_VERSION PROPERTY STRINGS 17 20)
endif()

include(GNUInstallDirs)

find_package(LibPressio REQUIRED)
find_package(MPI REQUIRED COMPONENTS CXX)
find_package(LibDistributed REQUIRED)
find_package(dlib REQUIRED)
find_package(PkgConfig)
find_package(Threads)

add_library(libpressio_predict
  #core features
    src/predictors/nearest_neighbor.cc
  #public headers

  #private headers
  )
target_include_directories(
  libpressio_predict
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/libpressio_predict>
  )
target_link_libraries(libpressio_predict PUBLIC LibDistributed::libdistributed
  LibPressio::libpressio PRIVATE dlib::dlib )
target_compile_features(libpressio_predict PUBLIC cxx_std_${LIBPRESSIO_PREDICT_CXX_VERSION})

option(USE_CLANG_TIDY "include clang-tidy warnings in the build log" OFF)
if(USE_CLANG_TIDY)
  find_program(CLANG_TIDY clang-tidy)
  set_target_properties(libpressio_predict PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY}")
  set_target_properties(libpressio_predict PROPERTIES C_CLANG_TIDY "${CLANG_TIDY}")
endif()

export(TARGETS libpressio_predict NAMESPACE LibPressioPredict:: FILE
  LibPressioPredictConfig.cmake)
install(TARGETS libpressio_predict EXPORT LibPressioPredictConfig
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
install(EXPORT LibPressioPredictConfig NAMESPACE LibPressioPredict:: DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/LibPressioPredict/cmake)
install(DIRECTORY include/ DESTINATION
  ${CMAKE_INSTALL_INCLUDEDIR}/libpressio_predict)

option(BUILD_DOCS "build the documetation" OFF)
if(BUILD_DOCS)
  find_package(Doxygen REQUIRED dot)
  set(DOXYGEN_GENERATE_HTML YES)
  set(DOXYGEN_MAN_LINKS YES)
  set(DOXYGEN_GENERATE_MAN YES)
  set(DOXYGEN_EXTRACT_LOCAL_METHODS YES)
  set(DOXYGEN_EXTRACT_STATIC YES)
  set(DOXYGEN_MACRO_EXPANSION YES)
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
  doxygen_add_docs(
    docs
    ${PROJECT_SOURCE_DIR}/docs
    ${PROJECT_SOURCE_DIR}/README.md
    ${PROJECT_SOURCE_DIR}/include
    COMMENT "Generate Documenation"
  )
endif()

if(BUILD_TESTING)
  add_subdirectory(test)
endif()

