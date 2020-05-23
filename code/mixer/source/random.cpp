#include "random.hpp"
#include "utilities.hpp"

namespace tabletopify
{
    random::random(uint32_t seed)
    {
        _rand.seed(seed);
    }

    float random::next_float(float min, float max)
    {
        MX_ASSERT(min <= max);

        std::lock_guard<std::mutex> lock(_mutex);
        std::uniform_real_distribution<float> uniform(min, max);
        return uniform(_rand);
    }

    size_t random::next_index(size_t count)
    {
        MX_ASSERT(count > 0);

        std::lock_guard<std::mutex> lock(_mutex);
        std::uniform_int_distribution<size_t> uniform(size_t(0), count - 1);
        return uniform(_rand);
    }

    uuid random::next_uuid()
    {
        uuid result;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            std::uniform_int_distribution<uint64_t> uniform(uint64_t(0), ~uint64_t(0));
            result.qwords[0] = uniform(_rand);
            result.qwords[1] = uniform(_rand);
        }

        // set version to 4
        result.bytes[6] = (result.bytes[6] & 0x0F) | 0x40;
        // set variant to 1
        result.bytes[8] = (result.bytes[8] & 0x3F) | 0x80;

        return result;
    }
}