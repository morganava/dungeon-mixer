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
        linear_constraint_solver<3> lcg({
            {50.0, 500.0, 1.0},
            {0.0, 1000000.0, 2.0},
            {0.0, 300.0, 3.0},
        });

        rect viewport = {{0.0, 0.0}, {(double)width, (double)height}};
        mxRoot(viewport)
        {
            mxVbox(&gd)
            {
                for(size_t idx = 0; idx < 1; ++idx)
                {
                    auto& currentLabel = ld[idx];
                    auto& currentButton = bd[idx];

                    lcg.update_size(width / 2.0);
                    mxHbox(self.widths = lcg.sizes(); self.requestHeight(200.0);)
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
                    currentLabel.text = "hello world!";

                    mxLabel(&currentLabel,
                        self.requestHeight(100.0););
                }
            }
        }
    }
}
