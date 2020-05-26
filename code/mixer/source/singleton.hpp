#pragma once

namespace tabletopify
{
    template<typename TYPE>
    class singleton
    {
    public:
        static constexpr TYPE* addr()
        {
            return &(*data);
        }

        static constexpr TYPE& get()
        {
            return *data;
        }

        template<typename... ARGS>
        static void construct(ARGS&&... args)
        {
            data.emplace(std::forward<ARGS>(args)...);
        }

        static constexpr void free()
        {
            data.reset();
        }

        static constexpr bool assigned()
        {
            return data.has_value();
        }

    private:
        static std::optional<TYPE> data;
    };
    template<typename TYPE>
    typename std::optional<TYPE> singleton<TYPE>::singleton::data;

    #define MX_MAKE_SINGLETON(TYPE) \
    constexpr TYPE* get_##TYPE() { return tabletopify::singleton<TYPE>::addr(); } \
    template<typename... ARGS> \
    void init_##TYPE(ARGS&&... args) { tabletopify::singleton<TYPE>::construct(std::forward<ARGS>(args)...); } \
    constexpr void free_##TYPE() { tabletopify::singleton<TYPE>::free(); } \
    inline constexpr TYPE& g_##TYPE = tabletopify::singleton<TYPE>::get();
}