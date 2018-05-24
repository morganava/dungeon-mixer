#pragma once

#define MX_DEBUGBREAK(...) __builtin_trap()

#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)
#define __STR_LINE__ STRINGIFY(__LINE__)

#define MX_ASSERT(X) do {bool result = (X); if(!result) { tabletopify::assertion_log("Assertion Failure: " __FILE__ ":" __STR_LINE__ " " #X); MX_DEBUGBREAK();} } while(false);
#define MX_STATIC_ASSERT(X) static_assert(X, STRINGIFY(X))
#define MX_THROW_IF_FALSE(X) do {bool result = (X); if(!result) {throw std::runtime_error("Runtime Failure: " __FILE__ ":" __STR_LINE__ " " #X);} } while(0);

namespace tabletopify
{
    void assertion_log(const char* msg);

    // print operator for some nuklear types
    std::ostream& operator<<(std::ostream& out, const struct nk_rect& rect);
    std::ostream& operator<<(std::ostream& out, const struct nk_vec2& vec);
    std::ostream& operator<<(std::ostream& out, const sf::Vector3f& vec);

    std::string string_format(const char* format, ...);
    std::string vstring_format(const char* format, va_list argsSrc);

    // split string into list of strings delimited by the given token
    std::vector<std::string> split(const std::string& str, const std::string& token);

    // does a string contain the given string pattern
    bool contains(const std::string& str, const std::string& pattern);

    // convert string to lower-case (not a general utf-8 solution)
    std::string to_lower(const std::string& str);

    // remove whitespace from head and tail of string
    std::string trim(const std::string& str);

    fs::path open_dialog(const char* filterList);
    fs::path save_dialog(const char* filterList);
}
