#include "utf8_string.hpp"
#include "utilities.hpp"
#include "hash.hpp"
#include "logger.hpp"
#include "serialization_buffer.hpp"

namespace dm
{
    utf8_codepoint::utf8_codepoint(utf8proc_int32_t codepoint)
    : _codepoint(codepoint)
    { }


    bool utf8_codepoint::operator==(const utf8_codepoint& that) const
    {
        return this->_codepoint == that._codepoint;
    }

    bool utf8_codepoint::operator!=(const utf8_codepoint& that) const
    {
        return !this->operator==(that);
    }

    bool utf8_codepoint::is_valid() const
    {
        return utf8proc_codepoint_valid(_codepoint);
    }

    bool utf8_codepoint::is_whitespace() const
    {
        constexpr const static utf8proc_int32_t singles[] =
        {
            0x20,
            0x85,
            0xA0,
            0x1680,
            0x202F,
            0x205F,
            0x3000,
        };

        for(auto s : singles)
        {
            if(s == _codepoint) return true;
        }

        constexpr const static utf8proc_int32_t ranges[][2] =
        {
            {0x09,0x0D},
            {0x2000,0x200A},
            {0x2028,0x2029},
        };

        for(auto r : ranges)
        {
            if(r[0] <= _codepoint && _codepoint <= r[1]) return true;
        }

        return false;
    }

    utf8_codepoint utf8_codepoint::to_lower() const
    {
        return utf8_codepoint(utf8proc_tolower(_codepoint));
    }

    utf8_codepoint utf8_codepoint::to_upper() const
    {
        return utf8_codepoint(utf8proc_toupper(_codepoint));
    }

    // utf8_string::string_data

    utf8_string::string_data::string_data(size_t size, char const* data)
    : counter(1)
    , size(size)
    {
        std::copy(data, data + size, this->data);
        this->data[size] = 0;
    }

    utf8_string::string_data* utf8_string::string_data::increment()
    {
        ++counter;
        return this;
    }

    void utf8_string::string_data::decrement()
    {
        if(const auto count = --counter; count == 0)
        {
            operator delete(this);
        }
    }

    void* utf8_string::string_data::operator new(size_t, size_t size, std::nullptr_t)
    {
        // +1 for null-terminator
        return ::operator new(offsetof(string_data, data) + size + 1);
    }

    void utf8_string::string_data::operator delete(void* ptr, size_t, std::nullptr_t)
    {
        ::operator delete(ptr);
    }

    // Destructor

    utf8_string::~utf8_string()
    {
        if (stringData != nullptr)
        {
            stringData->decrement();
            stringData = nullptr;
        }
    }

    // Constructors

    utf8_string::utf8_string()
    : stringData(new (0) string_data(0, ""))
    { }

    utf8_string::utf8_string(const utf8_string& that)
    : stringData(that.stringData->increment())
    { }

    utf8_string::utf8_string(utf8_string&& that)
    : stringData(that.stringData)
    {
        that.stringData = nullptr;
    }

    utf8_string::utf8_string(const char* cstr)
    : utf8_string(cstr, std::strlen(cstr))
    { }

    utf8_string::utf8_string(const char* cstr, size_t len)
    : stringData(nullptr)
    {
        normalize_and_assign(cstr, len);
    }

    utf8_string::utf8_string(const std::string& str)
     : utf8_string(str.c_str(), str.size())
    { }

    utf8_string::utf8_string(const std::string_view sv)
    : utf8_string(sv.data(), sv.size())
    { }

    utf8_string::utf8_string(string_data* stringData)
    : stringData(stringData)
    { }

    // Assignment Operators
    utf8_string& utf8_string::operator=(const utf8_string& that)
    {
        this->stringData->decrement();
        this->stringData = that.stringData->increment();
        return *this;
    }

    utf8_string& utf8_string::operator=(utf8_string&& that)
    {
        this->stringData->decrement();
        this->stringData = that.stringData;
        that.stringData = nullptr;
        return *this;
    }

    utf8_string& utf8_string::operator=(const char* cstr)
    {
        normalize_and_assign(cstr, std::strlen(cstr));
        return *this;
    }

    utf8_string& utf8_string::operator=(const std::string& str)
    {
        normalize_and_assign(str.c_str(), str.size());
        return *this;
    }

