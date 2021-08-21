#pragma once

#include "work_queue.hpp"
#include "input.hpp"

namespace dm
{
    template<size_t N>
    struct framerate_calculator
    {
        void push_dt(double dt)
        {
            sum += dt - _dt_buffer[_idx];
            _dt_buffer[_idx] = dt;

            _idx = (_idx + 1) % N;
        }

        double framerate() const
        {
            return (double)N / sum;
        }
    private:
        double sum = 0.0;
        double _dt_buffer[N] = {};
        size_t _idx = 0;
    };

    // new hotness
    class graphics
    {
    public:
        graphics();
        ~graphics();
    private:
        void update();

        static modifier_keys GLFW_MOD_to_modifier_keys(int32_t mods);
        static void error_func(int error, const char* description);
        static void key_func(GLFWwindow*, int32_t key, int32_t scancode, int32_t action, int32_t mods);
        static void charmods_func(GLFWwindow*, uint32_t codepoint, int32_t mods);
        static void cursor_pos_func(GLFWwindow* window, double xpos, double ypos);
        static void mouse_button_func(GLFWwindow* window, int button, int action, int mods);
        static void mouse_enter_func(GLFWwindow* window, int entered);
        static void scroll_func(GLFWwindow* window, double xoffset, double yoffset);
        static void window_size_func(GLFWwindow* window, int width, int height);
        static void framebuffer_size_func(GLFWwindow* window, int width, int height);

        GLFWwindow* window = nullptr;
        int window_width = 0;
        int window_height = 0;
        int framebuffer_width = 0;
        int framebuffer_height = 0;
        double pixel_ratio = 0.0;
        double previousTime = 0.0;
    };
    MX_MAKE_SINGLETON(graphics, worker_flag::ui);
}