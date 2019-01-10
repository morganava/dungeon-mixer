#include "geometry.hpp"
#include "hash.hpp"
#include "utilities.hpp"
#include "nanovg.hpp"
#include "singleton.hpp"

namespace tabletopify
{
    extern GLFWwindow* g_window;

    nanovg* get_nanovg()
    {
        return singleton<nanovg>::addr();
    }

    void init_nanovg()
    {
        singleton<nanovg>::construct();
    }

    void free_nanovg()
    {
        singleton<nanovg>::free();
    }

    void nanovg::create_context()
    {
        // init nanovg
        _ctx = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
        MX_THROW_IF_NULL(_ctx);        
    }

    typedef enum draw_command : uint8_t
    {
        fill_rect,
    } draw_command_t;

    void nanovg::update(float dt, double window_width, double window_height, double pixel_ratio)
    {
        if(command_queue.current == command_queue.previous)
            return;

        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

        nvgBeginFrame(_ctx, window_width, window_height, pixel_ratio);

        draw_command_t cmd;
        auto& cq = command_queue.current;
        while(cq.next(cmd))
        {
            switch(cmd)
            {
                case draw_command::fill_rect:
                {
                    rect r;
                    NVGcolor col;
                    MX_THROW_IF_FALSE(cq.next(r))
                    MX_THROW_IF_FALSE(cq.next(col))

                    nvgBeginPath(_ctx);
                    nvgRect(_ctx, r.position.x, r.position.y, r.size.x, r.size.y);
                    nvgFillColor(_ctx, col);
                    nvgFill(_ctx);
                }
                break;
            }
        }

        nvgEndFrame(_ctx);
        glfwSwapBuffers(g_window);

        std::swap(command_queue.current, command_queue.previous);
        command_queue.current.clear();
    }

    void nanovg::fill_rect(const rect& r, const NVGcolor& col)
    {
        auto& cq = command_queue.current;
        cq.push_back(draw_command::fill_rect);
        cq.push_back(r);
        cq.push_back(col);
    }
}