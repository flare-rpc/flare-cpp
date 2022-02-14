
add_executable(reset_flags_test reset_flags_test.cc)
target_link_libraries(reset_flags_test flare-shared-debug
        gtest
        gmock
        ${GPERFTOOLS_LIBRARIES})
add_test(NAME symbolize_test COMMAND reset_flags_test)