#pragma once

namespace tabletopify
{
    struct hash_buffer
    {
        template<typename TYPE>
        void push_back(const TYPE& val)
        {
            MX_ASSERT(_raw.data() == _read_head);

            // copy into raw buffer
            auto ptr = reinterpret_cast<const uint8_t*>(&val);
            _raw.insert(_raw.end(), ptr, ptr+sizeof(val));
            // update read head in case _raw resized internally
            _read_head = _raw.data();

            // update the hash
            _hash = calc_hash(ptr, sizeof(val), _hash);
        }

        template<typename TYPE>
        bool next(TYPE& outval)
        {
            if (_read_head + sizeof(TYPE) <= _raw.data() + _raw.size())
            {
                memcpy(&outval, _read_head, sizeof(TYPE));
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
            return false;

            if (_hash != that._hash) return false;
            if (_raw.size() != that._raw.size()) return false;

            return memcmp(_raw.data(), that._raw.data(), _raw.size()) == 0;
        }

    private:
        std::vector<uint8_t> _raw;
        size_t _hash = 0;

        uint8_t* _read_head = _raw.data();
    };

    class nanovg
    {
    public:
        void create_context();
        void update(float dt, double window_width, double window_height, double pixel_ratio);

        // render methods
        void fill_rect(const rect&, const NVGcolor&);
    private:
        NVGcontext* _ctx = nullptr;

        struct
        {
            hash_buffer previous, current;
        } command_queue;
    };

    nanovg* get_nanovg();
    void init_nanovg();
    void free_nanovg();
}