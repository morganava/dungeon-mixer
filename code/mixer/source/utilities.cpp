#include "uuid.hpp"
#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"


// stream operator for various nk and sfml types

std::ostream& operator<<(std::ostream& out, const struct nk_rect& rect)
{
    return out << "{ x: " << rect.x << ", y: " << rect.y << ", w: " << rect.w << ", h: " << rect.h << " }";
}

std::ostream& operator<<(std::ostream& out, const struct nk_vec2& vec)
{
    return out << "{ x: " << vec.x << ", y: " << vec.y << " }";
}

std::ostream& operator<<(std::ostream& out, const sf::Vector3f& vec)
{
    return out << "{ x: " << vec.x << ", y: " << vec.y << ", z: " << vec.z << " }";
}

namespace tabletopify
{
    // file dialog functions

    fs::path open_dialog(const char* filterList)
    {
        nfdchar_t* path = nullptr;
        switch(NFD_OpenDialog(filterList, nullptr, &path))
        {
            case NFD_ERROR:
                MX_LOG_ERROR("{}", std::string(NFD_GetError()));
                return {};
            case NFD_OKAY:
            {
                auto result = fs::path(path);
                free(path);
                return result;
            }
            case NFD_CANCEL:
                return {};
        }
        return {};
    }

    fs::path save_dialog(const char* filterList)
    {
        nfdchar_t* path = nullptr;
        switch(NFD_SaveDialog(filterList, nullptr, &path))
        {
            case NFD_ERROR:
                MX_LOG_ERROR("{}", std::string(NFD_GetError()));
                return {};
            case NFD_OKAY:
            {
                auto result = fs::path(path);
                free(path);
                return result;
            }
            case NFD_CANCEL:
                return {};
        }
        return {};
    }
}