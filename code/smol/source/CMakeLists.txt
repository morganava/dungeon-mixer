add_library(smol STATIC
    precomp.cpp
    file.cpp
    error.cpp)

target_precompile_headers(smol PRIVATE precomp.hpp)

include_directories(../../smol/include)
include_directories(../../fmt/include)
