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
                // intial guess
                for(size_t i = 0; i < N; ++i)
                {
                    _sizes[i] = _constraints[i].ratio * totalSize;
                }

                auto constraints_met = [&]() -> bool
                {
                    for(size_t i = 0; i < N; ++i)
                    {
                        const auto& size = _sizes[i];
                        const auto& constraint = _constraints[i]; 
                        if(size < constraint.min || size > constraint.max)
                        {
                            return false;
                        }
                    }
                    return true;
                };

                do
                {
                    double remainingRatio =  1.0;
                    double remainingSize = totalSize;
                    bool resolved[N] = {false};
                    // resolve mins
                    for(size_t i = 0; i < N; ++i)
                    {
                        auto& size = _sizes[i];
                        const auto& constraint = _constraints[i];

                        if(size <= constraint.min)
                        {
                            size = constraint.min;
                            remainingSize -= constraint.min;
                            remainingRatio -= constraint.ratio;
                            resolved[i] = true;
                        }
                    }

                    // squish remaining
                    for(size_t i = 0; i < N; ++i)
                    {
                        auto& size = _sizes[i];
                        const auto& constraint = _constraints[i];
                        
                        if(!resolved[i])
                        {
                            size = remainingSize * constraint.ratio / remainingRatio;
                        }
                    }

                    // resolve maxs
                    for(size_t i = 0; i < N; ++i)
                    {
                        auto& size = _sizes[i];
                        const auto& constraint = _constraints[i];

                        if(size >= constraint.max)
                        {
                            size = constraint.max;
                            remainingSize -= constraint.max;
                            remainingRatio -= constraint.ratio;
                            resolved[i] = true;
                        }
                    }

                    // stretch remaining
                    for(size_t i = 0; i < N; ++i)
                    {
                        auto& size = _sizes[i];
                        const auto& constraint = _constraints[i];
                        
                        if(!resolved[i])
                        {
                            size = remainingSize * constraint.ratio / remainingRatio;
                        }
                    }                    
                }
                while(!constraints_met());
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