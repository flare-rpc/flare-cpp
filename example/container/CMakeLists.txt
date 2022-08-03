
# include PROTO_HEADER
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(example_dynamic_bitset dynamic_bitset.cc)
target_link_libraries(example_dynamic_bitset ${FLARE_LIB} ${DYNAMIC_LIB})
