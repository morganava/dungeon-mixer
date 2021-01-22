#pragma once
#include "utf8_string.hpp"

namespace dm
{
    class file_dialog
    {
    public:
        [[nodiscard]] static std::optional<fs::path> open(
        	std::initializer_list<utf8_string> filters,
        	fs::path beginPath);
        [[nodiscard]] static std::optional<fs::path> save(
        	std::initializer_list<utf8_string> filters,
        	fs::path beginPath,
        	const utf8_string& filename);
    };
}

