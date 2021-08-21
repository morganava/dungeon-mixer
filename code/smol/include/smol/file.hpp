#pragma once

namespace smol
{
    typedef std::unique_ptr<::FILE> unique_file;

    // forward declaration
    template<typename T> class array;
    array<uint8_t> load_file(const fs::path&);
}

namespace std
{
    template<>
    class default_delete<std::FILE>
    {
    public:
        void operator()(std::FILE* file)
        {
            std::fclose(file);
        }
    };
}