    void utf8_string::normalize_and_assign(const char* cstr, size_t len)
    {
        MX_THROW_IF_NULL(cstr);

        // compose codepoints and what-not
        std::unique_ptr<utf8proc_uint8_t, decltype(&std::free)> data(nullptr, std::free);
        constexpr auto options = static_cast<utf8proc_option_t>(UTF8PROC_STABLE | UTF8PROC_COMPOSE | UTF8PROC_IGNORE);
        auto size = utf8proc_map(reinterpret_cast<utf8proc_uint8_t const*>(cstr), len, out(data), options);
        if(size < 0)
        {
            MX_THROW_MSG("{}", utf8proc_errmsg(size));
        }

        if (this->stringData != nullptr)
        {
            this->stringData->decrement();
            this->stringData = nullptr;
        }

        this->stringData = new (size) string_data(size, reinterpret_cast<char*>(data.get()));
    }

    // Equality Operators

    bool utf8_string::operator==(const utf8_string& that) const
    {
        if (this->stringData == that.stringData)
        {
            return true;
        }

        if (this->stringData->size != that.stringData->size)
        {
            return false;
        }

        return std::equal(this->stringData->data, this->stringData->data + this->stringData->size,
                          that.stringData->data);
    }

    bool utf8_string::operator!=(const utf8_string& that) const
    {
        return !this->operator==(that);
    }

    // Methods

    std::vector<utf8_string> utf8_string::split() const
    {
        std::vector<utf8_string> retval;
        utf8_stringbuffer sb;
        sb.reserve(this->size() + 1);

        for(auto cp : *this)
        {
            if(cp.is_whitespace())
            {
                if(sb.size() > 0)
                {
                    retval.push_back(sb.str());
                    sb.clear();
                }
            }
            else
            {
                sb.push_back(cp);
            }
        }

        if(sb.size() > 0)
        {
            retval.push_back(sb.str());
        }

        return retval;
    }

    utf8_string utf8_string::normalized() const
    {
        if (this->size() == 0)
        {
            return *this;
        }

        std::unique_ptr<utf8proc_uint8_t, decltype(&std::free)> data(nullptr, std::free);
        constexpr auto options = static_cast<utf8proc_option_t>(UTF8PROC_NULLTERM | UTF8PROC_CASEFOLD);
        auto size = utf8proc_map(reinterpret_cast<utf8proc_uint8_t*>(this->stringData->data), 0, out(data), options);

        if(size < 0)
        {
            MX_THROW_MSG("{}", utf8proc_errmsg(size));
        }

        return utf8_string(new(size) string_data(size, reinterpret_cast<char*>(data.get())));
    }

    std::string utf8_string::str() const
    {
        return std::string(this->c_str());
    }

    const char* utf8_string::c_str() const
    {
        return this->stringData->data;
    }

    const uint8_t* utf8_string::data() const
    {
        return reinterpret_cast<const uint8_t*>(this->c_str());
    }

    size_t utf8_string::size() const
    {
        return this->stringData->size;
    }

    bool utf8_string::empty() const
    {
        return this->size() == 0;
    }

    // Iterator Getters
    utf8_string_iterator utf8_string::begin() const
    {
        utf8proc_uint8_t* head = reinterpret_cast<utf8proc_uint8_t*>(const_cast<char*>(this->c_str()));
        auto tail = head + this->size();
        return utf8_string_iterator(head, tail);
    }

    utf8_string_iterator utf8_string::end() const
    {
        utf8proc_uint8_t* head = reinterpret_cast<utf8proc_uint8_t*>(const_cast<char*>(this->c_str()));
        auto tail = head + this->size();
        return utf8_string_iterator(tail, tail);
    }
//
// Hash objects
//

    size_t hash<dm::utf8_string>::operator()(const utf8_string& str, size_t seed) const
    {
        return calc_hash_raw(str.data(), str.size(), seed);
    };
}

namespace std
{
    size_t hash<dm::utf8_codepoint>::operator()(const dm::utf8_codepoint& cp) const
    {
        std::hash<utf8proc_int32_t> h;
        return h(cp.value());
    }

    size_t hash<dm::utf8_string>::operator()(const dm::utf8_string& str) const
    {
        dm::hash<dm::utf8_string> h;
        return h(str, 0);
    }
}

namespace dm
{
    void serialization_buffer_write(serialization_buffer& self, const utf8_string& str)
    {
        self.write(str.size());
        self.write(str.data(), str.size());
    }

    // Helper Functions


    // UTF8 String Iterator

    utf8_string_iterator::utf8_string_iterator(const utf8proc_uint8_t* head, const utf8proc_uint8_t* tail)
    : _currentCodepoint(head)
    , _nextCodepoint(head)
    , _tail(tail)
    , _codepoint(0)
    {
        this->iterate();
    }

