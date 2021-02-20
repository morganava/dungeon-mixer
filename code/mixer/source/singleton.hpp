#pragma once

#include "errors.hpp"
#include "worker_enums.hpp"

namespace dm
{
    template<typename TYPE>
    class singleton
    {
    public:
        constexpr TYPE& unsafe_get()
        {
            return *data;
        }

        template<typename... ARGS>
        void construct(ARGS&&... args)
        {
            data.emplace(std::forward<ARGS>(args)...);
        }

        constexpr void free()
        {
            data.reset();
        }

        constexpr bool assigned()
        {
            return data.has_value();
        }

        singleton& operator=(TYPE&& val)
        {
            data = std::move(val);
            return *this;
        }

        // todo: check for correct thread and whatnot
        TYPE* operator->()
        {
            MX_ASSERT(can_call_from_current_thread(allowedThreads));
            return data.operator->();
        }

        TYPE const* operator->() const
        {
            MX_ASSERT(can_call_from_current_thread(allowedThreads));
            return data.operator->();
        }

        //
        // Forward operator calls through operator->()
        //
        template<typename ARG>
        auto operator[](ARG&& arg)
        {
            auto& ref = *this->operator->();
            return ref.operator[](std::forward<ARG>(arg));
        }

        template<typename... ARGS>
        auto operator()(ARGS&&... args)
        {
            auto& ref = *this->operator->();
            return ref.operator()(std::forward<ARGS>(args)...);
        }

        static singleton instance;
    private:
        std::optional<TYPE> data;
        const worker_flag allowedThreads = worker_flag::any;
    };
    template<typename TYPE>
    typename singleton<TYPE>::singleton singleton<TYPE>::singleton::instance;

    #define MX_MAKE_SINGLETON(TYPE) \
    inline constexpr dm::singleton<TYPE>& g_##TYPE = dm::singleton<TYPE>::instance;\
    template<typename... ARGS> \
    void init_##TYPE(ARGS&&... args) { dm::singleton<TYPE>::instance.construct(std::forward<ARGS>(args)...); } \
    constexpr void free_##TYPE() { dm::singleton<TYPE>::instance.free(); } \

}
