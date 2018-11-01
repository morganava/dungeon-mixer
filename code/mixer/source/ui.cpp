#include "ui.hpp"
#include "utilities.hpp"

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
        MX_LOG_INFO("location: {}, dt: {}, width: {}, height: {}", __PRETTY_FUNCTION__, dt, width, height);
        // NVGcontext
    }



}