    void utf8_string_iterator::iterate()
    {
        if(_nextCodepoint < _tail)
        {
            auto codepointSize = utf8proc_iterate(_nextCodepoint, _tail - _nextCodepoint, &_codepoint);
            if(codepointSize < 0)
            {
                MX_THROW_MSG("{}", utf8proc_errmsg(codepointSize));
            }
            _currentCodepoint = _nextCodepoint;
            _nextCodepoint += codepointSize;
        }
        else
        {
            _codepoint = 0;
        }
    }

    utf8_string_iterator& utf8_string_iterator::operator++()
    {
        this->iterate();
        return *this;
    }

    utf8_string_iterator utf8_string_iterator::operator++(int)
    {
        utf8_string_iterator retval = *this;
        this->iterate();
        return retval;
    }

    bool utf8_string_iterator::operator==(utf8_string_iterator that) const
    {
        return this->_nextCodepoint == that._nextCodepoint &&
               this->_tail == that._tail &&
               this->_codepoint == that._codepoint;
    }

    bool utf8_string_iterator::operator!=(utf8_string_iterator that) const
    {
        return !this->operator==(that);
    }

    utf8_codepoint utf8_string_iterator::operator*() const
    {
        return utf8_codepoint(_codepoint);
    }

    // UTF String View

    utf8_string_iterator utf8_string_view::begin() const
    {
        return utf8_string_iterator(_begin, _end);
    }

    utf8_string_iterator utf8_string_view::end() const
    {
        return utf8_string_iterator(_end, _end);
    }

    bool serialization_buffer_read(serialization_buffer& self, utf8_string_view& out)
    {
        size_t len = 0;
        if (self.read<size_t>(len))
        {
            const auto* read_head = self.read_head();
            if (self.skip(len))
            {
                out = utf8_string_view {
                    reinterpret_cast<const utf8proc_uint8_t*>(read_head),
                    reinterpret_cast<const utf8proc_uint8_t*>(read_head) + len
                };
                return true;
            }
        }
        return false;
    }

    // UTF8 Stringbuffer

    utf8_stringbuffer::utf8_stringbuffer(const utf8_string& str)
    {
        this->push_back(str);
    }

    utf8_stringbuffer& utf8_stringbuffer::operator=(const utf8_string& str)
    {
        this->clear();
        this->push_back(str);
        return *this;
    }

    void utf8_stringbuffer::push_back(const utf8_string& str)
    {
        for(auto cp : str)
        {
            this->push_back(cp);
        }
    }

    void utf8_stringbuffer::insert(const_iterator position, const utf8_string& str)
    {
        for(auto cp : str)
        {
            position = this->insert(position, cp) + 1;
        }
    }

    utf8_string utf8_stringbuffer::str() const
    {
        return str(this->begin(), this->end());
    }

    utf8_string utf8_stringbuffer::str(const_iterator from, const_iterator to) const
    {
        MX_ASSERT(to >= from);
        char buffer[(int32_t)(to - from) * 4];
        utf8proc_uint8_t* head = reinterpret_cast<utf8proc_uint8_t*>(buffer);
        size_t len = 0;
        for(auto it = from; it < to; ++it)
        {
            auto cpSize = utf8proc_encode_char(it->value(), head);
            if (cpSize < 0)
            {
                MX_THROW_MSG("{}", utf8proc_errmsg(cpSize));
            }
            head += cpSize;
            len += cpSize;
        }

        utf8_string retval(buffer, len);
        return retval;
    }

    utf8_stringbuffer::iterator utf8_stringbuffer::find_word_begin(iterator it, bool skipOverWhitespace)
    {
        if (it == this->begin())
        {
            return it;
        }

        if (skipOverWhitespace)
        {
            // iterate until we hit something that isn't whitespace
            for(--it ;it > this->begin() && it->is_whitespace(); --it);
        }

        const bool findWhitespace =  it->is_whitespace();
        // iterate back until we get to the opposite of what we want
        for(;it > this->begin() && it->is_whitespace() == findWhitespace; --it);

        if(it->is_whitespace() != findWhitespace) {
            ++it;
        }
        return it;
    }

    utf8_stringbuffer::iterator utf8_stringbuffer::find_word_end(iterator it, bool skipOverWhitespace)
    {
        if (it == this->end())
        {
            return it;
        }

        if (skipOverWhitespace)
        {
            for(; it < this->end() && it->is_whitespace(); ++it);
        }

        const bool findWhitespace = it->is_whitespace();
        // iterate forward until we get to the opposite of what we want
        for(; it < this->end() && it->is_whitespace() == findWhitespace; ++it);

        return it;
    }
}
