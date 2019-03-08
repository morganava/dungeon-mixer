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
#include "linear_constraint_solver.hpp"
#include "array.hpp"

namespace tabletopify
{
    void ui::update(float dt, int32_t width, int32_t height)
    {
        auto& imgui_ctx = imgui::g_context;
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

        linear_constraint_solver<3> lcg({
            {0.0, 100.0, 1.0},
            {0.0, 200.0, 2.0},
            {0.0, 300.0, 3.0},
        });

        mxVbox(&gd)
        {
            for(size_t idx = 0; idx < 1; ++idx)
            {
                auto& currentLabel = ld[idx];
                auto& currentButton = bd[idx];

                lcg.update_size(width / 2.0);
                mxHbox(self.widths = lcg.sizes(); self.bounds.size.y = 200.0f;)
                {
                    currentButton.text = "button";
                    mxButton(&currentButton, self.on_click = [&]() -> void
                    {
                       MX_LOG_INFO("Clicked Button {}", idx); 
                    };);
                    for(int j = 0; j < 2; ++j) {
                        mxDebug(
                            self.on_mouse_over = [&](const vec2&) 
                            { 
                                self.bg_color = color::rgb(255, 128, 0); 
                            };);
                    }
                };
                mxDebug(
                    self.bounds.size.y = 80.0f;
                    self.bg_color = color::rgb(255, 32, 0);
                    self.on_mouse_over = [&](const vec2&)
                    {
                        self.bg_color = color::rgb(64, 128, 64);
                    };
                    self.on_mouse_move = [](const vec2& pos)
                    {
                        // MX_LOG_INFO("MOVE CALLBACK {}", pos);
                    };);
                mxDebug(self.bounds.size.y = 120.0f;);
                currentLabel.text = "hello world!";
                currentLabel.text_color = color::rgb(0,0,0);
                currentLabel.text_size = 36.0f;

                mxLabel(&currentLabel, 
                    self.bounds.size.y = 100.0f;);
            }
        }

        imgui_ctx.pop_render_bounds();
    }
}
