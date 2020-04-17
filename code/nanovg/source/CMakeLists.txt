add_compile_options(-Wall)
add_compile_options(-Werror)
add_compile_options(-Wfatal-errors)
add_compile_options(-O3)
add_compile_options(-g)

add_library(nanovg STATIC
    nanovg.c
    nanovg_gl.c)

include_directories(../include)
include_directories(../../stb/include)
include_directories(../../glfw/include)

target_link_libraries(nanovg stb)
