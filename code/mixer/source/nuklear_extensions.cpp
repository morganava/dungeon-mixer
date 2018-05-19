#include "text_buffer.hpp"
#include "nuklear_extensions.hpp"

namespace tabletopify
{
    void
    nk_debug_fill(struct nk_context* ctx)
    {
        auto canvas = nk_window_get_canvas(ctx);

        struct nk_rect space;
        if(!nk_widget(&space, ctx)) return;

        nk_fill_rect(canvas, space, 0, nk_rgb(128, 255, 0));
        space.x += 1.0f;
        space.y += 1.0f;
        space.w -= 2.0f;
        space.h -= 2.0f;
        nk_fill_rect(canvas, space, 0, nk_rgb(255, 0, 255));
    }

    nk_flags
    nk_edit_string_zero_terminated(struct nk_context *ctx, nk_flags flags,
        text_buffer& textBuffer, nk_plugin_filter filter)
    {
        nk_flags result = nk_edit_string_zero_terminated(ctx, flags, textBuffer.buffer(), textBuffer.capacity(), filter);
        textBuffer.update();
        return result;
    }
}