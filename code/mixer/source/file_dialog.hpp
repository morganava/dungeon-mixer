#pragma once

namespace dm
{
    class file_dialog
    {
    public:
        static std::optional<fs::path> open(std::initializer_list<utf8_string> filters, fs::path beginPath);
        static std::optional<fs::path> save(std::initializer_list<utf8_string> filters, fs::path beginPath);
    };
}

