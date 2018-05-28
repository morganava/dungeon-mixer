/* nuklear - 1.32.0 - public domain */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <math.h>
#include <limits.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#include "../include/nuklear.h"
#include "nuklear_glfw_gl3.h"

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

#define UNUSED(a) (void)a
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#define LEN(a) (sizeof(a)/sizeof(a)[0])

static struct nk_callbacks callbacks;

static void nk_glfw3_error_callback(int e, const char *d)
{
    if(callbacks.error)
    {
        callbacks.error(d);
    }
}

#include <sys/time.h>

// get time in microseconds
static int64_t get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (int64_t)tv.tv_sec * (int64_t)1000000 + (int64_t)tv.tv_usec;
}

void nk_font_stash_begin(
    struct nk_font_atlas **atlas)
{
    return nk_glfw3_font_stash_begin(atlas);
}

void nk_font_stash_end(void)
{
    return nk_glfw3_font_stash_end();
}

struct nk_image
nk_load_image_from_memory(int width, int height, const void* data)
{
    return nk_glfw3_load_image_from_memory(width, height, data);
}

void
nk_main(
    const char* window_title,
    int width,
    int height,
    struct nk_callbacks callbackFunctions)
{
    const int WINDOW_WIDTH = width;
    const int WINDOW_HEIGHT = height;
    callbacks = callbackFunctions;

    /* Platform */
    static GLFWwindow *win;
    struct nk_context *ctx;
    struct nk_color background;

    /* GLFW */
    glfwSetErrorCallback(nk_glfw3_error_callback);
    if (!glfwInit()) 
    {
        if(callbacks.error) callbacks.error("GLFW failed to init");
        if(callbacks.close) callbacks.close(NULL);
        return;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    win = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, window_title, NULL, NULL);
    glfwMakeContextCurrent(win);
    glfwGetWindowSize(win, &width, &height);
    glfwSetWindowSizeLimits(win, width, height, GLFW_DONT_CARE, GLFW_DONT_CARE);

    /* OpenGL */
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glewExperimental = 1;
    if (glewInit() != GLEW_OK) 
    {
        if(callbacks.error) callbacks.error("GLEW failed to init");
        if(callbacks.close) callbacks.close(NULL);
        return;
    }

    ctx = nk_glfw3_init(win, NK_GLFW3_INSTALL_CALLBACKS);

    /* Init */
    if(callbacks.init)
    {
        callbacks.init(ctx);
    }

    int64_t prev_time = get_time();

    /* Magenta */
    background = nk_rgb(255,0,255);
    while (!glfwWindowShouldClose(win))
    {
        int64_t current_time = get_time();
        float dt = (current_time - prev_time) / 1000000.0f;
        ctx->delta_time_seconds = dt;
        prev_time = current_time;

        /* Input */
        glfwPollEvents();
        nk_glfw3_new_frame();

        /* Get Window Size prior to drawing widgets */
        glfwGetWindowSize(win, &width, &height);

        /* GUI */
        if(callbacks.update)
        {
            callbacks.update(dt, ctx, width, height);
        }

        /* Draw */
        {
            float bg[4];
            nk_color_fv(bg, background);
            glViewport(0, 0, width, height);
            glClear(GL_COLOR_BUFFER_BIT);
            glClearColor(bg[0], bg[1], bg[2], bg[3]);
            /* IMPORTANT: `nk_glfw_render` modifies some global OpenGL state
             * with blending, scissor, face culling, depth test and viewport and
             * defaults everything back into a default state.
             * Make sure to either a.) save and restore or b.) reset your own state after
             * rendering the UI. */
            nk_glfw3_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
            glfwSwapBuffers(win);
        }
    }
    if(callbacks.close) 
    {
        callbacks.close(ctx);
    }

    nk_glfw3_shutdown();
    glfwTerminate();
    return;
}
