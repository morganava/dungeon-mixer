#include "uuid.hpp"
#include "utilities.hpp"
#include "work_queue.hpp"
#include "logger.hpp"

namespace dm
{
    memory_buffer::memory_buffer(const fs::path& path)
    {
        MX_THROW_IF_FALSE(fs::exists(path));
        const size_t fileSize = fs::file_size(path);
        this->_bytes = fileSize;
        this->_memory = std::make_unique<uint8_t[]>(fileSize);

        unique_file file(std::fopen(path.c_str(), "rb"));
        MX_THROW_IF_FALSE(fileSize == std::fread(this->_memory.get(), sizeof(uint8_t), fileSize, file.get()));
    }

    // list must be a string in the form "foo, bar, baz"
    std::string_view string_list_get_entry(std::string_view list, size_t index)
    {
        auto head = std::begin(list);
        if (index > 0)
        {
            for(size_t k = 0 ; k < index; k++)
            {
                head = std::find(head + 1, std::end(list), ',');
            }
            // skip ", "
            head += 2;
        }
        auto tail = std::find(head, std::end(list), ',');
        return std::string_view(head, tail - head);
    }
}