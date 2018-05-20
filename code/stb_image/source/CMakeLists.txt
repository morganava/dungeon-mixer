add_compile_options(-Wall)
add_compile_options(-Wfatal-errors)
add_compile_options(-O3)
add_compile_options(-g)

add_library(stb_image STATIC
    stb_image.c)

include_directories(../include)