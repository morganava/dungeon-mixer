#include "graphics.hpp"
#include "utilities.hpp"
#include "logger.hpp"
#include "utf8_string.hpp"
#include "geometry.hpp"
#include "input.hpp"

namespace tabletopify
{
    static GLFWwindow* g_window = nullptr;
    static NVGcontext* g_context =  nullptr;
    static graphics_callbacks g_callbacks = {0};    

    static void graphics_error_func(int error, const char* description)
    {
        MX_LOG_ERROR("GLFW Error {:08x} : {}", error, description);
    }

    static void graphics_key_func(GLFWwindow*, int32_t key, int32_t scancode, int32_t action, int32_t mods)
    {
        switch(action)
        {
            case GLFW_PRESS:
                get_input()->push_keydown(key);
                break;
            case GLFW_REPEAT:
                break;
            case GLFW_RELEASE:
                get_input()->push_keyup(key);
                break;
        }
    }

    static void graphics_charmods_func(GLFWwindow*, uint32_t codepoint, int32_t mods)
    {
        get_input()->push_keypress(codepoint);
    }

    static void graphics_cursor_pos_func(GLFWwindow* window, double xpos, double ypos)
    {
        get_input()->push_mousemove({xpos, ypos});
    }

    static void graphics_mouse_buttons_func(GLFWwindow* window, int button, int action, int mods)
    {
        switch(action)
        {
            case GLFW_PRESS:
                get_input()->push_mousedown(static_cast<mouse_button_t>(button));
                break;
            case GLFW_RELEASE:
                get_input()->push_mouseup(static_cast<mouse_button_t>(button));
                break;
        }
    }

    static void graphics_mouse_enter_func(GLFWwindow* window, int entered)
    {
        if (!entered) 
        {
            graphics_cursor_pos_func(window, -1.0, -1.0);
        }
    }

    void graphics_main(const graphics_callbacks& callbacks) 
    {
        try
        {
            g_callbacks = callbacks;

            // init glfw and set error callback
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
            glfwSwapInterval(1);

            // set input callbacks
            MX_THROW_IF_NOT_NULL(glfwSetKeyCallback(g_window, &graphics_key_func));
            MX_THROW_IF_NOT_NULL(glfwSetCharModsCallback(g_window, &graphics_charmods_func));
            MX_THROW_IF_NOT_NULL(glfwSetCursorPosCallback(g_window, &graphics_cursor_pos_func));
            MX_THROW_IF_NOT_NULL(glfwSetMouseButtonCallback(g_window, &graphics_mouse_buttons_func));
            MX_THROW_IF_NOT_NULL(glfwSetCursorEnterCallback(g_window, &graphics_mouse_enter_func));

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
                // get input events
                glfwPollEvents();
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