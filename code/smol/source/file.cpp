#include <smol/file.hpp>
#include <smol/serialization_buffer.hpp>
#include <smol/array.hpp>

namespace smol
{
    array<uint8_t> load_file(const fs::path& path)
    {
        SMOL_THROW_IF_FALSE(fs::exists(path));
        const size_t fileSize = fs::file_size(path);

        array<uint8_t> retval(fileSize);

        unique_file file(std::fopen(path.c_str(), "rb"));
        SMOL_THROW_IF_FALSE(fileSize == std::fread(retval.data(), sizeof(uint8_t), fileSize, file.get()));

        return retval;
    }
}