#pragma once

namespace tabletopify
{

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
        std::stringstream ss;
        ss << getenv("HOME") << '/';
        return ss.str();
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


    inline
    fs::path get_tabletopify_path()
    {
        std::stringstream ss;
        ss << get_home_path().native() << ".tabletopify/";
        return ss.str();
    }

    // ensures object is only accessed from a single thread
    template<typename T>
    class single_threaded
    {
    public:
        single_threaded() {}

        single_threaded(std::thread::id id)
        : _id(id)
        , _val(new T())
        {}

        single_threaded(std::thread::id id, T* val)
        : _id(id)
        , _val(val)
        {}

        void free()
        {
            delete _val;
            _val = nullptr;
        }

        T* operator->()
        {
            assert(std::this_thread::get_id() == _id);
            return _val;
        }
    private:
        std::thread::id _id;
        T* _val = nullptr;
    };
}