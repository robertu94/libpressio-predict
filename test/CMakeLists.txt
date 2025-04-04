
if(LIBPRESSIO_PREDICT_HAS_PYTHON)
    find_package(Python REQUIRED COMPONENTS Interpreter)
    execute_process(
        COMMAND
            ${Python_EXECUTABLE}  -c "import numpy"
        RESULT_VARIABLE NUMPY_FOUND
        OUTPUT_QUIET
        ERROR_QUIET
    )
    if(${NUMPY_FOUND} EQUAL 0)
        #this test requires numpy, so make sure it is installed
        add_executable(test_python test_python.cc)
        target_link_libraries(test_python PRIVATE libpressio_predict)
        add_test(NAME libpressio_predict_test_python COMMAND test_python)
    endif()
endif()

add_executable(test_scheme test_scheme.cc)
target_link_libraries(test_scheme PRIVATE libpressio_predict)
add_test(NAME test_scheme COMMAND test_scheme)
