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

if (NOT DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNDEBUG")
endif ()

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(FLARE_BASE_LIB ${FLARE_SRC})
add_library(SOURCES_LIB OBJECT ${SOURCES})
add_dependencies(SOURCES_LIB PROTO_LIB)

# shared library needs POSITION_INDEPENDENT_CODE
set_property(TARGET ${SOURCES_LIB} PROPERTY POSITION_INDEPENDENT_CODE 1)
set_property(TARGET ${FLARE_BASE_LIB} PROPERTY POSITION_INDEPENDENT_CODE 1)

add_library(flare-shared SHARED  $<TARGET_OBJECTS:FLARE_BASE_LIB>
        $<TARGET_OBJECTS:SOURCES_LIB>
        $<TARGET_OBJECTS:PROTO_LIB>)
add_library(flare-static STATIC
        $<TARGET_OBJECTS:FLARE_BASE_LIB>
        $<TARGET_OBJECTS:SOURCES_LIB>
        $<TARGET_OBJECTS:PROTO_LIB>)

target_link_libraries(flare-shared ${DYNAMIC_LIB})


if (FLARE_WITH_THRIFT)
    target_link_libraries(flare-shared thrift)
    target_link_libraries(flare-static thrift)
endif ()

SET_TARGET_PROPERTIES(flare-static PROPERTIES OUTPUT_NAME flare CLEAN_DIRECT_OUTPUT 1)
SET_TARGET_PROPERTIES(flare-shared PROPERTIES OUTPUT_NAME flare CLEAN_DIRECT_OUTPUT 1)

# for protoc-gen-mcpack
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/output/bin)

set(protoc_gen_mcpack_SOURCES
        ${PROJECT_SOURCE_DIR}/flare/mcpack2pb/generator.cpp
        )
add_executable(protoc-gen-mcpack ${protoc_gen_mcpack_SOURCES})
target_link_libraries(protoc-gen-mcpack flare-shared)

#install directory
install(TARGETS flare-shared
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )

install(TARGETS flare-static
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
