#pragma once

namespace dm
{
    template<typename TYPE>
    class singleton
    {
    public:
        constexpr TYPE& get()
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

        static singleton instance;
    private:
        std::optional<TYPE> data;
    };
    template<typename TYPE>
    typename singleton<TYPE>::singleton singleton<TYPE>::singleton::instance;

    #define MX_MAKE_SINGLETON(TYPE) \
    inline constexpr TYPE& g_##TYPE = dm::singleton<TYPE>::instance.get();\
    template<typename... ARGS> \
    void init_##TYPE(ARGS&&... args) { dm::singleton<TYPE>::instance.construct(std::forward<ARGS>(args)...); } \
    constexpr void free_##TYPE() { dm::singleton<TYPE>::instance.free(); } \

}