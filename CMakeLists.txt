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

cmake_minimum_required(VERSION 3.1.3)
project(flare C CXX)

option(WITH_GLOG "With glog" ON)
option(WITH_MESALINK "With MesaLink" OFF)
option(DEBUG "Print debug logs" OFF)
option(WITH_DEBUG_SYMBOLS "With debug symbols" ON)
option(WITH_THRIFT "With thrift framed protocol supported" OFF)
option(BUILD_UNIT_TESTS "Whether to build unit tests" OFF)
option(DOWNLOAD_GTEST "Download and build a fresh copy of googletest. Requires Internet access." ON)

# Enable MACOSX_RPATH. Run "cmake --help-policy CMP0042" for policy details.
if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif()

set(BRPC_VERSION 0.9.0)

SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "brpc authors")
INCLUDE(CPack)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # require at least gcc 4.8
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
        message(FATAL_ERROR "GCC is too old, please install a newer version supporting C++11")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # require at least clang 3.3
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.3)
        message(FATAL_ERROR "Clang is too old, please install a newer version supporting C++11")
    endif()
else()
    message(WARNING "You are using an unsupported compiler! Compilation has only been tested with Clang and GCC.")
endif()


if(WITH_DEBUG_SYMBOLS)
    set(DEBUG_SYMBOL "-g")
endif()

if(WITH_THRIFT)
    set(THRIFT_CPP_FLAG "-DENABLE_THRIFT_FRAMED_PROTOCOL")
    set(THRIFT_LIB "thrift")
endif()

include(GNUInstallDirs)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(GFLAGS REQUIRED)

execute_process(
    COMMAND bash -c "grep \"namespace [_A-Za-z0-9]\\+ {\" ${GFLAGS_INCLUDE_PATH}/gflags/gflags_declare.h | head -1 | awk '{print $2}' | tr -d '\n'"
    OUTPUT_VARIABLE GFLAGS_NS
)
if(${GFLAGS_NS} STREQUAL "GFLAGS_NAMESPACE")
    execute_process(
        COMMAND bash -c "grep \"#define GFLAGS_NAMESPACE [_A-Za-z0-9]\\+\" ${GFLAGS_INCLUDE_PATH}/gflags/gflags_declare.h | head -1 | awk '{print $3}' | tr -d '\n'"
        OUTPUT_VARIABLE GFLAGS_NS
    )
endif()

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

execute_process(
    COMMAND bash -c "${PROJECT_SOURCE_DIR}/tools/get_brpc_revision.sh ${PROJECT_SOURCE_DIR} | tr -d '\n'"
    OUTPUT_VARIABLE BRPC_REVISION
)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    include(CheckFunctionExists)
    CHECK_FUNCTION_EXISTS(clock_gettime HAVE_CLOCK_GETTIME)
    if(NOT HAVE_CLOCK_GETTIME)
        set(DEFINE_CLOCK_GETTIME "-DNO_CLOCK_GETTIME_IN_MAC")
    endif()
    set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -Wno-deprecated-declarations -Wno-inconsistent-missing-override")
endif()

set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} ${DEFINE_CLOCK_GETTIME}  -DGFLAGS_NS=${GFLAGS_NS}")
if(WITH_MESALINK)
    set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -DUSE_MESALINK")
endif()
set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -DBTHREAD_USE_FAST_PTHREAD_MUTEX -D__const__= -D_GNU_SOURCE -DUSE_SYMBOLIZE -DNO_TCMALLOC -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -DBRPC_REVISION=\\\"${BRPC_REVISION}\\\" -D__STRICT_ANSI__")
set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} ${DEBUG_SYMBOL} ${THRIFT_CPP_FLAG}")
set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS} -O2 -pipe -Wall -W -fPIC -fstrict-aliasing -Wno-invalid-offsetof -Wno-unused-parameter -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS "${CMAKE_CPP_FLAGS} -O2 -pipe -Wall -W -fPIC -fstrict-aliasing -Wno-unused-parameter -fno-omit-frame-pointer")

macro(use_cxx17)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endmacro(use_cxx17)

