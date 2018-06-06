#pragma once

namespace tabletopify
{
    class utf8_string
    {
    public:
        utf8_string();
        utf8_string(const utf8_string&);
        utf8_string(utf8_string&&);
        utf8_string(const char*);
        utf8_string(const std::string&);
        utf8_string& operator=(const utf8_string&);
        utf8_string& operator=(utf8_string&&);
        utf8_string& operator=(const char*);
        utf8_string& operator=(const std::string&);

        utf8_string normalized() const;

        bool operator==(const utf8_string&) const;

        std::string str() const;
        const char* c_str() const;
        // number of bytes used to encode this string (not counting null terminator)
        size_t size() const;

    private:
        // private constructor, takes ownership of pointer
        utf8_string(size_t size, utf8proc_uint8_t*);
        // size in bytes excluding null terminator
        size_t _size;
        std::unique_ptr<utf8proc_uint8_t[], decltype(&std::free)> _data;
        static const char* _emptyString;

        friend struct uf8_string_hash;
        friend void to_json(nl::json&, const utf8_string&);
        friend void from_json(const nl::json&, utf8_string&);
    };

    void to_json(nl::json&, const utf8_string&);
    void from_json(const nl::json&, utf8_string&);

    struct utf8_string_hash
    {
        size_t operator()(const utf8_string&) const;
    };

    class utf8_string_buffer
    {
        utf8_string_buffer& add(utf8proc_int32_t);
        utf8_string_buffer& add(const utf8_string&);
        utf8_string_buffer& add(const char*);
    };  
}

std::ostream& operator<<(std::ostream& out, const tabletopify::utf8_string& str);
