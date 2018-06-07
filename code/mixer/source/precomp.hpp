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
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#include <iterator>

// utf8proc : 
#define UTF8PROC_STATIC
#include <utf8proc.h>

// nuklear : https://github.com/vurtun/nuklear
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_STANDARD_IO
#include <nuklear.h>

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