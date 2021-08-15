add_library(smol STATIC
    # platform things
    precomp.cpp
    error.cpp
    platform.cpp
    # algorithms
    hash.cpp
    # types
    color.cpp
    utf8_string.cpp
    uuid.cpp
    # data structures
    serialization_buffer.cpp
    hash_buffer.cpp)

target_precompile_headers(smol PRIVATE precomp.hpp)

include_directories(../../smol/include)
include_directories(../../utf8proc/include)
include_directories(../../xxhash/include)
include_directories(../../fmt/include)
include_directories(../../json/include)
