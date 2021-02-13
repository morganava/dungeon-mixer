#pragma once

#define MX_STRINGIFY(X) MX_STRINGIFY_IMPL(X)
#define MX_STRINGIFY_IMPL(X) #X

#define __STR_LINE__ MX_STRINGIFY(__LINE__)
