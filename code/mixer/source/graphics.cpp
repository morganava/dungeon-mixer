#include "graphics.hpp"
#include "utilities.hpp"
#include "logger.hpp"
#include "utf8_string.hpp"
#include "geometry.hpp"
#include "input.hpp"
#include "singleton.hpp"
#include "hash.hpp"
#include "nanovg.hpp"

namespace tabletopify
{
    GLFWwindow* g_window = nullptr;
    graphics_callbacks g_callbacks = {};
    int g_window_width = 0;
    int g_window_height = 0;
    int g_framebuffer_width = 0;
    int g_framebuffer_height = 0;
    double g_pixel_ratio = 0.0;

    static modifier_keys GLFW_MOD_to_modifier_keys(int32_t mods)
    {
        modifier_keys retval;
        retval.shift = mods & GLFW_MOD_SHIFT;
        retval.control = mods & GLFW_MOD_CONTROL;
        retval.alt = mods & GLFW_MOD_ALT;
        retval.super = mods & GLFW_MOD_SUPER;
        retval.num_lock = mods & GLFW_MOD_NUM_LOCK;
        return retval;
    }

    static void graphics_error_func(int error, const char* description)
    {
        MX_LOG_ERROR("GLFW Error {:08x} : {}", error, description);
    }

    static void graphics_key_func(GLFWwindow*, int32_t key, int32_t scancode, int32_t action, int32_t mods)
    {
        // handle num lock keys
        if (mods & GLFW_MOD_NUM_LOCK)
        {
            // MX_LOG_INFO("NUM LOCK");
        }

        switch(action)
        {
            case GLFW_PRESS:
                g_input.push_keydown(key, GLFW_MOD_to_modifier_keys(mods));
                break;
            case GLFW_REPEAT:
                g_input.push_keyrepeat(key, GLFW_MOD_to_modifier_keys(mods));
                break;
            case GLFW_RELEASE:
                g_input.push_keyup(key, GLFW_MOD_to_modifier_keys(mods));
                break;
        }
    }

    static void graphics_charmods_func(GLFWwindow*, uint32_t codepoint, int32_t mods)
    {
        g_input.push_charpress(codepoint, GLFW_MOD_to_modifier_keys(mods));
    }

    static void graphics_cursor_pos_func(GLFWwindow* window, double xpos, double ypos)
    {
        g_input.push_mousemove({xpos, ypos});
    }

    static void graphics_mouse_button_func(GLFWwindow* window, int button, int action, int mods)
    {
        switch(action)
        {
            case GLFW_PRESS:
                g_input.push_mousedown(static_cast<mouse_button_t>(button), GLFW_MOD_to_modifier_keys(mods));
                break;
            case GLFW_RELEASE:
                g_input.push_mouseup(static_cast<mouse_button_t>(button), GLFW_MOD_to_modifier_keys(mods));
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

    static void graphics_scroll_func(GLFWwindow* window, double xoffset, double yoffset)
    {
        g_input.push_scroll(xoffset, yoffset);
    }

    static void graphics_window_size_func(GLFWwindow* window, int width, int height)
    {
        g_window_width = width;
        g_window_height = height;
        g_pixel_ratio = (double)g_framebuffer_width / (double)g_window_width;
    }

    static void graphics_framebuffer_size_func(GLFWwindow* window, int width, int height)
    {
        g_framebuffer_width = width;
        g_framebuffer_height = height;
        g_pixel_ratio = (double)g_framebuffer_width / (double)g_window_width;

        glViewport(0, 0, width, height);
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

            g_window = glfwCreateWindow(1024, 768, "Tabletopify", NULL, NULL);
            MX_THROW_IF_NULL(g_window);
            glfwMakeContextCurrent(g_window);
            glfwSwapInterval(1);

            // get initial window stats
            glfwGetWindowSize(g_window, &g_window_width, &g_window_height);
            glfwGetFramebufferSize(g_window, &g_framebuffer_width, &g_framebuffer_height);
            g_pixel_ratio = (double)g_framebuffer_width / (double)g_window_width;
            glViewport(0, 0, g_framebuffer_width, g_framebuffer_height);

            // set input callbacks
            MX_THROW_IF_NOT_NULL(glfwSetKeyCallback(g_window, &graphics_key_func));
            MX_THROW_IF_NOT_NULL(glfwSetCharModsCallback(g_window, &graphics_charmods_func));
            MX_THROW_IF_NOT_NULL(glfwSetCursorPosCallback(g_window, &graphics_cursor_pos_func));
            MX_THROW_IF_NOT_NULL(glfwSetMouseButtonCallback(g_window, &graphics_mouse_button_func));
            MX_THROW_IF_NOT_NULL(glfwSetCursorEnterCallback(g_window, &graphics_mouse_enter_func));
            MX_THROW_IF_NOT_NULL(glfwSetScrollCallback(g_window, &graphics_scroll_func));
            glfwSetInputMode(g_window, GLFW_LOCK_KEY_MODS, GLFW_TRUE);

            // graphics callbacks
            MX_THROW_IF_NOT_NULL(glfwSetWindowSizeCallback(g_window, &graphics_window_size_func));
            MX_THROW_IF_NOT_NULL(glfwSetFramebufferSizeCallback(g_window, &graphics_framebuffer_size_func));

            // init glew
            glewExperimental = GL_TRUE;
            MX_THROW_IF_FALSE(glewInit() == GLEW_OK);
            // GLEW generates GL error because it calls glGetString(GL_EXTENSIONS), we'll consume it here.
            glGetError();

            if (callbacks.init) {
                callbacks.init();
            }

            // set our clear color
            glClearColor(1.0f, 0.0f, 1.0f, 0.0f);

            // update loop
            glfwSetTime(0);
            double prev_t = glfwGetTime();

            // framerate_calculator<128> fc;
            // double time_since_framerate_print = 0.0;

            while (!glfwWindowShouldClose(g_window))
            {
                 // calculate frame delta
                double t = glfwGetTime();
                double dt = t - prev_t;
                prev_t = t;

                // calculate framerate
                // fc.push_dt(dt);
                // time_since_framerate_print += dt;
                // if(time_since_framerate_print > 1.0)
                // {
                //     MX_LOG_INFO("FPS : {}", fc.framerate());
                //     time_since_framerate_print = 0.0;
                // }

                if (callbacks.update) {
                    callbacks.update(dt, g_window_width, g_window_height, g_pixel_ratio);
                }

               // get input events
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
            callbacks.close();
        }
    }
}