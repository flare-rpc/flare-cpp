

file(GLOB TEST_BVAR_SRCS "bvar_*_unittest.cpp")
add_executable(test_bvar ${TEST_BVAR_SRCS}
        $<TARGET_OBJECTS:BVAR_DEBUG_LIB>
        $<TARGET_OBJECTS:BUTIL_DEBUG_LIB>
        $<TARGET_OBJECTS:FLARE_BASE_DEBUG_LIB>)
target_link_libraries(test_bvar gtest
        ${GPERFTOOLS_LIBRARIES}
        ${DYNAMIC_LIB})
add_test(NAME test_bvar COMMAND test_bvar)