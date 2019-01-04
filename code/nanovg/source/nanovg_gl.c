#include <GL/glew.h>

#ifdef __APPLE__
#   define GLFW_INCLUDE_GLCOREARB
#endif
// #define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

#include <nanovg.h>
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg_gl.h>
