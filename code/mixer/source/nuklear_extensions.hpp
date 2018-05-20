#pragma once

#include "text_buffer.hpp"

namespace tabletopify
{
    void
    nk_debug_fill(struct nk_context* ctx);

    nk_flags
    nk_edit_string_zero_terminated(struct nk_context *ctx, nk_flags flags,
        text_buffer& textBuffer, nk_plugin_filter filter);

    struct nk_image
    nk_load_image(const fs::path& src);
    
}