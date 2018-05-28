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
    std::vector<std::string> split(const std::string& str, const std::string& delim)
    {
        std::vector<std::string> result;
        const char* head = str.data();
        const char* tail;
        const char* end = &*str.cend();
        do
        {
            tail = std::strstr(head, delim.c_str());
            if(tail == nullptr)
            {
                tail = end;
            }

            if(tail > head)
            {
                result.push_back(std::string(head, tail));
            }

            head = tail+delim.size();
        }
        while(tail != end);

        return result;
    }

    bool contains(const std::string& str, const std::string& pattern)
    {
        bool result = std::strstr(str.data(), pattern.data()) != nullptr;
        return result;
    }

    std::string to_lower(const std::string& str)
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    std::string trim(const std::string& str)
    {
        size_t first = str.find_first_not_of(" \t");
        if(first == std::string::npos) return "";

        size_t last = str.find_last_not_of(" \t");

        return std::string(str, first, last - first + 1);
    }

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