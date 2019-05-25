#pragma once

#include "singleton.hpp"

namespace tabletopify
{
    class ui
    {
    public:
        void update(float dt, int32_t width, int32_t height);
    private:
        imgui::hbox_data hd;
        imgui::scroll_area_data sd;
        imgui::label_data ld[20];
        imgui::button_data bd[20];
    };

    constexpr ui* get_ui()
    {
        return singleton<ui>::addr();
    }

    inline void init_ui()
    {
        singleton<ui>::construct();
    }

    constexpr void free_ui()
    {
        singleton<ui>::free();
    }
}