use_cxx17()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    #required by butil/crc32.cc to boost performance for 10x
    if((CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)") AND NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4))
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4 -msse4.2")
    elseif((CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64"))
        # segmentation fault in libcontext
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-gcse")
    endif()
    if(NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0))
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-aligned-new")
    endif()
endif()

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

find_path(LEVELDB_INCLUDE_PATH NAMES leveldb/db.h)
find_library(LEVELDB_LIB NAMES leveldb)
if ((NOT LEVELDB_INCLUDE_PATH) OR (NOT LEVELDB_LIB))
    message(FATAL_ERROR "Fail to find leveldb")
endif()

    find_path(GLOG_INCLUDE_PATH NAMES glog/logging.h)
    find_library(GLOG_LIB NAMES glog)
    if((NOT GLOG_INCLUDE_PATH) OR (NOT GLOG_LIB))
        message(FATAL_ERROR "Fail to find glog")
    endif()
    include_directories(${GLOG_INCLUDE_PATH})

if(WITH_MESALINK)
    find_path(MESALINK_INCLUDE_PATH NAMES mesalink/openssl/ssl.h)
    find_library(MESALINK_LIB NAMES mesalink)
    if((NOT MESALINK_INCLUDE_PATH) OR (NOT MESALINK_LIB))
        message(FATAL_ERROR "Fail to find MesaLink")
    else()
        message(STATUS "Found MesaLink: ${MESALINK_LIB}")
    endif()
    include_directories(${MESALINK_INCLUDE_PATH})
endif()

find_library(PROTOC_LIB NAMES protoc)
if(NOT PROTOC_LIB)
    message(FATAL_ERROR "Fail to find protoc lib")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(OPENSSL_ROOT_DIR
        "/usr/local/opt/openssl"    # Homebrew installed OpenSSL
        )
endif()

find_package(OpenSSL)

include_directories(
        ${GFLAGS_INCLUDE_PATH}
        ${PROTOBUF_INCLUDE_DIRS}
        ${LEVELDB_INCLUDE_PATH}
        ${OPENSSL_INCLUDE_DIR}
        )

set(DYNAMIC_LIB
    ${GFLAGS_LIBRARY}
    ${PROTOBUF_LIBRARIES}
    ${LEVELDB_LIB}
    ${PROTOC_LIB}
    ${CMAKE_THREAD_LIBS_INIT}
    ${THRIFT_LIB}
    ${THRIFTNB_LIB}
    ${OPENSSL_CRYPTO_LIBRARY}
    dl
    z)

if(WITH_MESALINK)
    list(APPEND DYNAMIC_LIB ${MESALINK_LIB})
else()
    list(APPEND DYNAMIC_LIB ${OPENSSL_SSL_LIBRARY})
endif()

set(BRPC_PRIVATE_LIBS "-lgflags -lprotobuf -lleveldb -lprotoc -lssl -lcrypto -ldl -lz")

    set(DYNAMIC_LIB ${DYNAMIC_LIB} ${GLOG_LIB})
    set(BRPC_PRIVATE_LIBS "${BRPC_PRIVATE_LIBS} -lglog")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(DYNAMIC_LIB ${DYNAMIC_LIB} rt)
    set(BRPC_PRIVATE_LIBS "${BRPC_PRIVATE_LIBS} -lrt")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(DYNAMIC_LIB ${DYNAMIC_LIB}
        pthread
        "-framework CoreFoundation"
        "-framework CoreGraphics"
        "-framework CoreData"
        "-framework CoreText"
        "-framework Security"
        "-framework Foundation"
        "-Wl,-U,_MallocExtension_ReleaseFreeMemory"
        "-Wl,-U,_ProfilerStart"
        "-Wl,-U,_ProfilerStop")
endif()

# for *.so
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/output/lib)
# for *.a
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/output/lib)

