
file(GLOB BRPC_UNITTESTS "brpc_*_unittest.cpp")
foreach(BRPC_UT ${BRPC_UNITTESTS})
    get_filename_component(BRPC_UT_WE ${BRPC_UT} NAME_WE)
    add_executable(${BRPC_UT_WE} ${BRPC_UT} $<TARGET_OBJECTS:TEST_PROTO_LIB>)
    target_link_libraries(${BRPC_UT_WE} flare-shared-debug
            gtest_main
            ${GPERFTOOLS_LIBRARIES})
    add_test(NAME ${BRPC_UT_WE} COMMAND ${BRPC_UT_WE})
endforeach()