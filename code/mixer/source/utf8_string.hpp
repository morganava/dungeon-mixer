#pragma once

namespace tabletopify
{
    class utf8_codepoint
    {
    public:
        const utf8proc_int32_t raw;
        utf8_codepoint(utf8proc_int32_t);
        bool operator==(const utf8_codepoint&) const;        
        bool operator!=(const utf8_codepoint&) const;

        bool is_valid() const;
        bool is_whitespace() const;
    };

    class utf8_string_iterator;    
    class utf8_string
    {
    public:
        utf8_string();
        utf8_string(const utf8_string&);
        utf8_string(utf8_string&&);
        utf8_string(const char*);
        utf8_string(const char*, size_t len);
        utf8_string(const std::string&);
        template<size_t N>
        utf8_string(const char(&str)[N]) : utf8_string(str, N) {}
        utf8_string& operator=(const utf8_string&);
        utf8_string& operator=(utf8_string&&);
        utf8_string& operator=(const char*);
        utf8_string& operator=(const std::string&);
        template<size_t N>
        utf8_string& operator=(const char(&str)[N])
        {
            normalize_and_assign(str, N);
            return *this;
        }

        bool operator==(const utf8_string&) const;
        bool operator!=(const utf8_string&) const;

        // splits on all whitespace codepoints
        std::vector<utf8_string> split() const;
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
        // normalizes and assigns a new string
        void normalize_and_assign(const char* cstr, size_t len);

        // size in bytes excluding null terminator
        size_t _size;
        // data buffer
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
                                            utf8_codepoint>
    {
    public:
        utf8_string_iterator& operator++();
        utf8_string_iterator operator++(int);
        bool operator==(utf8_string_iterator that) const;
        bool operator!=(utf8_string_iterator that) const;
        utf8_codepoint operator*() const;
    private:
        void iterate();
        explicit utf8_string_iterator(utf8proc_uint8_t* head, utf8proc_uint8_t* tail);
        const utf8proc_uint8_t* _nextCodepoint;
        const utf8proc_uint8_t* _tail;
        utf8proc_int32_t _codepoint;
        
        friend class utf8_string;
    };

    class utf8_stringbuffer
    {
    public:
        utf8_stringbuffer();
        utf8_stringbuffer(size_t initialSize);

        void push_back(utf8_codepoint);
        void push_back(const utf8_string&);
        void push_back(const std::string&);
        template<size_t N>
        void push_back(const char(&str)[N])
        {
            for(size_t i = 0; i < N - 1; ++i) {
                _data.push_back(str[i]);
            }
        }

        // number of bytes (excluding null terminator)
        size_t size() const;
        utf8_string str() const;
        void clear();
    private:
        void push_cstr(const char* str);


        mutable std::vector<utf8proc_uint8_t> _data;
    };
}

std::ostream& operator<<(std::ostream& out, const tabletopify::utf8_string& str);
