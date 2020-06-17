 #pragma once

#include "array.hpp"
#include "serialization_buffer.hpp"

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
        linear_constraint_solver(linear_constraint_solver&&);
        linear_constraint_solver(const array<constraint>& constraints);
        linear_constraint_solver(std::initializer_list<constraint> constraints);
        linear_constraint_solver& operator=(linear_constraint_solver&&);
        void update_size(double totalSize);
        double size_at(size_t idx) const;

    private:
        const double _min_size;
        const double _max_size;
        const array<constraint> _constraints;

        // todo: ring buffer here so we don't always have to recalculate
        double _total_size;
        array<double> _sizes;

    friend void serialization_buffer_write(serialization_buffer&, const linear_constraint_solver&);
    friend bool serialization_buffer_read(serialization_buffer&, linear_constraint_solver&);
    };

    void serialization_buffer_write(serialization_buffer&, const linear_constraint_solver&);
    bool serialization_buffer_read(serialization_buffer&, linear_constraint_solver&);
}