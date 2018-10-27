add_compile_options(-Wall)
add_compile_options(-Wfatal-errors)
add_compile_options(-O3)
add_compile_options(-g)

add_library(stb STATIC
    stb_image.c
    stb_truetype.c)

include_directories(../include)