
include(FindThreads)
include(FindProtobuf)
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER echo.proto)
# include PROTO_HEADER
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(parallel_echo_client client.cc ${PROTO_SRC} ${PROTO_HEADER})
add_executable(parallel_echo_server server.cc ${PROTO_SRC} ${PROTO_HEADER})

target_link_libraries(parallel_echo_client ${FLARE_LIB} ${DYNAMIC_LIB} ${GPERFTOOLS_LIBRARIES})
target_link_libraries(parallel_echo_server ${FLARE_LIB} ${DYNAMIC_LIB} ${GPERFTOOLS_LIBRARIES})
