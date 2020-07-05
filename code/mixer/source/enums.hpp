#pragma once

#define MX_FLAG(N) (1 << (N))

// standard auto incrementing enum, automatic invalid and count values

#define MX_MAKE_ENUM(NAME, TYPE, FIRST, ...)\
enum class NAME : TYPE;\
typedef NAME NAME##_t;\
constexpr bool operator<(NAME##_t left, NAME##_t right) { return static_cast<TYPE>(left) < static_cast<TYPE>(right); }\
constexpr bool operator<=(NAME##_t left, NAME##_t right) { return static_cast<TYPE>(left) <= static_cast<TYPE>(right); }\
constexpr bool operator>(NAME##_t left, NAME##_t right) { return static_cast<TYPE>(left) > static_cast<TYPE>(right); }\
constexpr bool operator>=(NAME##_t left, NAME##_t right) { return static_cast<TYPE>(left) >= static_cast<TYPE>(right); }\
enum class NAME : TYPE {\
    invalid = static_cast<TYPE>(~TYPE(0)),\
    FIRST = 0,\
    __VA_ARGS__,\
    count\
};

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
#define MX_GLUE(A,B) A##B

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
inline NAME##_t next_##NAME() {\
    static size_t next = static_cast<size_t>(NAME::__first_generated);\
    return static_cast<NAME##_t>(next++);\
};\
inline std::ostream& operator<<(std::ostream& out, NAME##_t val) {\
    out << static_cast<size_t>(val);\
    return out;\
}


