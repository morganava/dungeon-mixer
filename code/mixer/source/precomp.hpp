#ifndef TABLETOPIFY_PRECOMP_H
#define TABLETOPIFY_PRECOMP_H

 // C++
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cinttypes>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
using ss = std::stringstream;
#include <string>
#include <string_view>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <mutex>
#include <thread>
#include <queue>
#include <chrono>
#include <atomic>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;
#include <iterator>
#include <algorithm>
#include <regex>
#include <type_traits>

// utf8proc : https://github.com/JuliaStrings/utf8proc
#define UTF8PROC_STATIC
#include <utf8proc.h>

// GLEW
#include <GL/glew.h>

// GLFW
#ifdef __APPLE__
#   define GLFW_INCLUDE_GLCOREARB
#endif
// #define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

// nanovg : https://github.com/memononen/nanovg
#define NANOVG_GL3
#include <nanovg.h>
#include <nanovg_gl.h>

// SFML
#include <SFML/Audio.hpp>

// json : https://github.com/nlohmann/json
#include <json.hpp>
namespace nl = nlohmann;

// nativefiledialog : https://github.com/mlabbe/nativefiledialog
#include <nfd.h>

// xxhash : https://github.com/Cyan4973/xxHash
#include <xxhash.h>

// stb_image : https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
#include <stb_image.h>

// fmt : https://github.com/fmtlib/fmt
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/printf.h>

// countof(X)
template<typename T, size_t N>
constexpr size_t countof(const T (&)[N]) { return N; }

#endif // TABLETOPIFY_PRECOMP_H
