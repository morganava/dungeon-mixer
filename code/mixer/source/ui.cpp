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
        mxRoot(rect(vec2::zero, vec2(width, height)))
        {
            mxHBox(&root.flex)
            {
                draw_sidebar(dt);
                switch(this->currentScreen)
                {
                    case Screen::Debug:
                        draw_debug(dt); break;
                    case Screen::Playlist:
                        draw_playlist(dt); break;
                    case Screen::Edit:
                        draw_edit(dt); break;
                    case Screen::About:
                        draw_about(dt); break;
                }
            }
        }
    }

    void ui::draw_sidebar(float dt)
    {
        mxVScroll(&root.sidebar.scroll)
        {
            mxSpacer(self.tags = {"sidebar"}; self.requestHeight(200.0));
        }

    }

    void ui::draw_playlist(float dt)
    {
        mxSpacer(self.tags = {"content"});
    }
}
