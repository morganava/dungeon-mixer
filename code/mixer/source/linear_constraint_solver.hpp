 #pragma once

#include "array.hpp"

namespace tabletopify
{
    struct constraint
    {
        double min = 0.0;
        double max = std::numeric_limits<double>::max();
        double ratio = 1.0;
    };

    template<size_t N>
    class linear_constraint_solver
    {
    public:
        constexpr linear_constraint_solver(const std::initializer_list<constraint>& constraints)
        : _min_size([&]() {double retval = 0.0; for(const auto& c : constraints) retval += c.min; return retval;}())
        , _max_size([&]() {double retval = 0.0; for(const auto& c : constraints) retval += c.max; return retval;}())
        {
            // copy constraints over
            std::copy(constraints.begin(), constraints.end(), _constraints);

            // normalize the ratios
            double ratio_sum = 0.0;
            for(const auto& c : _constraints)
            {
                ratio_sum += c.ratio;
            }

            for(auto& c : _constraints)
            {
                c.ratio /= ratio_sum;
            }
        }

        constexpr void update_size(double totalSize)
        {
            // nothing to do
            if(totalSize == _total_size)
            {
                return;
            }
            _total_size = totalSize;

            if(_total_size <= _min_size)
            {
                for(size_t i = 0; i < N; ++i)
                {
                    _sizes[i] = _constraints[i].min;
                }
            }
            else if(_total_size >= _max_size)
            {
                for(size_t i = 0; i < N; ++i)
                {
                    _sizes[i] = _constraints[i].max;
                }
            }

            bool resolved[N] = {false};
            double absdiff[N];
            
            double remainingRatio = 1.0;
            double remainingSize = totalSize;
            MX_LOG_INFO("---");
            MX_LOG_INFO("totalSize: {}", totalSize);

            for(size_t i = 0; i < N; ++i)
            {
                MX_LOG_INFO("ratio[{}] : {}", i, _constraints[i].ratio);
            }

            do
            {
                // initial guess
                for(size_t i = 0; i < N; ++i)
                {
                    if (!resolved[i])
                    {
                        _sizes[i] = _constraints[i].ratio / remainingRatio * remainingSize;    
                    }
                    MX_LOG_INFO("guess[{}] : {}", i, _sizes[i]);
                }

                for(size_t i = 0; i < N; ++i)
                {
                    MX_LOG_INFO("resolved[{}] : {}", i, resolved[i]);
                }

                size_t invalidCount = 0;
                auto calc_absdiff = [&](size_t i) -> double
                {
                    auto s = _sizes[i];
                    const auto& c = _constraints[i];

                    if (s < c.min) {
                        invalidCount++;
                        return c.min - s;
                    }
                    if (s > c.max) {
                        invalidCount++;
                        return s - c.max;
                    }
                    return 0.0;
                };

                size_t maxIndex = 0;
                for(size_t i = 0; i < N; ++i)
                {
                    if (!resolved[i])
                    {
                        const auto diff = calc_absdiff(i); 
                        absdiff[i] = diff;

                        if (invalidCount > 0 && diff > absdiff[maxIndex]) {
                            maxIndex = i;
                        }    
                    }
                }

                for(size_t i = 0; i < N; ++i)
                {
                    MX_LOG_INFO("diff[{}] : {}", i, absdiff[i]);
                }

                if (invalidCount > 0)
                {
                    _sizes[maxIndex] = clamp(_constraints[maxIndex].min, _sizes[maxIndex], _constraints[maxIndex].max);
                    resolved[maxIndex] = true;
                    remainingSize -= _sizes[maxIndex];
                    remainingRatio -= _constraints[maxIndex].ratio;

                    MX_LOG_INFO("maxIndex: {}", maxIndex);
                    MX_LOG_INFO("remainingSize : {}", remainingSize);
                }
                else
                {
                    break;
                }
            }
            while(true);

            MX_LOG_INFO("remainingSize : {}", remainingSize);
            for(size_t i = 0; i < N ; ++i)
            {
                MX_LOG_INFO("size[{}] : {}, resolved[{}] : {}", i, _sizes[i], i, resolved[i]);
            }
        }

        constexpr const array_view<double> sizes()
        {
            return array_view<double>(_sizes, _sizes + N);
        }

    private:
        const double _min_size;
        const double _max_size;
        double _total_size = 0.0;
        double _sizes[N] = {};
        constraint _constraints[N] = {};
    };
}