cmake_minimum_required(VERSION 3.16.6)
project(TABLETOP_MIXER_ROOT)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/" ${CMAKE_MODULE_PATH})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE RELWITHDEBINFO)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Werror -Wfatal-errors -fconcepts")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-comment")

add_subdirectory(code)
