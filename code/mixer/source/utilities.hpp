#pragma once

#define MX_DEBUGBREAK(...) __builtin_trap()

#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)
#define __STR_LINE__ STRINGIFY(__LINE__)

#define MX_ASSERT(X) do {bool result = (X); if(!result) { tabletopify::assertion_log("Assertion Failure: " __FILE__ ":" __STR_LINE__ " " #X); MX_DEBUGBREAK();} } while(false);

namespace tabletopify
{
    void assertion_log(const char* msg);

    // print operator for some nuklear types
    inline
    std::ostream& operator<<(std::ostream& out, struct nk_rect& rect)
    {
        return out << "{ x: " << rect.x << ", y: " << rect.y << ", w: " << rect.w << ", h: " << rect.h << " }";
    }

    inline
    std::ostream& operator<<(std::ostream& out, struct nk_vec2& vec)
    {
        return out << "{ x: " << vec.x << ", y: " << vec.y << " }";
    }

    inline
    std::ostream& operator<<(std::ostream& out, const sf::Vector3f& vec)
    {
        return out << "{ x: " << vec.x << ", y: " << vec.y << ", z: " << vec.z << " }";
    }

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
    bool contains(const std::string& str, const std::string& pattern)
    {
        bool result = std::strstr(str.data(), pattern.data()) != nullptr;
        return result;
    }

    // convert string to lower-case (not a general utf-8 solution)
    inline
    std::string to_lower(const std::string& str)
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    // remove whitespace from head and tail of string
    inline
    std::string trim(const std::string& str)
    {
        size_t first = str.find_first_not_of(" \t");
        if(first == std::string::npos) return "";

        size_t last = str.find_last_not_of(" \t");

        return std::string(str, first, last - first + 1);
    }

    inline
    void sleep_for(size_t ms)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    // verifies a file exists and can be opened and read
    inline
    void verify_file(std::string& path)
    {
        auto cpath = path.c_str();

        auto* file = fopen(cpath, "rb");
        if(file == nullptr)
        {
            throw std::runtime_error(string_format("Could not open file \"%s\"", cpath));
        }
        fclose(file);
        printf("Verified: %s\n", cpath);
    }

    // https://stackoverflow.com/questions/2552416/how-can-i-find-the-users-home-dir-in-a-cross-platform-manner-using-c
    inline
    fs::path get_home_path()
    {
        static fs::path home = std::string(getenv("HOME")) + "/";
        return home;
    }

    inline
    std::vector<fs::path> get_subdirectories(fs::path path)
    {
        std::vector<fs::path> result;

        for(auto& p : fs::directory_iterator(path))
        {
            result.push_back(p);
        }

        return result;
    }

    fs::path open_dialog(const char* filterList);
    fs::path save_dialog(const char* filterList);


    inline
    fs::path get_tabletopify_path()
    {
        static fs::path tabletopify = get_home_path() / ".tabletopify";
        return tabletopify;
    }

    struct text_buffer
    {
    public:
        text_buffer() : text_buffer(2) {}

        text_buffer(size_t capacity)
        : _capacity(capacity)
        , _buffer(new char[_capacity])
        {
            for(size_t idx = 0; idx < _capacity; ++idx)
                _buffer[idx] = 0;
        }

        size_t capacity() const { return _capacity; }
        char* buffer() { return _buffer.get(); }

        void update();
        text_buffer& operator=(const std::string& str);
        operator std::string() const;

    private:
        size_t _capacity;
        std::unique_ptr<char[]> _buffer;
    };
}