# the reason why not using file(GLOB_RECURSE...) is that we want to
# include different files on different platforms.
set(BUTIL_SOURCES
    ${PROJECT_SOURCE_DIR}/flare/butil/third_party/dmg_fp/g_fmt.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/third_party/dmg_fp/dtoa_wrapper.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/third_party/dynamic_annotations/dynamic_annotations.c
    ${PROJECT_SOURCE_DIR}/flare/butil/third_party/icu/icu_utf.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/third_party/superfasthash/superfasthash.c
    ${PROJECT_SOURCE_DIR}/flare/butil/third_party/modp_b64/modp_b64.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/third_party/symbolize/demangle.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/third_party/symbolize/symbolize.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/third_party/snappy/snappy-sinksource.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/third_party/snappy/snappy-stubs-internal.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/third_party/snappy/snappy.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/third_party/murmurhash3/murmurhash3.cpp
    ${PROJECT_SOURCE_DIR}/flare/butil/arena.cpp
    ${PROJECT_SOURCE_DIR}/flare/butil/at_exit.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/big_endian.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/debug/alias.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/debug/asan_invalid_access.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/debug/crash_logging.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/debug/debugger.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/debug/debugger_posix.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/debug/dump_without_crashing.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/debug/proc_maps_linux.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/debug/stack_trace.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/debug/stack_trace_posix.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/environment.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/files/file.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/files/file_posix.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/files/file_enumerator.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/files/file_enumerator_posix.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/files/file_path.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/files/file_path_constants.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/files/memory_mapped_file.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/files/memory_mapped_file_posix.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/files/scoped_file.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/files/scoped_temp_dir.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/file_util.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/file_util_posix.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/guid.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/guid_posix.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/hash.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/lazy_instance.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/memory/aligned_memory.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/memory/ref_counted.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/memory/ref_counted_memory.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/memory/singleton.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/memory/weak_ptr.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/process_util.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/rand_util.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/rand_util_posix.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/fast_rand.cpp
    ${PROJECT_SOURCE_DIR}/flare/butil/safe_strerror_posix.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/sha1_portable.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/strings/latin1_string_conversions.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/strings/nullable_string16.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/strings/safe_sprintf.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/strings/string16.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/strings/string_number_conversions.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/strings/string_split.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/strings/string_piece.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/strings/string_util.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/strings/string_util_constants.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/strings/stringprintf.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/strings/utf_offset_string_conversions.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/strings/utf_string_conversion_utils.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/strings/utf_string_conversions.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/synchronization/condition_variable_posix.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/synchronization/waitable_event_posix.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/threading/non_thread_safe_impl.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/threading/platform_thread_posix.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/threading/simple_thread.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/threading/thread_checker_impl.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/threading/thread_collision_warner.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/threading/thread_id_name_manager.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/threading/thread_local_posix.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/threading/thread_local_storage.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/threading/thread_local_storage_posix.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/threading/thread_restrictions.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/threading/watchdog.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/time/clock.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/time/default_clock.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/time/default_tick_clock.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/time/tick_clock.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/time/time.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/time/time_posix.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/version.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/logging.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/class_name.cpp
    ${PROJECT_SOURCE_DIR}/flare/butil/errno.cpp
    ${PROJECT_SOURCE_DIR}/flare/butil/find_cstr.cpp
    ${PROJECT_SOURCE_DIR}/flare/butil/status.cpp
    ${PROJECT_SOURCE_DIR}/flare/butil/string_printf.cpp
    ${PROJECT_SOURCE_DIR}/flare/butil/thread_local.cpp
    ${PROJECT_SOURCE_DIR}/flare/butil/unix_socket.cpp
    ${PROJECT_SOURCE_DIR}/flare/butil/endpoint.cpp
    ${PROJECT_SOURCE_DIR}/flare/butil/fd_utility.cpp
    ${PROJECT_SOURCE_DIR}/flare/butil/files/temp_file.cpp
    ${PROJECT_SOURCE_DIR}/flare/butil/files/file_watcher.cpp
    ${PROJECT_SOURCE_DIR}/flare/butil/time.cpp
    ${PROJECT_SOURCE_DIR}/flare/butil/zero_copy_stream_as_streambuf.cpp
    ${PROJECT_SOURCE_DIR}/flare/butil/crc32c.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/containers/case_ignored_flat_map.cpp
    ${PROJECT_SOURCE_DIR}/flare/butil/iobuf.cpp
    ${PROJECT_SOURCE_DIR}/flare/butil/binary_printer.cpp
    ${PROJECT_SOURCE_DIR}/flare/butil/recordio.cc
    ${PROJECT_SOURCE_DIR}/flare/butil/popen.cpp
    )

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(BUTIL_SOURCES ${BUTIL_SOURCES}
        ${PROJECT_SOURCE_DIR}/flare/butil/file_util_linux.cc
        ${PROJECT_SOURCE_DIR}/flare/butil/threading/platform_thread_linux.cc
        ${PROJECT_SOURCE_DIR}/flare/butil/strings/sys_string_conversions_posix.cc)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(BUTIL_SOURCES ${BUTIL_SOURCES}
        ${PROJECT_SOURCE_DIR}/flare/butil/mac/bundle_locations.mm
        ${PROJECT_SOURCE_DIR}/flare/butil/mac/foundation_util.mm
        ${PROJECT_SOURCE_DIR}/flare/butil/file_util_mac.mm
        ${PROJECT_SOURCE_DIR}/flare/butil/threading/platform_thread_mac.mm
        ${PROJECT_SOURCE_DIR}/flare/butil/strings/sys_string_conversions_mac.mm
        ${PROJECT_SOURCE_DIR}/flare/butil/time/time_mac.cc
        ${PROJECT_SOURCE_DIR}/flare/butil/mac/scoped_mach_port.cc)
