cmake_minimum_required(VERSION 3.2.2)
project(TABLETOP_MIXER_ROOT)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/" ${CMAKE_MODULE_PATH})

include("${CMAKE_SOURCE_DIR}/cmake_modules/PrecompiledHeader.cmake")

add_subdirectory(code)
