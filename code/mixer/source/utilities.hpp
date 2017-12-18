#pragma once

namespace tabletopify
{

    // countof(X)
    template<typename T, size_t N>
    constexpr size_t countof(const T (&)[N]) { return N; }

    inline
    std::string string_format(const char* format, ...)
    {
        va_list args1, args2;

        va_start (args1, format);
        // vsnprintf destructively modifes the va_list, so make an initial copy
        va_copy(args2, args1);
        auto len = vsnprintf(nullptr, 0, format, args1);
        va_end(args1); 

        std::string result(len, 0);
        vsnprintf(const_cast<char*>(result.data()), len+1, format, args2);
        va_end(args2);

        return result;
    }

    inline
    std::vector<std::string> split(const std::string& str, const std::string& delim)
    {
        std::vector<std::string> result;
        const char* head = str.data();
        const char* tail;
        const char* end = &*str.cend();
        do
        {
            tail = std::strstr(head, delim.c_str());
            if(tail == nullptr)
            {
                tail = end;
            }

            if(tail > head)
            {
                result.push_back(std::string(head, tail));
            }

            head = tail+delim.size();
        }
        while(tail != end);

        return result;
    }

    inline
    void yield(size_t ms)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

#if 0
    template<typename T>
    class synchronized
    {
    public:
        synchronized() {}

        synchronized(const T& val)
        : _val(val)
        { }

        synchronized(T&& val)
        : _val(val) 
        { }

        synchronized& operator=(const T& val)
        {
            std::scoped_lock lock(_mutex);
            _val = val;
            return *this;
        }

        synchronized& operator=(T&& val)
        {
            std::scoped_lock lock(_mutex);
            _val = val;
            return *this;
        }

        synchronized& apply(std::function<void(T& val)>&& func)
        {
            std::scoped_lock lock(_mutex);
            func(_val);
            return *this;
        }

        const synchronized& apply(std::function<void(const T& val)>&& func) const
        {
            std::scoped_lock lock(_mutex);
            func(_val);
            return *this;
        }
#if 0
        T get() const
        {
            std::scoped_lock lock(_mutex);
            return _val;
        }
#endif        
    private:
        mutable std::mutex _mutex;
        T _val;
    };
#endif    
}
