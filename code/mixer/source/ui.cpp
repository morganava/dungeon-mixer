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
        sd.direction = imgui::scroll_direction::both;

        hd.constraints = {
            {150.0, 500.0, 3.0},
            {0.0, 1000000.0, 2.0},
            {150.0, 300.0, 3.0},
        };

        rect viewport = {{0.0, 0.0}, {(double)width, (double)height}};
        double offset = 0.0;
        mxRoot(viewport)
        {
            mxScrollArea(&sd)
            {
                for(size_t idx = 0; idx < 4; ++idx)
                {
                    auto& currentLabel = ld[idx];
                    auto& currentButton = bd[idx];

                    mxHbox(&hd, self.requestPosition({offset, offset}); self.requestSize({400.0, 400.0});)
                    {
                        currentButton.text = "button";
                        mxButton(
                            &currentButton,
                            self.tags = {"foo"};
                            self.on_click = [&]() -> void
                            {
                               MX_LOG_INFO("Clicked Button {}", idx);
                            };);
                        mxSpacer();
                        currentButton.text = "push";
                        mxButton(
                            &currentButton,
                            self.tags = {"push"};
                            self.on_click = []() -> void
                            {
                                MX_LOG_INFO("PUSHED");
                            };);
                    }
                    offset += 400.0;
                    currentLabel.text = "hello world!";
                    mxLabel(&currentLabel, self.requestPosition({offset, offset}); self.requestSize({100.0, 100.0}););
                    offset += 100;
                }
            }
        }
    }
}
