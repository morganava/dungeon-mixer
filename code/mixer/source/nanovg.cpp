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

    //  hash_buffer

    template<>
    void hash_buffer::push_back<rect>(const rect& val)
    {
        push_back<float>(val.position.x);
        push_back<float>(val.position.y);
        push_back<float>(val.size.x);
        push_back<float>(val.size.y);        
    }

    // NanoVG

    void nanovg::create_context()
    {
        // init nanovg
        _ctx = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
        MX_THROW_IF_NULL(_ctx);        
    }

    typedef enum draw_command : uint8_t
    {
        set_scissor,
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
                case draw_command::set_scissor:
                {
                    float pos_x, pos_y, size_x, size_y;
                    MX_THROW_IF_FALSE(cq.next(pos_x));
                    MX_THROW_IF_FALSE(cq.next(pos_y));
                    MX_THROW_IF_FALSE(cq.next(size_x));
                    MX_THROW_IF_FALSE(cq.next(size_y));

                    nvgScissor(_ctx, pos_x, pos_y, size_x, size_y);
                }
                break;
                case draw_command::fill_rect:
                {
                    float pos_x, pos_y, size_x, size_y;
                    MX_THROW_IF_FALSE(cq.next(pos_x));
                    MX_THROW_IF_FALSE(cq.next(pos_y));
                    MX_THROW_IF_FALSE(cq.next(size_x));
                    MX_THROW_IF_FALSE(cq.next(size_y));

                    NVGcolor col;
                    MX_THROW_IF_FALSE(cq.next(col))

                    nvgBeginPath(_ctx);
                    nvgRect(_ctx, pos_x, pos_y, size_x, size_y);
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

    void nanovg::set_scissor(const rect& r)
    {
        auto& cq = command_queue.current;
        cq.push_back(draw_command::set_scissor);
        cq.push_back(r);
    }

    void nanovg::fill_rect(const rect& r, const NVGcolor& col)
    {
        auto& cq = command_queue.current;
        cq.push_back(draw_command::fill_rect);
        cq.push_back(r);
        cq.push_back(col);
    }
}