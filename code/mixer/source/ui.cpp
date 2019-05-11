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
        imgui::render_bounds{
            [](imgui::widget_base* widget, const rect&) -> rect
            {
                return *reinterpret_cast<rect*>(widget);
            },
            reinterpret_cast<imgui::widget_base*>(&bounds),
            bounds
        });

        linear_constraint_solver<3> lcg({
            {50.0, 500.0, 1.0},
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
                    mxButton(
                        &currentButton,
                        self.tags = {"foo"};
                        self.on_click = [&]() -> void
                        {
                           MX_LOG_INFO("Clicked Button {}", idx);
                        };);
                    currentButton.text = "push";
                    mxButton(
                        &currentButton,
                        self.tags = {"push"};
                        self.on_click = []() -> void
                        {
                            MX_LOG_INFO("PUSHED");
                        };);

                    mxDebug(
                        self.on_mouse_over = [&](const vec2&)
                        {
                            self.bg_color = color::rgb(255, 128, 0);
                        };);
                }
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

                mxLabel(&currentLabel,
                    self.bounds.size.y = 100.0f;);
            }
        }

        imgui_ctx.pop_render_bounds();
    }
}
