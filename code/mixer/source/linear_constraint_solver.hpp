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
            else
            {
                bool resolved[N] = {false};
                
                double remainingRatio = 1.0;
                double remainingSize = totalSize;

                bool needResolve = true;
                while(needResolve)
                {
                    // initial guess
                    for(size_t i = 0; i < N; ++i)
                    {
                        if (!resolved[i])
                        {
                            _sizes[i] = _constraints[i].ratio / remainingRatio * remainingSize;    
                        }
                    }

                    // figure out which index is most out of constraints and has not yet been resolved
                    needResolve = false;
                    size_t maxIndex = 0;
                    double maxAbsDiff = 0.0;
                    for(size_t i = 0; i < N; ++i)
                    {
                        if (!resolved[i])
                        {
                            const auto currentSize = _sizes[i];
                            const auto& currentConstraint = _constraints[i];

                            double absDiff = 0.0;
                            if (currentSize < currentConstraint.min)
                            {
                                absDiff = (currentConstraint.min - currentSize);
                                needResolve = true;
                            }
                            else if(currentConstraint.max < currentSize)
                            {
                                absDiff = (currentSize - currentConstraint.max);
                                needResolve = true;
                            }
     
                            if (needResolve && absDiff > maxAbsDiff)
                            {
                                maxIndex = i;
                                maxAbsDiff = absDiff;
                            }    
                        }
                    }

                    // resolve the index that has largest abs difference between the size and min or max
                    if (needResolve)
                    {
                        _sizes[maxIndex] = clamp(_constraints[maxIndex].min, _sizes[maxIndex], _constraints[maxIndex].max);
                        resolved[maxIndex] = true;

                        remainingSize -= _sizes[maxIndex];
                        remainingRatio -= _constraints[maxIndex].ratio;
                    }
                }
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