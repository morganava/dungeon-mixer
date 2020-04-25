#pragma once

namespace tabletopify
{
    template<typename TYPE>
    class singleton
    {
    public:
        static constexpr TYPE* addr()
        {
            return &data.value;
        }

        static constexpr TYPE& get()
        {
            return data.value;
        }

        template<typename... ARGS>
        static void construct(ARGS&&... args)
        {
            if (data.assigned) {
                free();
            }
            new (addr()) TYPE(std::forward<ARGS>(args)...);
            data.assigned = true;
        }

        static void move(TYPE&& that)
        {
            construct(std::forward<TYPE&&>(that));
        }

        static constexpr void free()
        {
            data.value.~TYPE();
            data.assigned = false;
        }

        static constexpr bool assigned()
        {
            return data.assigned;
        }

    private:
        struct singleton_data
        {
            union
            {
                TYPE value;
                uint8_t raw[sizeof(TYPE)];
            };
            bool assigned;
            singleton_data()
            : raw()
            , assigned(false)
            { }

            ~singleton_data() {}
        };
        static singleton_data data;
    };
    template<typename TYPE>
    typename singleton<TYPE>::singleton_data singleton<TYPE>::singleton::data;

    #define MX_MAKE_SINGLETON(TYPE) \
    constexpr TYPE* get_##TYPE() { return tabletopify::singleton<TYPE>::addr(); } \
    template<typename... ARGS> \
    void init_##TYPE(ARGS&&... args) { tabletopify::singleton<TYPE>::construct(std::forward<ARGS>(args)...); } \
    constexpr void free_##TYPE() { tabletopify::singleton<TYPE>::free(); } \
    inline constexpr TYPE& g_##TYPE = tabletopify::singleton<TYPE>::get();
}