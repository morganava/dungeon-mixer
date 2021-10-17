#include "graphics.hpp"
#include "logger.hpp"
#include "input.hpp"
#include "service.hpp"
#include "nanovg.hpp"
#include "ui.hpp"
#include "globals.hpp"

namespace dm
{
    // compile hack for input manager
    GLFWwindow* g_window = nullptr;

    modifier_keys graphics::GLFW_MOD_to_modifier_keys(int32_t mods)
    {
        modifier_keys retval;
        retval.shift = mods & GLFW_MOD_SHIFT;
        retval.control = mods & GLFW_MOD_CONTROL;
        retval.alt = mods & GLFW_MOD_ALT;
        retval.super = mods & GLFW_MOD_SUPER;
        retval.num_lock = mods & GLFW_MOD_NUM_LOCK;
        return retval;
    }

    void graphics::error_func(int error, const char* description)
    {
        MX_LOG_ERROR("GLFW Error {:08x} : {}", error, description);
    }

    void graphics::key_func(GLFWwindow*, int32_t key, int32_t scancode, int32_t action, int32_t mods)
    {
        switch(action)
        {
            case GLFW_PRESS:
                g_input->push_keydown(key, GLFW_MOD_to_modifier_keys(mods));
                break;
            case GLFW_REPEAT:
                g_input->push_keyrepeat(key, GLFW_MOD_to_modifier_keys(mods));
                break;
            case GLFW_RELEASE:
                g_input->push_keyup(key, GLFW_MOD_to_modifier_keys(mods));
                break;
        }
    }

    void graphics::charmods_func(GLFWwindow*, uint32_t codepoint, int32_t mods)
    {
        g_input->push_charpress(codepoint, GLFW_MOD_to_modifier_keys(mods));
    }

    void graphics::cursor_pos_func(GLFWwindow* window, double xpos, double ypos)
    {
        g_input->push_mousemove({xpos, ypos});
    }

    void graphics::mouse_button_func(GLFWwindow* window, int button, int action, int mods)
    {
        switch(action)
        {
            case GLFW_PRESS:
                g_input->push_mousedown(static_cast<mouse_button_t>(button), GLFW_MOD_to_modifier_keys(mods));
                break;
            case GLFW_RELEASE:
                g_input->push_mouseup(static_cast<mouse_button_t>(button), GLFW_MOD_to_modifier_keys(mods));
                break;
        }
    }

    void graphics::mouse_enter_func(GLFWwindow* window, int entered)
    {
        if (!entered)
        {
            graphics::cursor_pos_func(window, -1.0, -1.0);
        }
    }

    void graphics::scroll_func(GLFWwindow* window, double xoffset, double yoffset)
    {
        g_input->push_scroll(xoffset, yoffset);
    }

    void graphics::window_size_func(GLFWwindow* window, int width, int height)
    {
        g_graphics->window_width = width;
        g_graphics->window_height = height;
        g_graphics->pixel_ratio = (double)g_graphics->framebuffer_width / (double)g_graphics->window_width;
    }

    void graphics::framebuffer_size_func(GLFWwindow* window, int width, int height)
    {
        g_graphics->framebuffer_width = width;
        g_graphics->framebuffer_height = height;
        g_graphics->pixel_ratio = (double)g_graphics->framebuffer_width / (double)g_graphics->window_width;

        glViewport(0, 0, width, height);
    }

    void graphics::window_refresh_func(GLFWwindow* window)
    {
        g_graphics->update();
    }

    graphics::graphics()
    {
        // init glfw and set error callback
        SMOL_THROW_IF_FALSE(glfwInit());
        SMOL_THROW_IF_NOT_NULL(glfwSetErrorCallback(&error_func));

        // create window
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);

        glfwWindowHint(GLFW_SAMPLES, 4);

        window = glfwCreateWindow(1024, 768, "Dungeon Mixer", NULL, NULL);
        // todo: remove me please thanks
        g_window = window;
        SMOL_THROW_IF_NULL(window);
        glfwMakeContextCurrent(window);

        // init glew
        glewExperimental = GL_TRUE;
        SMOL_THROW_IF_FALSE(glewInit() == GLEW_OK);
        // GLEW generates GL error because it calls glGetString(GL_EXTENSIONS), we'll consume it here.
        glGetError();

        glfwSwapInterval(0);

        // get initial window stats
        glfwGetWindowSize(window, &window_width, &window_height);
        glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
        pixel_ratio = (double)framebuffer_width / (double)window_width;
        glViewport(0, 0, framebuffer_width, framebuffer_height);

        // set input callbacks
        SMOL_THROW_IF_NOT_NULL(glfwSetKeyCallback(window, &key_func));
        SMOL_THROW_IF_NOT_NULL(glfwSetCharModsCallback(window, &charmods_func));
        SMOL_THROW_IF_NOT_NULL(glfwSetCursorPosCallback(window, &cursor_pos_func));
        SMOL_THROW_IF_NOT_NULL(glfwSetMouseButtonCallback(window, &mouse_button_func));
        SMOL_THROW_IF_NOT_NULL(glfwSetCursorEnterCallback(window, &mouse_enter_func));
        SMOL_THROW_IF_NOT_NULL(glfwSetScrollCallback(window, &scroll_func));
        glfwSetInputMode(window, GLFW_LOCK_KEY_MODS, GLFW_TRUE);

        // graphics callbacks
        SMOL_THROW_IF_NOT_NULL(glfwSetWindowSizeCallback(window, &window_size_func));
        SMOL_THROW_IF_NOT_NULL(glfwSetFramebufferSizeCallback(window, &framebuffer_size_func));
        SMOL_THROW_IF_NOT_NULL(glfwSetWindowRefreshCallback(window, &window_refresh_func));

        // set our clear color
        glClearColor(1.0f, 0.0f, 1.0f, 0.0f);

        // init our timestamp
        glfwSetTime(0);

        worker_ui.enqueue([]() -> void {
            g_input_unsafe.build_key_map();
            g_input_unsafe.init_cursors();
            g_nanovg_unsafe.create_context();

            g_graphics_unsafe.update();
        });
    }

    graphics::~graphics()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void graphics::update()
    {
         // calculate frame delta
        double time = glfwGetTime();
        double dt = time - previousTime;
        previousTime = time;

        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

        // update our various systems
        g_input_unsafe.update(dt);
        g_ui_unsafe.update(dt, window_width, window_height);
        g_nanovg_unsafe.update(dt, window_width, window_height, pixel_ratio);
        g_input_unsafe.update_cursor();

        glfwSwapBuffers(window);

        // get input events
        if (!handlingEvents) {
            // update can be called from within glfwPollEvents within
            // one of our callbacks, so make sure we do not call glfwPollEvents()
            // from within itself
            handlingEvents = true;
            glfwPollEvents();
            handlingEvents = false;

            if (!glfwWindowShouldClose(window))
            {
                // enqueue next update
                worker_ui.enqueue([]() -> void {
                    g_graphics_unsafe.update();
                });
            }
            else
            {
                // signal termination
                g_work_manager->signal_termination();
            }
        }
    }
}
