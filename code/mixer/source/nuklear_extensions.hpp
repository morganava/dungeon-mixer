#pragma once

namespace tabletopify
{
    nk_flags
    nk_edit_string_zero_terminated(struct nk_context *ctx, nk_flags flags,
        text_buffer& textBuffer, nk_plugin_filter filter);
}