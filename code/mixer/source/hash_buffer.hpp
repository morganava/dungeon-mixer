namespace tabletopify
{
    class utf8_string;
    class utf8_string_view;

    struct hash_buffer
    {
        template<typename TYPE>
        void push_back(const TYPE& val)
        {
            push_back(&val, sizeof(val));
        }

        template<typename TYPE>
        bool next(TYPE& outval)
        {
            if (_read_head + sizeof(TYPE) <= _raw.data() + _raw.size())
            {
                std::memcpy(&outval, _read_head, sizeof(TYPE));
                _read_head += sizeof(TYPE);
                return true;
            }
            return false;
        }

        void clear()
        {
            _raw.clear();
            _hash = 0;
            _read_head = _raw.data();
        }

        bool operator==(const hash_buffer& that) const
        {
            if (_hash != that._hash) return false;
            if (_raw.size() != that._raw.size()) return false;

            return std::memcmp(_raw.data(), that._raw.data(), _raw.size()) == 0;
        }

        size_t size() const
        {
            return _raw.size();
        }

    private:
        void push_back(const void* pVal, size_t valSize);

        std::vector<uint8_t> _raw;
        size_t _hash = 0;

        uint8_t* _read_head = _raw.data();
    };

    template<>
    void hash_buffer::push_back<utf8_string>(const utf8_string& str);
    template<>
    bool hash_buffer::next<utf8_string_view>(utf8_string_view& sv);
}
