#include "utf8_string.hpp"
#include "utilities.hpp"
#include "geometry.hpp"
#include "input.hpp"
#include "singleton.hpp"
#include "hash.hpp"
#include "nanovg.hpp"
#include "imgui.hpp"
#include "ui.hpp"

namespace tabletopify
{
    static ui* g_ui = nullptr;

    ui* get_ui()
    {
        MX_ASSERT(g_ui != nullptr);
        return g_ui;
    }

    void init_ui()
    {
        MX_ASSERT(g_ui == nullptr);
        g_ui = new ui();
        g_ui->initialize();
    }
    void free_ui()
    {
        g_ui->finish();
        delete g_ui;
        g_ui = nullptr;
    }

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

        mxGroup(self.data = &gd;)
        {
            for(int i = 0; i < 10; ++i) {
                mxStaticRow(self._widths = {100.0f, 200.0f, 300.0f}; self.bounds.size.y = 100.0f;)
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
                mxDebug(self.bounds.size.y = 30.0f;);
            }
        }

        imgui_ctx.pop_render_bounds();
    }
}
