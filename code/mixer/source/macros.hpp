#pragma once

#define MX_STRINGIFY(X) MX_STRINGIFY_IMPL(X)
#define MX_STRINGIFY_IMPL(X) #X

#define MX_GLUE(A,B) A##B

#define __STR_LINE__ MX_STRINGIFY(__LINE__)

namespace dm
{
    // list must be a string in the form "foo, bar, baz" as with #__VA_ARGS__
    constexpr std::string_view string_list_get_entry(std::string_view list, size_t index)
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