#include "utf8_string.hpp"
#include "utilities.hpp"
#include "hash.hpp"


namespace tabletopify
{
    const char* utf8_string::_emptyString = "";

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
        MX_THROW_IF_NULL(cstr);
        if(*cstr == 0)
        {
            this->_size = 0;
            this->_data.reset(nullptr);
            return *this;
        }

        // compose codepoints and what-not
        utf8proc_uint8_t* data = nullptr;
        utf8proc_option_t options = static_cast<utf8proc_option_t>(UTF8PROC_NULLTERM | UTF8PROC_STABLE | UTF8PROC_COMPOSE | UTF8PROC_IGNORE);
        auto size = utf8proc_map(reinterpret_cast<const utf8proc_uint8_t*>(cstr), 0, &data, options);
        if(size < 0)
        {
            MX_THROW_MSG(utf8proc_errmsg(size));
        }

        this->_size = size;
        this->_data.reset(data);

        return *this;
    }

    utf8_string& utf8_string::operator=(const std::string& str)
    {
        return *this = str.c_str();
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
                MX_THROW_MSG(utf8proc_errmsg(size));
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

    void to_json(nl::json& js, const utf8_string& str)
    {
        js = str.str();
    }

    void from_json(const nl::json& js, utf8_string& str)
    {
        MX_ASSERT(js.is_string());
        str = (std::string)js;
    }

    size_t utf8_string_hash::operator()(const utf8_string& str) const
    {
        return calc_hash(str.c_str(), str.size());
    }

    // UTF8 String Iterator

    // utf8_string_iterator::utf8_string_iterator(const utf8_string& str, size_t index)
    // : _nextCodepoint(str._data.get() + index)
    // , _tail(_nextCodepoint + str.size())
    // { 
    //     iterate();
    // }

    utf8_string_iterator::utf8_string_iterator(utf8proc_uint8_t* head, utf8proc_uint8_t* tail)
    : _nextCodepoint(head)
    , _tail(tail)
    , _codepoint(0)
    {
        iterate();
    }

    void utf8_string_iterator::iterate()
    {
        if(_nextCodepoint < _tail)
        {
            auto codepointSize = utf8proc_iterate(_nextCodepoint, _tail - _nextCodepoint, &_codepoint);
            if(codepointSize < 0)
            {
                MX_THROW_MSG(utf8proc_errmsg(codepointSize));
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
        iterate();
        return *this;
    }

    utf8_string_iterator utf8_string_iterator::operator++(int)
    {
        utf8_string_iterator retval = *this;
        iterate();
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

    utf8proc_int32_t utf8_string_iterator::operator*() const
    {
        return _codepoint;
    }
}

std::ostream& operator<<(std::ostream& out, const tabletopify::utf8_string& str)
{
    return out << str.c_str();
}
