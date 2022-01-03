#pragma once

#include <ostream>

#include <smol/error.hpp>
#include <smol/macros.hpp>

namespace smol
{
    template<typename E>
    struct enum_iterator
    {
    public:
        constexpr enum_iterator(E val)
        : val_(val)
        { }

        constexpr E operator*() const
        {
            return val_;
        }

        constexpr enum_iterator& operator++()
        {
            if (val_ == E::count) return *this;

            val_ = static_cast<E>(static_cast<std::underlying_type<E>::type>(val_) + 1);
            return *this;
        }

        constexpr enum_iterator operator++(int)
        {
            if (val_ == E::count) return *this;

            auto retval = val_;
            val_ = static_cast<E>(static_cast<std::underlying_type<E>::type>(val_) + 1);
            return enum_iterator(retval);
        }

        constexpr bool operator==(const enum_iterator& that) const
        {
            return val_ == that.val_;
        }

        constexpr bool operator!=(const enum_iterator& that) const
        {
            return val_ != that.val_;
        }
    private:
        E val_;
    };

    template<typename E>
    struct enum_values
    {
        enum_values() = default;
        enum_values(E begin, E end)
        : begin_(begin)
        , end_(end)
        { }

        constexpr enum_iterator<E> begin() const
        {
            return enum_iterator(begin_);
        }

        constexpr enum_iterator<E> end() const
        {
            return enum_iterator(end_);
        }

    private:
        const E begin_ = E::__first_named;
        const E end_ = E::count;
    };

    template<typename E>
    enum_values<E> enum_range(E begin, E end)
    {
        return enum_values<E>(begin, end);
    }

    template<typename E>
    enum_values<E> enum_range(E begin, typename std::underlying_type<E>::type count)
    {
        typedef typename std::underlying_type<E>::type T;
        return enum_range(begin, static_cast<E>(static_cast<T>(begin) + count));
    }


    template<typename E>
    void to_json(nl::json& js, const E& e, const std::array<std::string_view, static_cast<size_t>(E::count)>& strings)
    {
        js = strings[static_cast<size_t>(e)];
    }

    template<typename E>
    void from_json(const nl::json& js, E& e, const std::array<std::string_view, static_cast<size_t>(E::count)>& strings)
    {
        static const std::map<std::string_view, E> stringMap = [&]() {
            std::map<std::string_view, E> retval;
            for(auto e : enum_values<E>())
            {
                auto index = static_cast<std::underlying_type_t<E>>(e);
                retval[strings[index]] = e;
            }
            return retval;
        }();

        auto name = js.get<std::string>();
        auto it = stringMap.find(name);
        SMOL_THROW_IF_FALSE(it != stringMap.end());

        e = it->second;
    }
}

template<typename E>
void to_json(nl::json& js, const E& e, const std::array<std::string_view, E::count>& strings)
{
    js = strings[static_cast<size_t>(e)];
}

#define SMOL_FLAG(N) (1 << (N))

// standard auto incrementing enum, automatic invalid and count values
#define SMOL_MAKE_ENUM(NAME, TYPE, FIRST, ...)\
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
        SMOL_THROW_IF_FALSE(val == NAME::invalid || (val >= NAME::__first_named && val <= NAME::count));\
        if (val == NAME::invalid) return std::string_view("invalid");\
        else if (val == NAME::count) return std::string_view("count");\
        else return ::smol::string_list_get_entry(#FIRST ", " #__VA_ARGS__, static_cast<size_t>(val));\
    }\
    inline constexpr std::array<std::string_view, static_cast<size_t>(NAME::count)> get_##NAME##_strings()\
    {\
        std::array<std::string_view, static_cast<size_t>(NAME::count)> retval;\
        for(auto e : smol::enum_values<NAME##_t>())\
        {\
            retval[static_cast<size_t>(e)] = to_string_view(e);\
        }\
        return retval;\
    }\
    inline std::ostream& operator<<(std::ostream& out, NAME##_t val) {\
        out << #NAME "::" << to_string_view(val);\
        return out;\
    }

#define SMOL_MAKE_ENUM_FROM_JSON(NAMESPACE, NAME)\
namespace nlohmann {\
    inline void from_json(const nl::json& js, NAMESPACE::NAME##_t& e)\
    {\
        ::smol::from_json<NAMESPACE::NAME##_t>(js, e, NAMESPACE::get_##NAME##_strings());\
    }\
}\

#define SMOL_MAKE_ENUM_TO_JSON(NAMESPACE, NAME)\
namespace nlohmann {\
    inline void to_json(nl::json& js, const NAMESPACE::NAME##_t& e)\
    {\
        ::smol::to_json<NAMESPACE::NAME##_t>(js, e, NAMESPACE::get_##NAME##_strings());\
    }\
}\

// explicitly named enum, automatic invalid value

#define SMOL_MAKE_EXPLICIT_ENUM(NAME, TYPE, ...)\
    enum class NAME : TYPE;\
    typedef NAME NAME##_t;\
    enum class NAME : TYPE {\
        invalid = static_cast<TYPE>(~TYPE(0)),\
        __VA_ARGS__,\
    };

// explicitly deined flag enum with bitwise operators defined
#define SMOL_MAKE_FLAG_ENUM(NAME, TYPE, ...)\
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

#define SMOL_ENUM_VALUES() __first_generated=0
#define SMOL_ENUM_VALUES_N(FIRST, ...) FIRST=0, __VA_ARGS__ __VA_OPT__(,) __first_generated

#define SMOL_MAKE_HANDLE_ENUM(NAME, ...)\
    enum class NAME : size_t;\
    typedef NAME NAME##_t;\
    enum class NAME : size_t {\
        invalid = static_cast<size_t>(~size_t(0)),\
        SMOL_GLUE(SMOL_ENUM_VALUES, __VA_OPT__(_N))(__VA_ARGS__)\
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
        if (val == NAME::invalid) out << SMOL_STRINGIFY(NAME::invalid);\
        else out << static_cast<size_t>(val);\
        return out;\
    }
