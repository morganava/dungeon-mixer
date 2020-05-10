add_library(nanovg STATIC
    nanovg.c
    nanovg_gl.c)

include_directories(../include)
include_directories(../../stb/include)
include_directories(../../glfw/include)

target_link_libraries(nanovg stb)
