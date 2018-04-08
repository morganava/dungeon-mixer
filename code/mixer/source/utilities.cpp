#include "precomp.hpp"

#include "uuid.hpp"
#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"

namespace tabletopify
{
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