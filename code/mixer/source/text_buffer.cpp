#include "text_buffer.hpp"

namespace tabletopify
{
    void text_buffer::update()
    {
        // last char will always be 0, second to last will be something
        // other than 0 if it has grown
        if(_buffer[_capacity - 2] != 0)
        {
            auto newCapacity = _capacity * 2;

            std::unique_ptr<char[]> newBuffer(new char[newCapacity]);

            size_t idx;
            for(idx = 0; idx < _capacity; ++idx)
                newBuffer[idx] = _buffer[idx];

            for(; idx < newCapacity; ++idx)
                newBuffer[idx] = 0;

            _capacity = newCapacity;
            _buffer = std::move(newBuffer);
        }
    }

    text_buffer& text_buffer::operator=(const std::string& str)
    {
        if(str.size() > _capacity)
        {
            _capacity = str.size() * 2;
            std::unique_ptr<char[]> newBuffer(new char[_capacity]);
            _buffer = std::move(newBuffer);
        }

        for(size_t idx = 0; idx <= str.size(); ++idx)
            _buffer[idx] = str[idx];

        return *this;
    }

    text_buffer::operator std::string() const
    {
        return std::string(_buffer.get());
    }
}
