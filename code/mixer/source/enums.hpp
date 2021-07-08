#pragma once

#include "errors.hpp"
#include "macros.hpp"

namespace dm
{
    template<typename E>
    struct enum_iterator
    {
    public:
        enum_iterator(E val)
        : val(val)
        { }

        E operator*() const
        {
            return val;
        }

        enum_iterator& operator++()
        {
            if (val == E::__last_named) return *this;

            val = static_cast<E>(static_cast<std::underlying_type<E>::type>(val) + 1);
            return *this;
        }

        enum_iterator& operator++(int)
        {
            if (val == E::__last_named) return *this;

            auto retval = val;
            val = static_cast<E>(static_cast<std::underlying_type<E>::type>(val) + 1);
            return *this;
        }

        bool operator==(const enum_iterator& that) const
        {
            return val == that.val;
        }

        bool operator!=(const enum_iterator& that) const
        {
            return val != that.val;
        }
    private:
        E val;
    };

    template<typename E>
    struct enum_values
    {
        enum_iterator<E> begin() const
        {
            return enum_iterator(E::__first_named);
        }

        enum_iterator<E> end() const
        {
            return enum_iterator(E::__last_named);
        }
    };
}

#define MX_FLAG(N) (1 << (N))

// standard auto incrementing enum, automatic invalid and count values

