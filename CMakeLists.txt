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

cmake_minimum_required(VERSION 3.15)


if (NOT $ENV{CONDA_PREFIX} STREQUAL "")
    if (APPLE)
        set(CMAKE_CXX_COMPILER $ENV{CONDA_PREFIX}/bin/clang++)
        set(CMAKE_C_COMPILER $ENV{CONDA_PREFIX}/bin/clang)
    else ()
        set(CMAKE_CXX_COMPILER $ENV{CONDA_PREFIX}/bin/g++)
        set(CMAKE_C_COMPILER $ENV{CONDA_PREFIX}/bin/gcc)
    endif ()
endif ()
project(flare C CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/copts)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/recipes)

include(carbin_print)

include(carbin_module)

# Compiler id for Apple Clang is now AppleClang.
cmake_policy(SET CMP0025 NEW)

# if command can use IN_LIST
cmake_policy(SET CMP0057 NEW)

# Project version variables are the empty string if version is unspecified
cmake_policy(SET CMP0048 NEW)

# Enable MACOSX_RPATH. Run "cmake --help-policy CMP0042" for policy details.
if (POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif ()

set(CMAKE_MACOSX_RPATH 1)


set(PROJECT_DESCRIPTION "carbin c++ lib")
set(PROJECT_VERSION ${CARBIN_PROJECT_REVISION})

# TODO modyify this for "-std=c++[x]"
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CARBIN_STATUS_PRINT "cmake toolchain print" ON)
option(CARBIN_STATUS_DEBUG "cmake toolchain debug info" ON)

set(CMAKE_VERBOSE_MAKEFILE ON)

option(DEBUG "Print debug logs" OFF)
option(WITH_DEBUG_SYMBOLS "With debug symbols" ON)
option(WITH_THRIFT "With thrift framed protocol supported" OFF)
option(BUILD_UNIT_TESTS "Whether to build unit tests" OFF)
option(DOWNLOAD_GTEST "Download and build a fresh copy of googletest. Requires Internet access." ON)


set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)


if (WITH_DEBUG_SYMBOLS)
    set(DEBUG_SYMBOL "-g")
endif ()

if (WITH_THRIFT)
    set(THRIFT_CPP_FLAG "-DENABLE_THRIFT_FRAMED_PROTOCOL")
    set(THRIFT_LIB "thrift")
endif ()

include(GNUInstallDirs)

find_package(GFLAGS REQUIRED)

set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -DFIBER_USE_FAST_PTHREAD_MUTEX -D__const__= -D_GNU_SOURCE -DUSE_SYMBOLIZE -DNO_TCMALLOC -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -DFLARE_RPC_REVISION=\\\"${CARBIN_PROJECT_REVISION}\\\" -D__STRICT_ANSI__")
set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} ${DEBUG_SYMBOL} ${THRIFT_CPP_FLAG}")
set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS} -O2 -pipe -Wall -W -fPIC -fstrict-aliasing -Wno-invalid-offsetof -Wno-unused-parameter -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS "${CMAKE_CPP_FLAGS} -O2 -pipe -Wall -W -fPIC -fstrict-aliasing -Wno-unused-parameter -fno-omit-frame-pointer")


