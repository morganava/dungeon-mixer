add_compile_options(-Wall)
add_compile_options(-Wfatal-errors)
add_compile_options(-O3)
add_compile_options(-g)

add_library(fmt STATIC
    format.cc
    posix.cc)

include_directories(../include)