#pragma once

namespace smol
{
    // tag map implementation using a trie data structure to form our index
    class tag_map_impl
    {
    public:
        std::unordered_set<size_t>  get_keys_by_tag_query(const std::vector<utf8_string>& query_tags) const;
        void remove_key(size_t key);
        void add_key(size_t key, const std::vector<utf8_string>& tags);
        void add_key(size_t key, const utf8_string& tag);
    private:
        struct node
        {
            utf8_codepoint codepoint;
            std::unordered_set<size_t> handles;
            std::unordered_map<utf8_codepoint,std::unique_ptr<node>> children;
        };
        node root;

        std::unordered_set<size_t> const* get_keys_impl(const utf8_string& query) const;
        void add_key_impl(size_t key, utf8_string_iterator begin, utf8_string_iterator end);
        void remove_key_impl(node& n, size_t key);
    };

    template<typename HANDLE>
    class tag_map
    {
        static_assert(std::is_same<size_t,typename std::underlying_type<HANDLE>::type>::value);
    public:
        std::vector<HANDLE> get_keys_by_tag_query(const utf8_string& query) const
        {
            auto keys = _tag_map.get_keys_by_tag_query(query.split());

            std::vector<HANDLE> retval;
            retval.reserve(keys.size());
            std::transform(keys.begin(), keys.end(), std::back_inserter(retval),
                [](size_t k) -> HANDLE {return static_cast<HANDLE>(k);});

            return retval;
        }
        void remove_key(HANDLE key)
        {
            _tag_map.remove_key(static_cast<size_t>(key));
        }
        void add_key(HANDLE key, const std::vector<utf8_string>& tags)
        {
            _tag_map.add_key(static_cast<size_t>(key), tags);
        }
    private:
        tag_map_impl _tag_map;
    };
}
