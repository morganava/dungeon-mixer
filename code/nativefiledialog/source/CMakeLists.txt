find_package(GTK3 3.0 REQUIRED gtk)

add_compile_options(-Wall)
add_compile_options(-Werror)
add_compile_options(-Wfatal-errors)
add_compile_options(-O3)
add_compile_options(-g)

add_library(nativefiledialog STATIC
    nfd_common.c
    nfd_gtk.c)

include_directories(../include)
include_directories(${GTK3_INCLUDE_DIRS})