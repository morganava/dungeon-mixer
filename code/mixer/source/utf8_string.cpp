#include "utf8_string.hpp"
#include "utilities.hpp"
#include "hash.hpp"
#include "logger.hpp"

namespace tabletopify
{
    const char* utf8_string::_emptyString = "";

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
        utf8proc_int32_t singles[] =
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

        utf8proc_int32_t ranges[][2] = 
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

    // Constructors

    utf8_string::utf8_string()
    : _size(0)
    , _data(nullptr, std::free)
    { }

    utf8_string::utf8_string(const utf8_string& that)
    : _size(0)
    , _data(nullptr, std::free)
    {
        *this = that;
    }

    utf8_string::utf8_string(utf8_string&& that)
    : _size(0)
    , _data(nullptr, std::free)
    {
        *this = that;
    }

    utf8_string::utf8_string(const char* cstr)
    : _size(0)
    , _data(nullptr, std::free)
    {
        *this = cstr;
    }

    utf8_string::utf8_string(const char* cstr, size_t len)
    : _size(0)
    , _data(nullptr, std::free)
    {
        normalize_and_assign(cstr, len);
    }

    utf8_string::utf8_string(const std::string& str)
     : utf8_string(str.c_str())
    { }

    utf8_string::utf8_string(size_t size, utf8proc_uint8_t* data)
    : _size(size)
    , _data(data, std::free)
    { }

    // Assignment Operators

    utf8_string& utf8_string::operator=(const utf8_string& that)
    {
        if(that._size == 0)
        {
            this->_size = 0;
            this->_data.reset(nullptr);
            return *this;
        }

        // copy buffer from that
        const auto n = that._size + 1;
        if(this->_size != that._size)
        {
            this->_size = that._size;
            this->_data.reset(reinterpret_cast<utf8proc_uint8_t*>(std::calloc(n, sizeof(utf8proc_uint8_t))));
        }
        std::memcpy(this->_data.get(), that._data.get(), n);

        return *this;
    }

    utf8_string& utf8_string::operator=(utf8_string&& that)
    {
        this->_size = that._size;
        this->_data = std::move(that._data);
        that._size = 0;

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
        if(*cstr == 0)
        {
            this->_size = 0;
            this->_data.reset(nullptr);
            return;
        }

        // compose codepoints and what-not
        utf8proc_uint8_t* data = nullptr;
        utf8proc_option_t options = static_cast<utf8proc_option_t>(UTF8PROC_STABLE | UTF8PROC_COMPOSE | UTF8PROC_IGNORE);
        auto size = utf8proc_map(reinterpret_cast<const utf8proc_uint8_t*>(cstr), len, &data, options);
        if(size < 0)
        {
            MX_THROW_MSG("{}", utf8proc_errmsg(size));
        }

        this->_size = size;
        this->_data.reset(data);        
    }

    // Equality Operators

    bool utf8_string::operator==(const utf8_string& that) const
    {
        if(this == &that)
        {
            return true;
        }
        
        if(this->_size != that._size)
        {
            return false;
        }

        for(size_t k = 0; k < this->_size; k++)
        {
            if(this->_data[k] != that._data[k])
            {
                return false;
            }
        }

        return true;
    }

    bool utf8_string::operator!=(const utf8_string& that) const
    {
        return !this->operator==(that);
    }

    // Methods

    std::vector<utf8_string> utf8_string::split() const
    {
        std::vector<utf8_string> retval;
        utf8_stringbuffer sb(this->_size + 1);

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
        if(this->_size > 0)
        {
            MX_ASSERT(this->_data.get() != nullptr);

            utf8proc_uint8_t* data = nullptr;
            utf8proc_option_t options = static_cast<utf8proc_option_t>(UTF8PROC_NULLTERM | UTF8PROC_CASEFOLD);
            auto size = utf8proc_map(this->_data.get(), 0, &data, options);
            if(size < 0)
            {
                MX_THROW_MSG("{}", utf8proc_errmsg(size));
            }

            return utf8_string(size, data);
        }

        return utf8_string();
    }

    std::string utf8_string::str() const
    {
        return std::string(this->c_str());
    }

    const char* utf8_string::c_str() const
    {
        if(this->_size == 0)
        {
            return _emptyString;
        }
        return reinterpret_cast<const char*>(_data.get());
    }

    size_t utf8_string::size() const
    {
        return this->_size;
    }

    // Iterator Getters
    utf8_string_iterator utf8_string::begin() const
    {
        utf8proc_uint8_t* head = this->_data.get();
        auto tail = head + this->_size;
        return utf8_string_iterator(head, tail);
    }

    utf8_string_iterator utf8_string::end() const
    {
        utf8proc_uint8_t* head = this->_data.get();
        auto tail = head + this->_size;
        return utf8_string_iterator(tail, tail);
    }

    // Helper Functions


    // UTF8 String Iterator

    utf8_string_iterator::utf8_string_iterator(const utf8proc_uint8_t* head, const utf8proc_uint8_t* tail)
    : _nextCodepoint(head)
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

    // UTF8 Stringbuffer

    utf8_stringbuffer::utf8_stringbuffer() { }

    utf8_stringbuffer::utf8_stringbuffer(size_t initialSize)
    {
        this->_data.reserve(initialSize);
    }

    void utf8_stringbuffer::push_back(utf8_codepoint codepoint)
    {
        char buffer[4] = {0};
        auto codepointSize = utf8proc_encode_char(codepoint, (utf8proc_uint8_t*)buffer);
        if(codepointSize < 0)
        {
            MX_THROW_MSG("{}", utf8proc_errmsg(codepointSize));
        }
        
        for(decltype(codepointSize) k = 0; k < codepointSize; ++k)
        {
            _data.push_back(buffer[k]);
        }
    }

    void utf8_stringbuffer::push_back(const utf8_string& str)
    {
        _data.insert(_data.end(), str.c_str(), str.c_str() + str.size());
    }

    void utf8_stringbuffer::push_back(const std::string& str)
    {
        _data.insert(_data.end(), str.c_str(), str.c_str() + str.size());
    }

    size_t utf8_stringbuffer::size() const
    {
        return _data.size();
    }

    utf8_string utf8_stringbuffer::str() const
    {
        utf8_string result(reinterpret_cast<const char*>(_data.data()), _data.size());
        return result;
    }

    void utf8_stringbuffer::clear()
    {
        _data.clear();
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
}
