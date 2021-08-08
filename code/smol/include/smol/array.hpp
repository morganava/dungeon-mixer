#pragma once

namespace smol
{
    template<typename T>
    class array
    {
    public:
        array() = default;
        array(const array& that)
        : array(that.size())
        {
            std::copy(that.begin(), that.end(), _data.get());
        }
        array(array&& that)
        : _size(that._size)
        , _data(std::move(that._data))
        {
            that.~array();
            new(&that) array();
        }

        explicit array(size_t N)
        : _size(N)
        , _data(std::make_unique<T[]>(N))
        { }

        array(std::initializer_list<T> il)
        : array(il.size())
        {
            const auto N = this->_size;
            for(size_t i = 0; i < N; ++i)
            {
                this->_data[i] = std::move(il.begin()[i]);
            }
        }

        array(const std::vector<T>& vec)
        : array(vec.size())
        {
            std::copy(vec.begin(), vec.end(), _data.get());
        }

        array& operator=(std::initializer_list<T> il)
        {
            const auto N = il.size();
            if(N == this->_size)
            {
                for(size_t i = 0; i < N; ++i)
                {
                    this->_data[i] = std::move(il.begin()[i]);
                }
            }
            else
            {
                this->~array();
                new (this) array(il);
            }

            return *this;
        }

        array& operator=(const array& that)
        {
            this->~array();
            new (this) array(that);
            return *this;
        }

        array concat(std::initializer_list<T> il) const
        {
            array retval(this->size() + il.size());
            // copy over this array's data
            std::copy(this->begin(), this->end(), retval._data.get());
            // copy over the new data
            std::copy(il.begin(), il.end(), retval._data.get() + this->size());

            return retval;
        }

        bool operator==(const array& that) const
        {
            if (this == &that) return true;
            const auto N = this->_size;
            if (N != that._size) return false;

            for(size_t i = 0; i < N; ++i)
            {
                if (this->_data[i] != that._data[i]) return false;
            }

            return true;
        }

        T& operator[](size_t idx)
        {
            MX_THROW_IF_FALSE(idx < _size);
            return _data[idx];
        }

        const T& operator[](size_t idx) const
        {
            MX_THROW_IF_FALSE(idx < _size);
            return _data[idx];
        }

        T* begin()
        {
            return _data.get();
        }

        T* end()
        {
            return this->begin() + _size;
        }

        T* const begin() const
        {
            return _data.get();
        }

        T* const end() const
        {
            return this->begin() + _size;
        }

        constexpr size_t size() const
        {
            return _size;
        }

        const T* data() const
        {
            return _data.get();
        }

        T* data()
        {
            return _data.get();
        }

    private:
        const size_t _size = 0;
        std::unique_ptr<T[]> _data;
    };
}