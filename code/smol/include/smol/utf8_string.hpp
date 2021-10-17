#pragma once

#include <smol/hash.hpp>
#include <smol/serialization_buffer.hpp>

namespace smol
{
    class utf8_string;
}

namespace nlohmann
{
    void to_json(nl::json&, const smol::utf8_string&);
    void from_json(const nl::json&, smol::utf8_string&);
}

namespace smol
{
    class utf8_codepoint
    {
    public:
        utf8_codepoint() = default;
        utf8_codepoint(utf8proc_int32_t);
        utf8_codepoint& operator=(const utf8_codepoint&) = default;
        bool operator==(const utf8_codepoint&) const;
        bool operator!=(const utf8_codepoint&) const;

        bool is_valid() const;
        bool is_whitespace() const;

        utf8_codepoint to_lower() const;
        utf8_codepoint to_upper() const;

        // operator utf8proc_int32_t() const {return _codepoint;}
        utf8proc_int32_t value() const {return _codepoint;}
    private:
        utf8proc_int32_t _codepoint = 0;
    };

    class utf8_string_iterator;
    class utf8_string
    {
    public:
        ~utf8_string();
        utf8_string();
        utf8_string(const utf8_string&);
        utf8_string(utf8_string&&);
        utf8_string(const char*);
        utf8_string(const char*, size_t len);
        utf8_string(const std::string&);
        utf8_string(const std::string_view);
        template<size_t N>
        utf8_string(const char(&str)[N]) : utf8_string(str, N-1)
        { }
        utf8_string& operator=(const utf8_string&);
        utf8_string& operator=(utf8_string&&);
        utf8_string& operator=(const char*);
        utf8_string& operator=(const std::string&);
        template<size_t N>
        utf8_string& operator=(const char(&str)[N])
        {
            normalize_and_assign(str, N-1);
            return *this;
        }

        bool operator==(const utf8_string&) const;
        bool operator!=(const utf8_string&) const;

        // splits on all whitespace codepoints
        std::vector<utf8_string> split() const;
        // returns utf8_string for case-insensitive comparisons
        utf8_string normalized() const;

        std::string str() const;
        const char* c_str() const;
        const uint8_t* data() const;
        // number of bytes used to encode this string (not counting null terminator)
        size_t size() const;
        [[nodiscard]] bool empty() const;

        utf8_string_iterator begin() const;
        utf8_string_iterator end() const;

    private:
        // refcounted variable-length struct that holds our shared string data
        struct string_data
        {
            // the number of references to this data
            std::atomic<size_t> counter;
            // size of the string (not including null-terminator)
            size_t size;
            // our string data
            char data[1];

            string_data(size_t size, char const* data);
            string_data* increment();
            void decrement();
            void* operator new(size_t, size_t size, std::nullptr_t=nullptr);
            void operator delete(void* ptr, size_t=0, std::nullptr_t=nullptr);
        };
        mutable string_data* stringData = nullptr;

        // private constructor, takes ownership of pointer
        utf8_string(string_data*);
        // normalizes  and assigns a new string
        void normalize_and_assign(const char* cstr, size_t len);

        friend void nl::to_json(nl::json&, const utf8_string&);
        friend void nl::from_json(const nl::json&, utf8_string&);
        friend class utf8_string_iterator;
    };

    void serialization_buffer_write(smol::serialization_buffer& self, const utf8_string& val);

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
        bool operator<(utf8_string_iterator that) const;
        bool operator>(utf8_string_iterator that) const;
        utf8_codepoint operator*() const;
        operator const char*() const { return reinterpret_cast<const char*>(_currentCodepoint); }
    private:
        void iterate();
        explicit utf8_string_iterator(const utf8proc_uint8_t* head, const utf8proc_uint8_t* tail);
        const utf8proc_uint8_t* _currentCodepoint = nullptr;
        const utf8proc_uint8_t* _nextCodepoint = nullptr;
        const utf8proc_uint8_t* _tail = nullptr;
        utf8proc_int32_t _codepoint;

        friend class utf8_string;
        friend class utf8_string_view;
    };

    class utf8_string_view
    {
    public:
        utf8_string_view() = default;
        utf8_string_view(const utf8_string_view& sv)
        : _begin(sv._begin)
        , _end(sv._end)
        { }
        utf8_string_view(const utf8proc_uint8_t* begin, const utf8proc_uint8_t* end)
        : _begin(begin)
        , _end(end)
        { }
        utf8_string_view(const utf8proc_int8_t* begin, const utf8proc_int8_t* end)
        : _begin(reinterpret_cast<const utf8proc_uint8_t*>(begin))
        , _end(reinterpret_cast<const utf8proc_uint8_t*>(end))
        { }

        utf8_string_iterator begin() const;
        utf8_string_iterator end() const;
        constexpr size_t size() const {return _end - _begin;};
    private:
        const utf8proc_uint8_t* _begin = nullptr;
        const utf8proc_uint8_t* _end = nullptr;
    };

    bool serialization_buffer_read(smol::serialization_buffer& self, utf8_string_view& out);

    class utf8_stringbuffer : public std::vector<utf8_codepoint>
    {
    public:
        using vector::vector;
        using vector::push_back;
        using vector::insert;
        using vector::operator=;

        utf8_stringbuffer(const utf8_string&);
        utf8_stringbuffer& operator=(const utf8_string&);
        void push_back(const utf8_string&);
        void insert(const_iterator position, const utf8_string& str);
        utf8_string str() const;
        utf8_string str(const_iterator from, const_iterator to) const;

        iterator find_word_begin(iterator from, bool skipOverWhitespace);
        iterator find_word_end(iterator from, bool skipOverWhitespace);
    };

    size_t calc_hash(const smol::utf8_string& str, size_t seed);
}

std::ostream& operator<<(std::ostream& out, const smol::utf8_codepoint cp);
std::ostream& operator<<(std::ostream& out, const smol::utf8_string& str);
std::ostream& operator<<(std::ostream& out, const smol::utf8_string_view& str);

SMOL_MAKE_STD_HASH(smol::utf8_codepoint);
SMOL_MAKE_STD_HASH(smol::utf8_string);
