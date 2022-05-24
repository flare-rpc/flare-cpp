

file(GLOB TEST_VARIABLE_SRCS "variable_*_test.cc")
add_executable(test_variable ${TEST_VARIABLE_SRCS}
        $<TARGET_OBJECTS:VARIABLE_DEBUG_LIB>
        $<TARGET_OBJECTS:FLARE_BASE_DEBUG_LIB>)
target_link_libraries(test_variable ${GTEST_LIB}
        ${GPERFTOOLS_LIBRARIES}
        ${DYNAMIC_LIB})
add_test(NAME test_variable COMMAND test_variable)