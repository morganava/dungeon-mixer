#include "utf8_string.hpp"
#include "utilities.hpp"


namespace tabletopify
{
    const char* utf8_string::_emptyString = "";

    // Constructors

    utf8_string::utf8_string()
    : _size(0)
    , _data(nullptr, std::free)
    { }

    utf8_string::utf8_string(const utf8_string& that)
    : _size(that._size)
    , _data(nullptr, std::free)
    {
        if(_size > 0)
        {
            const auto n = this->_size + 1;
            this->_data.reset(reinterpret_cast<utf8proc_uint8_t*>(std::calloc(n, sizeof(utf8proc_uint8_t))));
            std::memcpy(this->_data.get(), that._data.get(), n);
        }
    }

    utf8_string::utf8_string(utf8_string&& that)
    : _size(that._size)
    , _data(std::move(that._data))
    {
        const_cast<size_t&>(that._size) = 0;
    }

    utf8_string::utf8_string(const char* cstr)
    : _size(0)
    , _data(nullptr, std::free)
    {
        if(cstr != nullptr)
        {
            utf8proc_uint8_t* data = nullptr;
            utf8proc_option_t options = static_cast<utf8proc_option_t>(UTF8PROC_NULLTERM | UTF8PROC_STABLE | UTF8PROC_COMPOSE | UTF8PROC_IGNORE);
            auto size = utf8proc_map(reinterpret_cast<const utf8proc_uint8_t*>(cstr), 0, &data, options);
            if(size < 0)
            {
                MX_THROW_MSG(utf8proc_errmsg(size));
            }

            const_cast<size_t&>(this->_size) = size;
            this->_data.reset(data);
        }
    }

    utf8_string::utf8_string(size_t size, utf8proc_uint8_t* data)
    : _size(size)
    , _data(data, std::free)
    { }

    // Assignment Operators

    utf8_string& utf8_string::operator=(const utf8_string& that)
    {
        const auto n = that._size + 1;
        if(this->_size != that._size)
        {
            const_cast<size_t&>(this->_size) = that._size;
            this->_data.reset(reinterpret_cast<utf8proc_uint8_t*>(std::calloc(n, sizeof(utf8proc_uint8_t))));
        }
        std::memcpy(this->_data.get(), that._data.get(), n);

        return *this;
    }

    utf8_string& utf8_string::operator=(utf8_string&& that)
    {
        const_cast<size_t&>(this->_size) = that._size;
        this->_data = std::move(that._data);
        const_cast<size_t&>(that._size) = 0;

        return *this;
    }

    utf8_string utf8_string::normalized() const
    {
        MX_ASSERT(this->_data.get() != nullptr);
        
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
}

std::ostream& operator<<(std::ostream& out, const tabletopify::utf8_string& str)
{
    return out << str.c_str();
}
