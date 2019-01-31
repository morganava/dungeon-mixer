#include "utf8_string.hpp"
#include "utilities.hpp"
#include "geometry.hpp"
#include "input.hpp"
#include "singleton.hpp"
#include "hash.hpp"
#include "nanovg.hpp"
#include "imgui.hpp"
#include "ui.hpp"
#include "singleton.hpp"

namespace tabletopify
{
    void ui::initialize()
    {

    }

    void ui::finish()
    {

    }

    void ui::update(float dt, int32_t width, int32_t height)
    {
        imgui_ctx.update(dt);

        rect bounds = {{(double)width / 4.0, (double)height / 4.0}, {(double)width / 2.0, (double)height / 2.0}};
        imgui_ctx.push_render_bounds(
        {
            [](void* bounds, const rect&) -> rect 
            {
                return *reinterpret_cast<rect*>(bounds);
            },
            &bounds,
            bounds
        });

        mxVbox(&gd)
        {
            for(auto& currentLabel : ld)
            {
                mxHbox(self._widths = {100.0f, 200.0f, 300.0f}; self.bounds.size.y = 100.0f;)
                {
                    for(int j = 0; j < 3; ++j) {
                        mxDebug(
                            self.on_mouse_over = [&](const vec2&) 
                            { 
                                self.bg_color = nvgRGB(255, 128, 0); 
                            };);
                    }
                };
                mxDebug(
                    self.bounds.size.y = 20.0f;
                    self.bg_color = nvgRGB(255, 32, 0);
                    self.on_mouse_over = [&](const vec2&)
                    {
                        self.bg_color = nvgRGB(64, 128, 64);
                    };
                    self.on_mouse_move = [](const vec2& pos)
                    {
                        // MX_LOG_INFO("MOVE CALLBACK {}", pos);
                    };);
                mxDebug(self.bounds.size.y = 35.0f;);
                mxLabel(&currentLabel, 
                    self.bounds.size.y = 7.0f;);
            }
        }

        imgui_ctx.pop_render_bounds();
    }
}
