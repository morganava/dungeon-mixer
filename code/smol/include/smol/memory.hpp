#pragma once

namespace smol
{
    template<typename T, typename Deleter> class out_unique_ptr;

    template<typename T, typename Deleter>
    out_unique_ptr<T, Deleter> out(std::unique_ptr<T, Deleter>& ptr);

    template<typename T, typename  Deleter>
    class out_unique_ptr
    {
    public:
        out_unique_ptr() = delete;
        out_unique_ptr(const out_unique_ptr&) = delete;
        out_unique_ptr(out_unique_ptr&&) = delete;
        out_unique_ptr& operator=(const out_unique_ptr&) = delete;
        out_unique_ptr& operator=(out_unique_ptr&&) = delete;

        ~out_unique_ptr()
        {
            u.reset(ptr);
        }

        constexpr operator T**()
        {
            return &ptr;
        }
    private:
        out_unique_ptr(std::unique_ptr<T, Deleter>& u) : u(u) {}
        friend out_unique_ptr out<T, Deleter>(std::unique_ptr<T, Deleter>&);

        T* ptr = nullptr;
        std::unique_ptr<T, Deleter>& u;
    };

    // usage for legacy APIs with out pointer variables
    //
    // void give_int(int** outInt)
    // std::unique_ptr<int> pint;
    // give_int(out(pint));
    // int val = *pint;
    template<typename T, typename Deleter>
    out_unique_ptr<T, Deleter> out(std::unique_ptr<T, Deleter>& ptr)
    {
        return {ptr};
    }
}
