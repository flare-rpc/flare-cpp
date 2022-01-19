file(REMOVE_RECURSE
  "../output/lib/libbrpc.dylib"
  "../output/lib/libbrpc.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C CXX)
  include(CMakeFiles/brpc-shared.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