if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if ((CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)") AND NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4))
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4 -msse4.2")
    elseif ((CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64"))
        # segmentation fault in libcontext
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-gcse")
    endif ()
    if (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0))
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-aligned-new")
    endif ()
endif ()

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

find_package(Leveldb REQUIRED)
find_package(Protobuf REQUIRED)

find_package(OpenSSL REQUIRED)

include_directories(
        ${GFLAGS_INCLUDE_PATH}
        ${PROTOBUF_INCLUDE_DIRS}
        ${LEVELDB_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
)

set(DYNAMIC_LIB
        ${GFLAGS_LIBRARY}
        ${PROTOBUF_LIBRARIES}
        ${LEVELDB_SHARED_LIBRARIES}
        ${PROTOBUF_PROTOC_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${THRIFT_LIB}
        ${THRIFTNB_LIB}
        ${OPENSSL_SSL_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
        dl
        z)


set(FLARE_PRIVATE_LIBS "-lgflags -lprotobuf -lleveldb -lprotoc -lssl -lcrypto -ldl -lz")

include(carbin_system_dylib)

set(DYNAMIC_LIB ${DYNAMIC_LIB} ${CARBIN_DYNAMIC_LIB})

# for *.so
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/output/lib)
# for *.a
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/output/lib)

# the reason why not using file(GLOB_RECURSE...) is that we want to
# include different files on different platforms.

file(GLOB_RECURSE FLARE_BASE_SRC "${PROJECT_SOURCE_DIR}/flare/base/*.cc")
file(GLOB_RECURSE FLARE_CONTAINER_SRC "${PROJECT_SOURCE_DIR}/flare/container/*.cc")
file(GLOB_RECURSE FLARE_HASH_SRC "${PROJECT_SOURCE_DIR}/flare/hash/*.cc")
file(GLOB_RECURSE FLARE_MEMORY_SRC "${PROJECT_SOURCE_DIR}/flare/memory/*.cc")
file(GLOB_RECURSE FLARE_IO_SRC "${PROJECT_SOURCE_DIR}/flare/io/*.cc")
file(GLOB_RECURSE FLARE_STRINGS_SRC "${PROJECT_SOURCE_DIR}/flare/strings/*.cc")
file(GLOB_RECURSE FLARE_DEBUG_SRC "${PROJECT_SOURCE_DIR}/flare/debugging/*.cc")
file(GLOB_RECURSE FLARE_LOG_SRC "${PROJECT_SOURCE_DIR}/flare/log/*.cc")
file(GLOB_RECURSE FLARE_BOOTSTRAP_SRC "${PROJECT_SOURCE_DIR}/flare/bootstrap/*.cc")
file(GLOB_RECURSE FLARE_TIMES_SRC "${PROJECT_SOURCE_DIR}/flare/times/*.cc")
file(GLOB_RECURSE FLARE_FILES_SRC "${PROJECT_SOURCE_DIR}/flare/files/*.cc")
file(GLOB_RECURSE FLARE_THREAD_SRC "${PROJECT_SOURCE_DIR}/flare/thread/*.cc")

set(FLARE_SRC
        "${PROJECT_SOURCE_DIR}/flare/base/dynamic_annotations/dynamic_annotations.c"
        ${FLARE_BASE_SRC}
        ${FLARE_CONTAINER_SRC}
        ${FLARE_HASH_SRC}
        ${FLARE_MEMORY_SRC}
        ${FLARE_IO_SRC}
        ${FLARE_STRINGS_SRC}
        ${FLARE_DEBUG_SRC}
        ${FLARE_LOG_SRC}
        ${FLARE_BOOTSTRAP_SRC}
        ${FLARE_HASH_SRC}
        ${FLARE_TIMES_SRC}
        ${FLARE_FILES_SRC}
        ${FLARE_THREAD_SRC}
        )

file(GLOB_RECURSE VARIABLE_SOURCES "${PROJECT_SOURCE_DIR}/flare/variable/*.cc")
file(GLOB_RECURSE FIBER_SOURCES "${PROJECT_SOURCE_DIR}/flare/fiber/*.cc")
file(GLOB_RECURSE JSON2PB_SOURCES "${PROJECT_SOURCE_DIR}/flare/json2pb/*.cc")
file(GLOB_RECURSE RPC_SOURCES "${PROJECT_SOURCE_DIR}/flare/rpc/*.cc")
file(GLOB_RECURSE THRIFT_SOURCES "${PROJECT_SOURCE_DIR}/flare/rpc/thrift*.cc")

if (WITH_THRIFT)
    message("flare compile with thrift protocol")
else ()
    # Remove thrift sources
    foreach (v ${THRIFT_SOURCES})
        list(REMOVE_ITEM RPC_SOURCES ${v})
    endforeach ()
    set(THRIFT_SOURCES "")
endif ()

set(MCPACK2PB_SOURCES
        ${PROJECT_SOURCE_DIR}/flare/mcpack2pb/field_type.cc
        ${PROJECT_SOURCE_DIR}/flare/mcpack2pb/mcpack2pb.cc
        ${PROJECT_SOURCE_DIR}/flare/mcpack2pb/parser.cc
        ${PROJECT_SOURCE_DIR}/flare/mcpack2pb/serializer.cc)

include(CompileProto)
set(PROTO_FILES flare/idl_options.proto
        flare/rpc/rtmp.proto
        flare/rpc/rpc_dump.proto
        flare/rpc/get_favicon.proto
        flare/rpc/span.proto
        flare/rpc/builtin_service.proto
        flare/rpc/get_js.proto
        flare/rpc/errno.proto
        flare/rpc/nshead_meta.proto
        flare/rpc/options.proto
        flare/rpc/policy/baidu_rpc_meta.proto
        flare/rpc/policy/hulu_pbrpc_meta.proto
        flare/rpc/policy/public_pbrpc_meta.proto
        flare/rpc/policy/sofa_pbrpc_meta.proto
        flare/rpc/policy/mongo.proto
        flare/rpc/trackme.proto
        flare/rpc/streaming_rpc_meta.proto
        flare/rpc/proto_base.proto)

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/output/include/flare)
set(PROTOC_FLAGS ${PROTOC_FLAGS} -I${PROTOBUF_INCLUDE_DIR})
compile_proto(PROTO_HDRS PROTO_SRCS ${PROJECT_BINARY_DIR}
        ${PROJECT_BINARY_DIR}/output/include
        ${PROJECT_SOURCE_DIR}
        "${PROTO_FILES}")
add_library(PROTO_LIB OBJECT ${PROTO_SRCS} ${PROTO_HDRS})

set(SOURCES
        ${VARIABLE_SOURCES}
        ${FIBER_SOURCES}
        ${JSON2PB_SOURCES}
        ${MCPACK2PB_SOURCES}
        ${RPC_SOURCES}
        ${THRIFT_SOURCES}
        )

add_subdirectory(flare)
if (BUILD_UNIT_TESTS)
    enable_testing()
    add_subdirectory(test)
endif ()
add_subdirectory(tools)

#[[
file(COPY ${CMAKE_CURRENT_BINARY_DIR}/flare/
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/output/include/flare/
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        )
]]

file(COPY ${PROJECT_SOURCE_DIR}/flare/
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/output/include/flare/
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        )
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/output/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        )

# Install pkgconfig
configure_file(cmake/flare.pc.in ${PROJECT_BINARY_DIR}/flare.pc @ONLY)
install(FILES ${PROJECT_BINARY_DIR}/flare.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
