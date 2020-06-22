#include "linear_constraint_solver.hpp"
#include "geometry.hpp"

namespace tabletopify
{
    static void normalize_ratios(array<constraint>& constraints)
    {
        double ratio_sum = 0.0;
        for(auto& c : constraints) ratio_sum += c.ratio;
        for(auto& c : constraints) c.ratio /= ratio_sum;
    }

    linear_constraint_solver::linear_constraint_solver()
    : _min_size(0.0)
    , _max_size(0.0)
    , _total_size(0.0)
    { }

    linear_constraint_solver::linear_constraint_solver(linear_constraint_solver&& that)
    : _min_size(that._min_size)
    , _max_size(that._max_size)
    , _constraints(std::move(that._constraints))
    , _total_size(that._total_size)
    , _sizes(std::move(that._sizes))
    {
        that.~linear_constraint_solver();
        new (&that) linear_constraint_solver();
    }

    linear_constraint_solver::linear_constraint_solver(array<constraint>&& constraints)
    : _min_size([&]() {double retval = 0.0; for(const auto& c : constraints) retval += c.min; return retval;}())
    , _max_size([&]() {double retval = 0.0; for(const auto& c : constraints) retval += c.max; return retval;}())
    , _constraints([&]() {
        array<constraint> retval(std::move(constraints));
        normalize_ratios(retval);
        return retval;
    }())
    , _total_size(0.0)
    , _sizes(_constraints.size())
    { }

    linear_constraint_solver::linear_constraint_solver(std::initializer_list<constraint> constraints)
    : _min_size([&]() {double retval = 0.0; for(const auto& c : constraints) retval += c.min; return retval;}())
    , _max_size([&]() {double retval = 0.0; for(const auto& c : constraints) retval += c.max; return retval;}())
    , _constraints([&]() {
        array<constraint> retval(constraints);
        normalize_ratios(retval);
        return retval;
    }())
    , _total_size(0.0)
    , _sizes(_constraints.size())
    { }

    linear_constraint_solver& linear_constraint_solver::operator=(linear_constraint_solver&& that)
    {
        this->~linear_constraint_solver();
        new (this) linear_constraint_solver(std::move(that));
        return *this;
    }

    void linear_constraint_solver::update_size(double totalSize)
    {
        totalSize = std::max(0.0, totalSize);
        // nothing to do
        if(totalSize == _total_size)
        {
            return;
        }
        _total_size = totalSize;
        const size_t N = _constraints.size();
        if (N == 0)
        {
            return;
        }
        else if(_total_size <= _min_size)
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

    double linear_constraint_solver::size_at(size_t idx) const
    {
        if (idx < _sizes.size())
        {
            return _sizes[idx];
        }
        return 0.0;
    }

    void serialization_buffer_write(serialization_buffer& self, const linear_constraint_solver& val)
    {
        self.write(val._constraints);
    }

    bool serialization_buffer_read(serialization_buffer& self, linear_constraint_solver& out)
    {
        array<constraint> constraints;
        if (self.read(constraints))
        {
            linear_constraint_solver retval(std::move(constraints));
            out = std::move(retval);
            return true;
        }
        return false;
    }

    void from_json(const nl::json& js, constraint& out)
    {
        const auto parse_min = [](const nl::json& js) -> double
        {
            if (js.is_string())
            {
                auto str = js.get<std::string>();
                MX_THROW_IF_FALSE(str == "none");
                return 0.0;
            }
            MX_THROW_IF_FALSE(js.is_number());
            auto retval = js.get<double>();
            MX_THROW_IF_FALSE(retval >= 0.0);
            return retval;
        };

        const auto parse_max = [](const nl::json& js) -> double
        {
            if (js.is_string())
            {
                auto str = js.get<std::string>();
                MX_THROW_IF_FALSE(str == "none");
                return std::numeric_limits<double>::max();
            }
            MX_THROW_IF_FALSE(js.is_number());
            auto retval = js.get<double>();
            MX_THROW_IF_FALSE(retval >= 0.0);
            return retval;
        };

        const auto parse_scale = [](const nl::json& js) -> double
        {
            MX_THROW_IF_FALSE(js.is_number());
            auto retval = js.get<double>();
            MX_THROW_IF_FALSE(retval > 0.0);
            return retval;
        };

        constraint retval;

        if (js.is_array())
        {
            // purposeful fallthrough
            switch(js.size())
            {
                default:
                    static_assert(std::is_unsigned<decltype(js.size())>::value);
                case 3:
                    retval.ratio = parse_scale(js[2]);
                case 2:
                    retval.max = parse_max(js[1]);
                case 1:
                    retval.min = parse_min(js[0]);
                case 0:
                    break;
            }
        }
        else if (js.is_object())
        {
            if (auto it = js.find("ratio"); it != js.end())
            {
                retval.ratio = parse_scale(*it);
            }
            if (auto it = js.find("max"); it != js.end())
            {
                retval.max = parse_max(*it);
            }
            if (auto it = js.find("min"); it != js.end())
            {
                retval.min = parse_min(*it);
            }
        }
        else
        {
            MX_THROW_MSG("invalid constraint json")
        }

        MX_LOG_INFO("constraint {{ min : {}, max : {}, ratio : {} }}", retval.min, retval.max, retval.ratio);

        out = retval;
    }

    void from_json(const nl::json& js, linear_constraint_solver& out)
    {
        MX_THROW_IF_FALSE(js.is_array());
        auto constraints = js.get<array<constraint>>();
        out = {std::move(constraints)};
    }
}