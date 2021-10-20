#pragma once

#include "array.hpp"

namespace smol
{
    template<typename T> class array;
    nl::json parse_json(const array<uint8_t>&);
}
