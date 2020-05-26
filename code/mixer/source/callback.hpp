#include "singleton.hpp"

namespace tabletopify
{
    template<typename... ARGS>
    class callback
    {
    public:
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
                MX_ASSERT(singleton<FUNCTOR>::assigned() == false);

                // move to static storage
                singleton<FUNCTOR>::construct(std::move(functor));

                this->exec_func = [](ARGS... args) -> void
                {
                    singleton<FUNCTOR>::get()(std::forward<ARGS>(args)...);
                };
                this->delete_func = &singleton<FUNCTOR>::free;
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
    private:
        void(*delete_func)() = nullptr;
        void(*exec_func)(ARGS...) = nullptr;
    };
}