#pragma once

// OS
#define _GNU_SOURCE
#include <pthread.h>

// C++

#include <memory>
#include <string>
#include <experimental/source_location>

// fmt : https://github.com/fmtlib/fmt
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/printf.h>

// json : https://github.com/nlohmann/json
#include <json.hpp>
namespace nl = nlohmann;