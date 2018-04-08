#include "precomp.hpp"

#include "uuid.hpp"
#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"

namespace tabletopify
{
    // stream operator for various nk and sfml types

    std::ostream& operator<<(std::ostream& out, struct nk_rect& rect)
    {
        return out << "{ x: " << rect.x << ", y: " << rect.y << ", w: " << rect.w << ", h: " << rect.h << " }";
    }

    std::ostream& operator<<(std::ostream& out, struct nk_vec2& vec)
    {
        return out << "{ x: " << vec.x << ", y: " << vec.y << " }";
    }

    std::ostream& operator<<(std::ostream& out, const sf::Vector3f& vec)
    {
        return out << "{ x: " << vec.x << ", y: " << vec.y << ", z: " << vec.z << " }";
    }

    // string operations

    std::string vstring_format(const char* format, va_list argsSrc)
    {
        va_list args1, args2;
        va_copy(args1, argsSrc);
        va_copy(args2, argsSrc);

        auto len = vsnprintf(nullptr, 0, format, args1);
        va_end(args1);

        std::string result(len, 0);
        vsnprintf(const_cast<char*>(result.data()), len+1, format, args2);
        va_end(args2);

        return result;
    };

    std::string string_format(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        std::string result = vstring_format(format, args);
        va_end(args);

        return result;
    }

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
                log(LogLevel_Error, "%s", NFD_GetError());
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
                log(LogLevel_Error, "%s", NFD_GetError());
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