#define MX_MAKE_ENUM(NAME, TYPE, FIRST, ...)\
enum class NAME : TYPE;\
typedef NAME NAME##_t;\
constexpr bool operator<(NAME##_t left, NAME##_t right) { return static_cast<TYPE>(left) < static_cast<TYPE>(right); }\
constexpr bool operator<=(NAME##_t left, NAME##_t right) { return static_cast<TYPE>(left) <= static_cast<TYPE>(right); }\
constexpr bool operator>(NAME##_t left, NAME##_t right) { return static_cast<TYPE>(left) > static_cast<TYPE>(right); }\
constexpr bool operator>=(NAME##_t left, NAME##_t right) { return static_cast<TYPE>(left) >= static_cast<TYPE>(right); }\
constexpr bool operator==(TYPE left, NAME##_t right) { return left == static_cast<TYPE>(right); }\
constexpr bool operator!=(TYPE left, NAME##_t right) { return left != static_cast<TYPE>(right); }\
constexpr bool operator<(TYPE left, NAME##_t right) { return left < static_cast<TYPE>(right); }\
constexpr bool operator<=(TYPE left, NAME##_t right) { return left <= static_cast<TYPE>(right); }\
constexpr bool operator>(TYPE left, NAME##_t right) { return left > static_cast<TYPE>(right); }\
constexpr bool operator>=(TYPE left, NAME##_t right) { return left >= static_cast<TYPE>(right); }\
constexpr bool operator==(NAME##_t left, TYPE right) { return static_cast<TYPE>(left) == right; }\
constexpr bool operator!=(NAME##_t left, TYPE right) { return static_cast<TYPE>(left) != right; }\
constexpr bool operator<(NAME##_t left, TYPE right) { return static_cast<TYPE>(left) < right; }\
constexpr bool operator<=(NAME##_t left, TYPE right) { return static_cast<TYPE>(left) <= right; }\
constexpr bool operator>(NAME##_t left, TYPE right) { return static_cast<TYPE>(left) > right; }\
constexpr bool operator>=(NAME##_t left, TYPE right) { return static_cast<TYPE>(left) >= right; }\
enum class NAME : TYPE {\
    invalid = static_cast<TYPE>(~TYPE(0)),\
    __first_named = 0,\
    FIRST = 0,\
    __VA_ARGS__ __VA_OPT__(,)\
    count,\
    __last_named = NAME::count - 1\
};\
inline constexpr std::string_view to_string_view(NAME##_t val) {\
    MX_THROW_IF_FALSE(val == NAME::invalid || (val >= NAME::__first_named && val <= NAME::count));\
    if (val == NAME::invalid) return std::string_view("invalid");\
    else if (val == NAME::count) return std::string_view("count");\
    else return dm::string_list_get_entry(#FIRST ", " #__VA_ARGS__, static_cast<size_t>(val));\
}\
inline std::ostream& operator<<(std::ostream& out, NAME##_t val) {\
    out << #NAME "::" << to_string_view(val);\
    return out;\
}

// explicitly named enum, automatic invalid value

#define MX_MAKE_EXPLICIT_ENUM(NAME, TYPE, ...)\
enum class NAME : TYPE;\
typedef NAME NAME##_t;\
enum class NAME : TYPE {\
    invalid = static_cast<TYPE>(~TYPE(0)),\
    __VA_ARGS__,\
};

//

#define MX_MAKE_FLAG_ENUM(NAME, TYPE, ...)\
enum class NAME : TYPE;\
typedef NAME NAME##_t;\
constexpr NAME##_t operator~(NAME##_t val) { return static_cast<NAME##_t>(~static_cast<size_t>(val)); }\
constexpr NAME##_t operator|(NAME##_t left, NAME##_t right) { return static_cast<NAME##_t>(static_cast<TYPE>(left) | static_cast<TYPE>(right)); }\
constexpr NAME##_t operator&(NAME##_t left, NAME##_t right) { return static_cast<NAME##_t>(static_cast<TYPE>(left) & static_cast<TYPE>(right)); }\
constexpr NAME##_t operator^(NAME##_t left, NAME##_t right) { return static_cast<NAME##_t>(static_cast<TYPE>(left) ^ static_cast<TYPE>(right)); }\
constexpr NAME##_t& operator|=(NAME##_t& left, NAME##_t right) { left = left | right; return left; }\
constexpr NAME##_t& operator&=(NAME##_t& left, NAME##_t right) { left = left & right; return left; }\
constexpr NAME##_t& operator^=(NAME##_t& left, NAME##_t right) { left = left ^ right; return left; }\
enum class NAME : TYPE {\
    none = 0,\
    __VA_ARGS__\
};\
inline std::ostream& operator<<(std::ostream& out, NAME##_t val) {\
    out << fmt::format("{0:#b}", static_cast<TYPE>(val));\
    return out;\
};

#define MX_ENUM_VALUES() __first_generated=0
#define MX_ENUM_VALUES_N(FIRST, ...) FIRST=0, __VA_ARGS__ __VA_OPT__(,) __first_generated

#define MX_MAKE_HANDLE_ENUM(NAME, ...)\
enum class NAME : size_t;\
typedef NAME NAME##_t;\
enum class NAME : size_t {\
    invalid = static_cast<size_t>(~size_t(0)),\
    MX_GLUE(MX_ENUM_VALUES, __VA_OPT__(_N))(__VA_ARGS__)\
};\
constexpr bool operator<(NAME##_t left, NAME##_t right) { return static_cast<size_t>(left) < static_cast<size_t>(right); }\
constexpr bool operator<=(NAME##_t left, NAME##_t right) { return static_cast<size_t>(left) <= static_cast<size_t>(right); }\
constexpr bool operator>(NAME##_t left, NAME##_t right) { return static_cast<size_t>(left) > static_cast<size_t>(right); }\
constexpr bool operator>=(NAME##_t left, NAME##_t right) { return static_cast<size_t>(left) >= static_cast<size_t>(right); }\
constexpr bool operator<(size_t left, NAME##_t right) { return left < static_cast<size_t>(right); }\
constexpr bool operator<=(size_t left, NAME##_t right) { return left <= static_cast<size_t>(right); }\
constexpr bool operator>(size_t left, NAME##_t right) { return left > static_cast<size_t>(right); }\
constexpr bool operator>=(size_t left, NAME##_t right) { return left >= static_cast<size_t>(right); }\
constexpr bool operator<(NAME##_t left, size_t right) { return static_cast<size_t>(left) < right; }\
constexpr bool operator<=(NAME##_t left, size_t right) { return static_cast<size_t>(left) <= right; }\
constexpr bool operator>(NAME##_t left, size_t right) { return static_cast<size_t>(left) > right; }\
constexpr bool operator>=(NAME##_t left, size_t right) { return static_cast<size_t>(left) >= right; }\
inline NAME##_t next_##NAME() {\
    static std::atomic<size_t> next = static_cast<size_t>(NAME::__first_generated);\
    return static_cast<NAME##_t>(next.fetch_add(1, std::memory_order::relaxed));\
};\
inline std::ostream& operator<<(std::ostream& out, NAME##_t val) {\
    if (val == NAME::invalid) out << MX_STRINGIFY(NAME::invalid);\
    else out << static_cast<size_t>(val);\
    return out;\
}


