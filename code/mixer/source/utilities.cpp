#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"

namespace dm
{
    memory_buffer::memory_buffer(const fs::path& path)
    {
        SMOL_THROW_IF_FALSE(fs::exists(path));
        const size_t fileSize = fs::file_size(path);
        this->_bytes = fileSize;
        this->_memory = std::make_unique<uint8_t[]>(fileSize);

        unique_file file(std::fopen(path.c_str(), "rb"));
        SMOL_THROW_IF_FALSE(fileSize == std::fread(this->_memory.get(), sizeof(uint8_t), fileSize, file.get()));
    }
}