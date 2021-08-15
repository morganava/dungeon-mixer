#pragma once

// OS
#define _GNU_SOURCE
#include <pthread.h>

// C++

#include <memory>
#include <string>
#include <experimental/source_location>
#include <atomic>
#include <filesystem>
namespace fs = std::filesystem;
#include <utility>
#include <iostream>

// utf8proc : https://github.com/JuliaStrings/utf8proc
#define UTF8PROC_STATIC
#include <utf8proc.h>

// xxhash : https://github.com/Cyan4973/xxHash
#include <xxhash.h>

// fmt : https://github.com/fmtlib/fmt
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/printf.h>

// json : https://github.com/nlohmann/json
#include <json.hpp>
namespace nl = nlohmann;
