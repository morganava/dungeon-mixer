#include "singleton.hpp"

namespace dm
{
    // todo rename singleton_callback
    template<typename... ARGS>
    class callback
    {
    public:
        callback() = default;
        callback(const callback&) = delete;
        callback(callback&&) = delete;
        callback& operator=(const callback&) = delete;
        callback& operator=(callback&&) = delete;

        ~callback()
        {
            if (delete_func) {
                delete_func();
            }
        }

        template<typename FUNCTOR>
        callback& operator=(FUNCTOR&& functor)
        {
            MX_ASSERT(delete_func == nullptr && exec_func == nullptr);
            // convertible to a raw function pointer
            if constexpr (std::is_convertible<FUNCTOR, void(*)(ARGS...)>::value)
            {
                this->exec_func = static_cast<void(*)(ARGS...)>(functor);
                this->delete_func = nullptr;
            }
            // functor object
            else
            {
                // ensure we're not overriding a callback somehow
                MX_ASSERT(singleton<FUNCTOR>::instance.assigned() == false);

                // move to static storage
                singleton<FUNCTOR>::instance.construct(worker_flag::any, std::move(functor));

                this->exec_func = [](ARGS... args) -> void
                {
                    singleton<FUNCTOR>::instance(std::forward<ARGS>(args)...);
                };
                this->delete_func = []() -> void
                {
                    singleton<FUNCTOR>::instance.free();
                };
            }

            return *this;
        }

        constexpr void operator()(auto&&... args)
        {
            if (exec_func)
            {
                exec_func(std::forward<decltype(args)>(args)...);
            }
        }

        operator bool() const
        {
            return exec_func != nullptr;
        }
    private:
        void(*delete_func)() = nullptr;
        void(*exec_func)(ARGS...) = nullptr;
    };
}