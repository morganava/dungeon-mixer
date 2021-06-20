#pragma once
#include "singleton.hpp"
#include "uuid.hpp"

namespace dm
{
    class random
    {
    public:
        random(uint32_t seed);
        float next_float(float min, float max);
        size_t next_index(size_t count);
        uuid next_uuid();
    private:
        std::mutex _mutex;
        std::minstd_rand _rand;
    };
    MX_MAKE_SINGLETON(random, worker_flag::any);
}
