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
            contruct(std::forward<TYPE&&>(that));
        }

        static constexpr void free()
        {
            data.value.~TYPE();
            data.assigned = false;            
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
}