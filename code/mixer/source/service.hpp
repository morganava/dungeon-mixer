#pragma once

#include "worker_enums.hpp"

namespace dm
{
    template<typename TYPE>
    class service
    {
    public:
        template<typename... ARGS>
        constexpr service(ARGS&&... args) : data(std::forward<ARGS>(args)...) { }
        service(service&&) = default;
        service& operator=(service&&) = default;

        constexpr TYPE& unsafe_get()
        {
            return data;
        }

        constexpr TYPE& get()
        {
            if constexpr (allowed_threads<TYPE>::value != worker_flag::any)
            {
                SMOL_ASSERT(can_call_from_current_thread(allowed_threads<TYPE>::value));
            }
            return data;
        }

        constexpr TYPE* operator->()
        {
            if constexpr (allowed_threads<TYPE>::value != worker_flag::any)
            {
                SMOL_ASSERT(can_call_from_current_thread(allowed_threads<TYPE>::value));
            }
            return &data;
        }

        constexpr TYPE const* operator->() const
        {
            if constexpr (allowed_threads<TYPE>::value != worker_flag::any)
            {
                SMOL_ASSERT(can_call_from_current_thread(allowed_threads<TYPE>::value));
            }
            return &data;
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

        TYPE data;
    };

    #define MX_MAKE_SERVICE_IMPL(TYPE, NAME, ALLOWED_THREADS) \
        inline constexpr dm::service<TYPE>& g_##NAME = *smol::singleton<dm::service<TYPE>>::instance.get(); \
        inline constexpr TYPE& g_##NAME##_unsafe = g_##NAME.unsafe_get(); \
        template<typename... ARGS> \
        void init_##NAME(ARGS&&... args) { smol::singleton<dm::service<TYPE>>::instance.construct(std::forward<ARGS>(args)...);} \
        constexpr void free_##NAME() { smol::singleton<dm::service<TYPE>>::instance.free(); } \
        template<> struct allowed_threads<TYPE> { constexpr static worker_flag_t value=(ALLOWED_THREADS); };

    #define MX_MAKE_SERVICE(TYPE, ALLOWED_THREADS) MX_MAKE_SERVICE_IMPL(TYPE, TYPE, ALLOWED_THREADS)
}