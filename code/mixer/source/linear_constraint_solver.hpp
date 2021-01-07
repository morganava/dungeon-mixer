 #pragma once

#include "array.hpp"
#include "serialization_buffer.hpp"

namespace dm
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
        linear_constraint_solver(const linear_constraint_solver&) = default;
        linear_constraint_solver(linear_constraint_solver&&);
        linear_constraint_solver(array<constraint>&& constraints);
        linear_constraint_solver(std::initializer_list<constraint> constraints);
        linear_constraint_solver& operator=(linear_constraint_solver&&);

        double size_at(double totalSize, size_t idx) const;
    private:
        void update_size(double totalSize) const;
        const double _min_size;
        const double _max_size;
        const array<constraint> _constraints;

        // todo: ring buffer here so we don't always have to recalculate
        mutable double _total_size;
        mutable array<double> _sizes;

    friend void serialization_buffer_write(serialization_buffer&, const linear_constraint_solver&);
    friend bool serialization_buffer_read(serialization_buffer&, linear_constraint_solver&);
    };

    void serialization_buffer_write(serialization_buffer&, const linear_constraint_solver&);
    bool serialization_buffer_read(serialization_buffer&, linear_constraint_solver&);

    void from_json(const nl::json&, constraint&);
    void from_json(const nl::json&, linear_constraint_solver&);
}