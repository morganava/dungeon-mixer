#pragma once

namespace tabletopify
{

    class utf8_string_iterator;    
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

        bool operator==(const utf8_string&) const;
        bool operator!=(const utf8_string&) const;

        utf8_string normalized() const;
        std::string str() const;
        const char* c_str() const;
        // number of bytes used to encode this string (not counting null terminator)
        size_t size() const;

        utf8_string_iterator begin() const;
        utf8_string_iterator end() const;

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
        friend class utf8_string_iterator;
    };

    void to_json(nl::json&, const utf8_string&);
    void from_json(const nl::json&, utf8_string&);

    struct utf8_string_hash
    {
        size_t operator()(const utf8_string&) const;
    };

    class utf8_string_iterator : public std::iterator<
                                            std::forward_iterator_tag,
                                            utf8proc_int32_t,
                                            std::ptrdiff_t,
                                            const utf8proc_int32_t*,
                                            utf8proc_int32_t>
    {
    private:
        const utf8proc_uint8_t* _nextCodepoint;
        const utf8proc_uint8_t* _tail;
        utf8proc_int32_t _codepoint;

        void iterate();
        explicit utf8_string_iterator(utf8proc_uint8_t* head, utf8proc_uint8_t* tail);        
    public:
        utf8_string_iterator& operator++();
        utf8_string_iterator operator++(int);
        bool operator==(utf8_string_iterator that) const;
        bool operator!=(utf8_string_iterator that) const;
        utf8proc_int32_t operator*() const;

        friend class utf8_string;
    };

    class utf8_string_buffer
    {
        utf8_string_buffer& put(utf8proc_int32_t);
        utf8_string_buffer& put(const utf8_string&);

        void reset();
        utf8_string str() const;
    };  
}

std::ostream& operator<<(std::ostream& out, const tabletopify::utf8_string& str);
