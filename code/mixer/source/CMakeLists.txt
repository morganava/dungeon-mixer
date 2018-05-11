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
    globals.cpp
    mixer.cpp
    audio_data.cpp
    audio_directory.cpp
    track_directory.cpp
    atmosphere_directory.cpp
    playback_manager.cpp
    draw.cpp
    work_queue.cpp
    logger.cpp
    utilities.cpp
    serialization.cpp
    nuklear_extensions.cpp
    uuid.cpp
    tag_map.cpp
    text_buffer.cpp
    hash.cpp
    font_stash.cpp)

add_precompiled_header(tabletop_mixer precomp.hpp SOURCE_CXX precomp.cpp FORCEINCLUDE)

target_link_libraries(tabletop_mixer xxhash)
target_link_libraries(tabletop_mixer nuklear)
target_link_libraries(tabletop_mixer glfw)
target_link_libraries(tabletop_mixer ${OPENGL_gl_LIBRARY})
target_link_libraries(tabletop_mixer ${OPENGL_glu_LIBRARY})
target_link_libraries(tabletop_mixer GLEW)
target_link_libraries(tabletop_mixer ${SFML_LIBRARIES})
target_link_libraries(tabletop_mixer nativefiledialog)
target_link_libraries(tabletop_mixer ${GTK3_LIBRARIES})
target_link_libraries(tabletop_mixer pthread)
target_link_libraries(tabletop_mixer stdc++fs)

include_directories(../../xxhash/include)
include_directories(../../json/include)
include_directories(../../nuklear/include)
include_directories(${SFML_INCLUDE_DIR})
include_directories(../../nativefiledialog/include)
