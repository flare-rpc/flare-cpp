
include(FindThreads)
include(FindProtobuf)
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER echo.proto)
# include PROTO_HEADER
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(streaming_echo_client client.cc ${PROTO_SRC} ${PROTO_HEADER})
add_executable(streaming_echo_server server.cc ${PROTO_SRC} ${PROTO_HEADER})

target_link_libraries(streaming_echo_client ${FLARE_LIB} ${DYNAMIC_LIB})
target_link_libraries(streaming_echo_server ${FLARE_LIB} ${DYNAMIC_LIB})
