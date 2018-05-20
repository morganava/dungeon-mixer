#include "text_buffer.hpp"
#include "nuklear_extensions.hpp"
#include "utilities.hpp"
#include "logger.hpp"

namespace tabletopify
{
    void
    nk_debug_fill(struct nk_context* ctx)
    {
        nk_debug_fill(ctx, 1);
    }

    void
    nk_debug_fill(struct nk_context* ctx, int count)
    {
        auto canvas = nk_window_get_canvas(ctx);

        while(count--)
        {
            struct nk_rect space;
            if(!nk_widget(&space, ctx)) return;

            nk_fill_rect(canvas, space, 0, nk_rgb(128, 255, 0));
            space.x += 1.0f;
            space.y += 1.0f;
            space.w -= 2.0f;
            space.h -= 2.0f;
            nk_fill_rect(canvas, space, 0, nk_rgb(255, 0, 255));
        }
    }

    void
    nk_empty_fill(struct nk_context* ctx)
    {
        nk_empty_fill(ctx, 1);
    }

    void
    nk_empty_fill(struct nk_context* ctx, int count)
    {
        struct nk_rect space;
        while(count--) nk_widget(&space, ctx);
    }

    nk_flags
    nk_edit_string_zero_terminated(struct nk_context *ctx, nk_flags flags,
        text_buffer& textBuffer, nk_plugin_filter filter)
    {
        nk_flags result = nk_edit_string_zero_terminated(ctx, flags, textBuffer.buffer(), textBuffer.capacity(), filter);
        textBuffer.update();
        return result;
    }

    struct nk_image
    nk_load_image(const fs::path& imageFile)
    {
        MX_THROW_IF_FALSE(fs::exists(imageFile));

        int width, height, channels;
        std::unique_ptr<uint8_t, decltype(&stbi_image_free)> raw(
            (uint8_t*)stbi_load(imageFile.c_str(), &width, &height, &channels, STBI_rgb_alpha), 
            stbi_image_free);

        if(raw == nullptr)
        {
            MX_LOG_ERROR("Failed to load image \"%s\", \"%s\"", imageFile.c_str(), stbi_failure_reason());
            return nk_image_id(0);
        }

        auto result = nk_load_image_from_memory(width, height, raw.get());        
        MX_LOG_INFO("Loaded image: %s", imageFile.c_str());
        return result;
    }
}