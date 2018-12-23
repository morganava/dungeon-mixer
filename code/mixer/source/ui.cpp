#include "ui.hpp"
#include "utilities.hpp"
#include "imgui.hpp"

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

    void ui::update(NVGcontext* ctx, float dt, int32_t width, int32_t height)
    {
        MX_LOG_INFO("--Begin Update--");

        imgui::context imgui_ctx(ctx);
        imgui::rect bounds = {{0, 0}, {(float)width, (float)height}};
        imgui_ctx.push_bounds(
        {
            [](void* bounds, const imgui::rect&) -> imgui::rect 
            {
                return *reinterpret_cast<imgui::rect*>(bounds);
            },
            &bounds
        });

        mxGroup(self._bounds.size = {(float)width, (float)height};)
        {
            for(int i = 0; i < 3; ++i) {            
                mxStaticRow(self._widths = {100.0f, 200.0f, 300.0f}; self._bounds.size.y = 100.0f;)
                {
                    mxDebug(self._bounds.size.y = 10.0f;);
                    mxDebug(self._bounds.size.y = 20.0f;);
                    mxDebug(self._bounds.size.y = 30.0f;);
                };
            }
        }

        imgui_ctx.pop_bounds();

        MX_LOG_INFO("--End Update--");
    }
}
