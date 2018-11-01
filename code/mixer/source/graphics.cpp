#include "graphics.hpp"
#include "utilities.hpp"
#include "logger.hpp"

namespace tabletopify
{
    static GLFWwindow* g_window = nullptr;
    static NVGcontext* g_context =  nullptr;
    static graphics_callbacks g_callbacks = {0};    

    static void graphics_error_func(int error, const char* description)
    {
        MX_LOG_ERROR("GLFW Error {:08x} : {}", error, description);
    }

    void graphics_main(const graphics_callbacks& callbacks) 
    {
        try
        {
            g_callbacks = callbacks;

            (void)g_window;
            // init glfw
            MX_THROW_IF_FALSE(glfwInit());
            MX_THROW_IF_NOT_NULL(glfwSetErrorCallback(&graphics_error_func));
        }
        catch(const std::exception& ex) 
        {
            MX_LOG_ERROR(ex.what());
            return;
        }

        try 
        {
           // create window
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);

            g_window = glfwCreateWindow(800, 600, "Tabletopify", NULL, NULL);
            MX_THROW_IF_NULL(g_window);
            glfwMakeContextCurrent(g_window);
            glfwSwapInterval(0);

            // init glew
            glewExperimental = GL_TRUE;
            MX_THROW_IF_FALSE(glewInit() == GLEW_OK);
            // GLEW generates GL error because it calls glGetString(GL_EXTENSIONS), we'll consume it here.
            glGetError();

            // init nanovg
            g_context = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
            MX_THROW_IF_NULL(g_context);

            if (callbacks.init) {
                callbacks.init(g_context);
            }

            // update loop
            glfwSetTime(0);
            double prev_t = glfwGetTime();            
            while (!glfwWindowShouldClose(g_window))
            {
                // calculate frame delta
                double t = glfwGetTime();
                double dt = t - prev_t;
                prev_t = t;

                // get window stats
                int window_width, window_height;
                int fb_width, fb_height;
                glfwGetWindowSize(g_window, &window_width, &window_height);
                glfwGetFramebufferSize(g_window, &fb_width, &fb_height);
                float pixel_ratio = (float)fb_width / (float)window_width;

                glViewport(0, 0, fb_width, fb_height);
                glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

                nvgBeginFrame(g_context, window_width, window_height, pixel_ratio);
                if (callbacks.update) {
                    callbacks.update(g_context, dt, window_width, window_height);
                }
                nvgEndFrame(g_context);

                glfwSwapBuffers(g_window);
                glfwPollEvents();
            }

            glfwTerminate();
        }
        catch(const std::exception& ex) 
        {
            MX_LOG_ERROR(ex.what());
            glfwTerminate();
            return;
        }
        
        // close
        if (callbacks.close) {
            callbacks.close(g_context);
        }
    }
}