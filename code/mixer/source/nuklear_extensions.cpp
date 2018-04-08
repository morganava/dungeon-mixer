#include "precomp.hpp"

#include "text_buffer.hpp"
#include "nuklear_extensions.hpp"

namespace tabletopify
{
    nk_flags
    nk_edit_string_zero_terminated(struct nk_context *ctx, nk_flags flags,
        text_buffer& textBuffer, nk_plugin_filter filter)
    {
        nk_flags result = nk_edit_string_zero_terminated(ctx, flags, textBuffer.buffer(), textBuffer.capacity(), filter);
        textBuffer.update();
        return result;
    }
}