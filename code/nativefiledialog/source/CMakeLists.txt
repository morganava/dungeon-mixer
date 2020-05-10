find_package(GTK3 3.0 REQUIRED gtk)

add_library(nativefiledialog STATIC
    nfd_common.c
    nfd_gtk.c)

include_directories(../include)
include_directories(${GTK3_INCLUDE_DIRS})