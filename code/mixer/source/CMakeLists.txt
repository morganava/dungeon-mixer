find_package(OpenGL REQUIRED COMPONENTS OpenGL)
find_package(GLEW REQUIRED)
find_package(SFML 2.4 REQUIRED COMPONENTS audio)
find_package(GTK3 3.0 REQUIRED gtk)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wfatal-errors -fconcepts")

add_executable(tabletop_mixer
    precomp.cpp
    # utilities
    tag_map.cpp
    hash.cpp
    utf8_string.cpp
    utilities.cpp
    format.cpp
    json.cpp
    serialization_buffer.cpp
    linear_constraint_solver.cpp
    color.cpp
    random.cpp
    uuid.cpp
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
    nanovg.cpp
    stylesheet.cpp
    # imgui
    imgui.cpp
    imgui.scroll_area.cpp
    imgui.flex_box.cpp
    imgui.label.cpp
    imgui.button.cpp
    imgui.text_box.cpp
    imgui.slider.cpp
    imgui.checkbox.cpp
    imgui.context_menu.cpp
    # audio backend
    audio_data.cpp
    audio_directory.cpp
    track_directory.cpp
    atmosphere_directory.cpp
    playlist_directory.cpp
    playback_manager.cpp
    # ui
    ui.cpp
    ui.playlist.cpp
    ui.edit.cpp)

target_precompile_headers(tabletop_mixer PRIVATE precomp.hpp)

target_link_libraries(tabletop_mixer xxhash)
target_link_libraries(tabletop_mixer utf8proc)
target_link_libraries(tabletop_mixer stb)
target_link_libraries(tabletop_mixer fmt)
target_link_libraries(tabletop_mixer glfw)
target_link_libraries(tabletop_mixer OpenGL::OpenGL)
target_link_libraries(tabletop_mixer ${OPENGL_glu_LIBRARY})
target_link_libraries(tabletop_mixer GLEW)
target_link_libraries(tabletop_mixer nanovg)
target_link_libraries(tabletop_mixer ${SFML_LIBRARIES})
target_link_libraries(tabletop_mixer nativefiledialog)
target_link_libraries(tabletop_mixer ${GTK3_LIBRARIES})
target_link_libraries(tabletop_mixer pthread)
target_link_libraries(tabletop_mixer stdc++fs)

include_directories(../../glfw/include)
include_directories(../../utf8proc/include)
include_directories(../../xxhash/include)
include_directories(../../json/include)
include_directories(../../stb/include)
include_directories(../../nativefiledialog/include)
include_directories(../../fmt/include)
include_directories(../../nanovg/include)
include_directories(${SFML_INCLUDE_DIR})