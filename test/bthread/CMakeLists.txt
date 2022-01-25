

file(GLOB BTHREAD_UNITTESTS "bthread*unittest.cpp")
foreach(BTHREAD_UT ${BTHREAD_UNITTESTS})
    get_filename_component(BTHREAD_UT_WE ${BTHREAD_UT} NAME_WE)
    add_executable(${BTHREAD_UT_WE} ${BTHREAD_UT})
    target_link_libraries(${BTHREAD_UT_WE} gtest_main
            flare-shared-debug
            ${GPERFTOOLS_LIBRARIES})
    add_test(NAME ${BTHREAD_UT_WE} COMMAND ${BTHREAD_UT_WE})
endforeach()