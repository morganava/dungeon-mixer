#pragma once

namespace tabletopify
{
    struct text_buffer
    {
    public:
        text_buffer() : text_buffer(2) {}

        text_buffer(size_t capacity)
        : _capacity(capacity)
        , _buffer(new char[_capacity])
        {
            for(size_t idx = 0; idx < _capacity; ++idx)
                _buffer[idx] = 0;
        }

        size_t capacity() const { return _capacity; }
        char* buffer() { return _buffer.get(); }

        void update();
        text_buffer& operator=(const std::string& str);
        operator std::string() const;

    private:
        size_t _capacity;
        std::unique_ptr<char[]> _buffer;
    };
}