endif()


file(GLOB_RECURSE FLARE_BASE_SRC "${PROJECT_SOURCE_DIR}/flare/base/*.cc")

file(GLOB_RECURSE BVAR_SOURCES "${PROJECT_SOURCE_DIR}/flare/bvar/*.cpp")
file(GLOB_RECURSE BTHREAD_SOURCES "${PROJECT_SOURCE_DIR}/flare/bthread/*.cpp")
file(GLOB_RECURSE JSON2PB_SOURCES "${PROJECT_SOURCE_DIR}/flare/json2pb/*.cpp")
file(GLOB_RECURSE BRPC_SOURCES "${PROJECT_SOURCE_DIR}/flare/brpc/*.cpp")
file(GLOB_RECURSE THRIFT_SOURCES "${PROJECT_SOURCE_DIR}/flare/brpc/thrift*.cpp")

if(WITH_THRIFT)
    message("brpc compile with thrift protocol")
else()
    # Remove thrift sources
    foreach(v ${THRIFT_SOURCES})
        list(REMOVE_ITEM BRPC_SOURCES ${v})
    endforeach()
    set(THRIFT_SOURCES "")
endif()

set(MCPACK2PB_SOURCES
    ${PROJECT_SOURCE_DIR}/flare/mcpack2pb/field_type.cpp
    ${PROJECT_SOURCE_DIR}/flare/mcpack2pb/mcpack2pb.cpp
    ${PROJECT_SOURCE_DIR}/flare/mcpack2pb/parser.cpp
    ${PROJECT_SOURCE_DIR}/flare/mcpack2pb/serializer.cpp)

include(CompileProto)
set(PROTO_FILES flare/idl_options.proto
        flare/brpc/rtmp.proto
        flare/brpc/rpc_dump.proto
        flare/brpc/get_favicon.proto
        flare/brpc/span.proto
        flare/brpc/builtin_service.proto
        flare/brpc/get_js.proto
        flare/brpc/errno.proto
        flare/brpc/nshead_meta.proto
        flare/brpc/options.proto
        flare/brpc/policy/baidu_rpc_meta.proto
        flare/brpc/policy/hulu_pbrpc_meta.proto
        flare/brpc/policy/public_pbrpc_meta.proto
        flare/brpc/policy/sofa_pbrpc_meta.proto
        flare/brpc/policy/mongo.proto
        flare/brpc/trackme.proto
        flare/brpc/streaming_rpc_meta.proto
        flare/brpc/proto_base.proto)

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/output/include/flare)
set(PROTOC_FLAGS ${PROTOC_FLAGS} -I${PROTOBUF_INCLUDE_DIR})
compile_proto(PROTO_HDRS PROTO_SRCS ${PROJECT_BINARY_DIR}
                                    ${PROJECT_BINARY_DIR}/output/include
                                    ${PROJECT_SOURCE_DIR}
                                    "${PROTO_FILES}")
add_library(PROTO_LIB OBJECT ${PROTO_SRCS} ${PROTO_HDRS})

set(SOURCES
    ${BVAR_SOURCES}
    ${BTHREAD_SOURCES}
    ${JSON2PB_SOURCES}
    ${MCPACK2PB_SOURCES}
    ${BRPC_SOURCES}
    ${THRIFT_SOURCES}
    )

add_subdirectory(flare)
if(BUILD_UNIT_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
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
configure_file(cmake/brpc.pc.in ${PROJECT_BINARY_DIR}/brpc.pc @ONLY)
install(FILES ${PROJECT_BINARY_DIR}/brpc.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
