#include "linear_constraint_solver.hpp"
#include "geometry.hpp"

namespace tabletopify
{
    linear_constraint_solver::linear_constraint_solver()
    : N(0)
    , _min_size(0.0)
    , _max_size(0.0)
    , _total_size(0.0)
    { }

    linear_constraint_solver::linear_constraint_solver(const std::initializer_list<constraint>& constraints)
    : N(constraints.size())
    , _min_size([&]() {double retval = 0.0; for(const auto& c : constraints) retval += c.min; return retval;}())
    , _max_size([&]() {double retval = 0.0; for(const auto& c : constraints) retval += c.max; return retval;}())
    , _total_size(0.0)
    , _sizes(std::make_unique<double[]>(N))
    , _constraints(std::make_unique<constraint[]>(N))
    {
        // copy constraints over
        std::copy(constraints.begin(), constraints.end(), _constraints.get());

        // normalize the ratios
        double ratio_sum = 0.0;
        for(size_t i = 0; i < N; ++i)
        {
            const auto& c = _constraints[i];
            ratio_sum += c.ratio;
        }

        for(size_t i = 0; i < N; ++i)
        {
            auto& c = _constraints[i];
            c.ratio /= ratio_sum;
        }
    }

    linear_constraint_solver& linear_constraint_solver::operator=(const std::initializer_list<constraint>& constraints)
    {
        this->~linear_constraint_solver();
        auto _this = new(this) linear_constraint_solver(constraints);
        return *_this;
    }

    void linear_constraint_solver::update_size(double totalSize)
    {
        MX_ASSERT(totalSize > 0.0);
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

            // resolve constraints with identical min and max values
            for(size_t i = 0; i < N; ++i) {
                const auto& currentConstraint = _constraints[i];
                if (currentConstraint.min == currentConstraint.max) {
                    const auto size = currentConstraint.min;
                    _sizes[i] = size;
                    resolved[i] = true;

                    remainingSize -= size;
                    remainingRatio -= currentConstraint.ratio;
                }
            }

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
}