find_package(glfw3 3.2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SFML 2.4 REQUIRED COMPONENTS audio)

add_compile_options(-Wall)
add_compile_options(-Werror)
add_compile_options(-Wfatal-errors)
add_compile_options(-O3)
add_compile_options(-g)

add_executable(tabletop_mixer
    mixer.cpp)

target_link_libraries(tabletop_mixer nuklear)
target_link_libraries(tabletop_mixer glfw)
target_link_libraries(tabletop_mixer ${OPENGL_gl_LIBRARY})
target_link_libraries(tabletop_mixer ${OPENGL_glu_LIBRARY})
target_link_libraries(tabletop_mixer GLEW)
target_link_libraries(tabletop_mixer ${SFML_LIBRARIES})

include_directories(../../nuklear/include)    
include_directories(${SFML_INCLUDE_DIR})