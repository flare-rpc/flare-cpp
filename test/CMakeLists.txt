# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

find_package(Gperftools)
include_directories(${GPERFTOOLS_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR}/test)

include(CompileProto)
set(TEST_PROTO_FILES addressbook1.proto
        addressbook_encode_decode.proto
        addressbook_map.proto
        addressbook.proto
        echo.proto
        cord_buf.proto
        message.proto
        repeated.proto
        snappy_message.proto
        v1.proto
        v2.proto
        grpc.proto
        health_check.proto)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test/hdrs)
set(PROTOC_FLAGS ${PROTOC_FLAGS} -I${CMAKE_SOURCE_DIR}/test -I${CMAKE_SOURCE_DIR})
compile_proto(PROTO_HDRS PROTO_SRCS ${CMAKE_BINARY_DIR}/test
        ${CMAKE_BINARY_DIR}/test/hdrs
        ${CMAKE_SOURCE_DIR}/test
        "${TEST_PROTO_FILES}")
add_library(TEST_PROTO_LIB OBJECT ${PROTO_SRCS} ${PROTO_HDRS})

set(BRPC_SYSTEM_GTEST_SOURCE_DIR "" CACHE PATH "System googletest source directory.")

if (DOWNLOAD_GTEST)
    include(SetupGtest)
elseif (BRPC_SYSTEM_GTEST_SOURCE_DIR)
    add_subdirectory("${BRPC_SYSTEM_GTEST_SOURCE_DIR}" "${PROJECT_BINARY_DIR}/system-googletest-build")
else ()
    message(FATAL_ERROR "Googletest is not available")
endif ()

set(CMAKE_CPP_FLAGS "${DEFINE_CLOCK_GETTIME}  -DGFLAGS_NS=${GFLAGS_NS}")
set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -DBTHREAD_USE_FAST_PTHREAD_MUTEX -D__const__= -D_GNU_SOURCE -DUSE_SYMBOLIZE -DNO_TCMALLOC -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -DUNIT_TEST -Dprivate=public -Dprotected=public -DVARIABLE_NOT_LINK_DEFAULT_VARIABLES -D__STRICT_ANSI__ -include ${PROJECT_SOURCE_DIR}/test/sstream_workaround.h")
set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS} -g -O2 -pipe -Wall -W -fPIC -fstrict-aliasing -Wno-invalid-offsetof -Wno-unused-parameter -fno-omit-frame-pointer")
use_cxx17()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4))
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4 -msse4.2")
    endif ()
    if (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0))
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-aligned-new")
    endif ()
endif ()

file(COPY ${PROJECT_SOURCE_DIR}/test/cert1.key DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rpc)
file(COPY ${PROJECT_SOURCE_DIR}/test/cert2.key DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rpc)
file(COPY ${PROJECT_SOURCE_DIR}/test/cert1.crt DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rpc)
file(COPY ${PROJECT_SOURCE_DIR}/test/cert2.crt DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rpc)
file(COPY ${PROJECT_SOURCE_DIR}/test/jsonout DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rpc)

set(IO_TEST_SRC
        ${PROJECT_SOURCE_DIR}/test/io/cord_buf_test.cc
        )


# bthread_* functions are used in logging.cc, and they need to be marked as
# weak symbols explicitly in Darwin system.
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(DYNAMIC_LIB ${DYNAMIC_LIB}
            "-Wl,-U,_bthread_getspecific"
            "-Wl,-U,_bthread_setspecific"
            "-Wl,-U,_bthread_key_create")
endif ()

file(GLOB_RECURSE TESTING_SOURCES "${PROJECT_SOURCE_DIR}/test/testing/*.cc")

add_library(TESTING_DEBUG_LIB OBJECT ${TESTING_SOURCES})

add_library(FLARE_BASE_DEBUG_LIB OBJECT ${FLARE_SRC})
add_library(SOURCES_DEBUG_LIB OBJECT ${SOURCES})
add_dependencies(SOURCES_DEBUG_LIB PROTO_LIB)

# shared library needs POSITION_INDEPENDENT_CODE
set_property(TARGET ${SOURCES_DEBUG_LIB} PROPERTY POSITION_INDEPENDENT_CODE 1)
set_property(TARGET ${FLARE_BASE_DEBUG_LIB} PROPERTY POSITION_INDEPENDENT_CODE 1)
set_property(TARGET ${TESTING_DEBUG_LIB} PROPERTY POSITION_INDEPENDENT_CODE 1)

add_library(flare-shared-debug SHARED
        $<TARGET_OBJECTS:TESTING_DEBUG_LIB>
        $<TARGET_OBJECTS:FLARE_BASE_DEBUG_LIB>
        $<TARGET_OBJECTS:SOURCES_DEBUG_LIB>
        $<TARGET_OBJECTS:PROTO_LIB>)
# change the debug lib output dir to be different from the release output
set_target_properties(flare-shared-debug PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/test)

target_link_libraries(flare-shared-debug ${DYNAMIC_LIB})

# test_io
add_executable(test_io ${TEST_IO_SRC}
        ${CMAKE_CURRENT_BINARY_DIR}/cord_buf.pb.cc)
target_link_libraries(test_io flare-shared-debug
        gtest
        gtest_main
        ${GPERFTOOLS_LIBRARIES})

add_test(NAME test_io COMMAND test_io)

# test_variable
# -DVARIABLE_NOT_LINK_DEFAULT_VARIABLES not work for gcc >= 5.0, just remove the file to prevent linking into unit tests
list(REMOVE_ITEM VARIABLE_SOURCES ${PROJECT_SOURCE_DIR}/flare/variable/default_variables.cpp)

add_library(VARIABLE_DEBUG_LIB OBJECT ${VARIABLE_SOURCES})
add_subdirectory(var)

# bthread tests
add_subdirectory(bthread)

# rpc tests
add_subdirectory(rpc)

#
add_subdirectory(base)
add_subdirectory(container)
add_subdirectory(memory)
add_subdirectory(strings)
add_subdirectory(io)
add_subdirectory(debugging)
