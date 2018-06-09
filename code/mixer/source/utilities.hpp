#pragma once

#include "logger.hpp"

#define MX_DEBUGBREAK(...) __builtin_trap()

#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)
#define __STR_LINE__ STRINGIFY(__LINE__)

#define MX_ASSERT(X) do {bool result = (X); if(!result) { tabletopify::get_logger()->blocking_log(LogLevel_Error, "Assertion Failure: " __FILE__ ":" __STR_LINE__ " " #X); MX_DEBUGBREAK();} } while(false);
#define MX_STATIC_ASSERT(X) static_assert(X, STRINGIFY(X))
#define MX_THROW_IF_FALSE(X) do {bool result = (X); if(!result) {throw std::runtime_error("Runtime Failure: " __FILE__ ":" __STR_LINE__ " " #X);} } while(0);
#define MX_THROW_IF_NULL(X) MX_THROW_IF_FALSE((X) != nullptr);
#define MX_THROW_MSG(X) throw std::runtime_error(fmt::format("Runtime Failure: " __FILE__ ":" __STR_LINE__ " {}", X));

// print operator for some nuklear types
std::ostream& operator<<(std::ostream& out, const struct nk_rect& rect);
std::ostream& operator<<(std::ostream& out, const struct nk_vec2& vec);
std::ostream& operator<<(std::ostream& out, const sf::Vector3f& vec);

namespace tabletopify
{
    // remove whitespace from head and tail of string
    std::string trim(const std::string& str);

    fs::path open_dialog(const char* filterList);
    fs::path save_dialog(const char* filterList);
}
