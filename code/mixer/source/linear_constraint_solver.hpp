 #pragma once

#include "array.hpp"

namespace tabletopify
{
    constexpr double unconstrained = std::numeric_limits<double>::max();

    struct constraint
    {
        double min = 0.0;
        double max = std::numeric_limits<double>::max();
        double ratio = 1.0;
    };

    class linear_constraint_solver
    {
    public:
        linear_constraint_solver();
        linear_constraint_solver(linear_constraint_solver&&) = default;
        linear_constraint_solver& operator=(linear_constraint_solver&&) = delete;

        linear_constraint_solver(const std::initializer_list<constraint>& constraints);
        linear_constraint_solver& operator=(const std::initializer_list<constraint>& constraints);
        void update_size(double totalSize);
        const double sizeAt(size_t idx) const
        {
            if (idx < N)
            {
                return _sizes[idx];
            }
            return 0.0;
        }

    private:
        const size_t N;
        const double _min_size;
        const double _max_size;
        double _total_size;
        std::unique_ptr<double[]> _sizes;
        std::unique_ptr<constraint[]> _constraints;
    };
}