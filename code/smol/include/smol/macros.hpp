#pragma once

#define SMOL_STRINGIFY(X) SMOL_STRINGIFY_IMPL(X)
#define SMOL_STRINGIFY_IMPL(X) #X

#define __STR_LINE__ SMOL_STRINGIFY(__LINE__)

#define SMOL_GLUE(A,B) A##B

namespace smol
{
    // list must be a string in the form "foo, bar, baz" as with #__VA_ARGS__
    constexpr inline std::string_view string_list_get_entry(std::string_view list, size_t index)
    {
        auto head = std::begin(list);
        if (index > 0)
        {
            for(size_t k = 0 ; k < index; k++)
            {
                head = std::find(head + 1, std::end(list), ',');
            }
            // skip ", "
            head += 2;
        }
        auto tail = std::find(head, std::end(list), ',');
        return std::string_view(head, tail - head);
    }
}