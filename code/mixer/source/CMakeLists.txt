find_package(glfw3 3.2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SFML 2.4 REQUIRED COMPONENTS audio)
find_package(GTK3 3.0 REQUIRED gtk)

add_compile_options(-Wall)
add_compile_options(-Werror)
add_compile_options(-Wfatal-errors)
add_compile_options(-O3)
add_compile_options(-g)
add_compile_options(-std=c++17)

add_executable(tabletop_mixer
    precomp.cpp
    # utilities
    uuid.cpp
    tag_map.cpp
    text_buffer.cpp
    hash.cpp
    utf8_string.cpp
    utilities.cpp
    # main
    mixer.cpp
    # systems
    globals.cpp
    work_queue.cpp
    graphics.cpp
    input.cpp
    logger.cpp
    serialization.cpp
    string_manager.cpp
    geometry.cpp
    json.cpp
    nanovg.cpp
    imgui.cpp
    # audio backend
    audio_data.cpp
    audio_directory.cpp
    track_directory.cpp
    atmosphere_directory.cpp
    playback_manager.cpp
    # ui 
    ui.cpp)

add_precompiled_header(tabletop_mixer precomp.hpp SOURCE_CXX precomp.cpp FORCEINCLUDE)

target_link_libraries(tabletop_mixer xxhash)
target_link_libraries(tabletop_mixer utf8proc)
target_link_libraries(tabletop_mixer stb)
target_link_libraries(tabletop_mixer fmt)
target_link_libraries(tabletop_mixer glfw)
target_link_libraries(tabletop_mixer ${OPENGL_gl_LIBRARY})
target_link_libraries(tabletop_mixer ${OPENGL_glu_LIBRARY})
target_link_libraries(tabletop_mixer GLEW)
target_link_libraries(tabletop_mixer nanovg)
target_link_libraries(tabletop_mixer ${SFML_LIBRARIES})
target_link_libraries(tabletop_mixer nativefiledialog)
target_link_libraries(tabletop_mixer ${GTK3_LIBRARIES})
target_link_libraries(tabletop_mixer pthread)
target_link_libraries(tabletop_mixer stdc++fs)

include_directories(../../utf8proc/include)
include_directories(../../xxhash/include)
include_directories(../../json/include)
include_directories(../../stb/include)
include_directories(../../nativefiledialog/include)
include_directories(../../fmt/include)
include_directories(../../nanovg/include)
include_directories(${SFML_INCLUDE_DIR})