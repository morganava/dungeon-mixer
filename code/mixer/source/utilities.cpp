#include "uuid.hpp"
#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"

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

    memory_buffer::memory_buffer(const fs::path& path)
    {
        MX_THROW_IF_FALSE(fs::exists(path));
        const size_t fileSize = fs::file_size(path);
        this->_bytes = fileSize;
        this->_memory = std::make_unique<uint8_t[]>(fileSize);

        unique_file file(std::fopen(path.c_str(), "rb"));
        MX_THROW_IF_FALSE(fileSize == std::fread(this->_memory.get(), sizeof(uint8_t), fileSize, file.get()));
    }
}