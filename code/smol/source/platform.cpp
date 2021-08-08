#include <smol/platform.hpp>
#include <smol/error.hpp>

namespace smol
{
    void set_current_thread_name(const std::string& name)
    {
        if constexpr (smol::current_platform == platform::linux)
        {
            // see pthread_set_name_np documentation
            constexpr size_t maxNameLength = 15;
            const size_t nameLength = name.size();

            // get the tail 15 characters
            const char* nameHead = name.c_str();
            if (nameLength > maxNameLength) {
                nameHead += nameLength - maxNameLength;
            }

            auto retval = ::pthread_setname_np(::pthread_self(), nameHead);
            SMOL_THROW_IF_FALSE(retval == 0);
        }
    }
}