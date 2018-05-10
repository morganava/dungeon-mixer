#pragma once

namespace tabletopify
{
    size_t calc_hash(const void* buffer, size_t len, size_t seed);
    size_t calc_hash(const void* buffer, size_t len);
}

