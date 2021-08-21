#pragma once

namespace smol
{
    template<typename TYPE>
    class singleton
    {
    public:
        constexpr TYPE* get()
        {
            return data.operator->();
        }

        constexpr TYPE* get() const
        {
            return data.operator->();
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

        constexpr TYPE* operator->()
        {
            return data.operator->();
        }

        constexpr TYPE const* operator->() const
        {
            return data.operator->();
        }

        //
        // Forward operator calls through operator->()
        //
        template<typename ARG>
        constexpr auto operator[](ARG&& arg)
        {
            auto& ref = *this->operator->();
            return ref.operator[](std::forward<ARG>(arg));
        }

        template<typename... ARGS>
        constexpr auto operator()(ARGS&&... args)
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
}
