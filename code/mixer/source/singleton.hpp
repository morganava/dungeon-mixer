#pragma once

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
        void construct(worker_flag allowedThreads, ARGS&&... args)
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

        TYPE* operator->()
        {
            SMOL_ASSERT(can_call_from_current_thread(allowed_threads<TYPE>::value));
            return data.operator->();
        }

        TYPE const* operator->() const
        {
            // do a constexpr to see if the any flag is set, if so don't bother asserting
            SMOL_ASSERT(can_call_from_current_thread(allowed_threads<TYPE>::value));
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
    };
    template<typename TYPE>
    typename singleton<TYPE>::singleton singleton<TYPE>::singleton::instance;


    #define MX_MAKE_SINGLETON_IMPL(TYPE, NAME, ALLOWED_THREADS) \
        inline constexpr dm::singleton<TYPE>& g_##NAME = dm::singleton<TYPE>::instance; \
        inline constexpr TYPE& g_##NAME##_unsafe = dm::singleton<TYPE>::instance.unsafe_get(); \
        template<typename... ARGS> \
        void init_##NAME(ARGS&&... args) { dm::singleton<TYPE>::instance.construct(worker_flag::any, std::forward<ARGS>(args)...); } \
        constexpr void free_##NAME() { dm::singleton<TYPE>::instance.free(); } \
        template<> struct allowed_threads<TYPE> { constexpr static worker_flag_t value=(ALLOWED_THREADS); };

    #define MX_MAKE_SINGLETON(TYPE, ALLOWED_THREADS) MX_MAKE_SINGLETON_IMPL(TYPE, TYPE, ALLOWED_THREADS)
}
