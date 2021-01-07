#pragma once

#include "utf8_string.hpp"

namespace dm
{
    // the implementation assumes passed in tags and queries are normalized utf8 strings
    class tag_map_impl
    {
    public:
        std::unordered_set<size_t> get_keys_by_tag_query(const std::vector<utf8_string>& query_tags) const;
        void remove_key(size_t key);
        void add_key(size_t key, const std::vector<utf8_string>& tags);
    private:
        std::unordered_multimap<utf8_string, size_t> _tag_to_keys;
        std::unordered_multimap<size_t, utf8_string> _key_to_tags;
        std::unordered_set<size_t> _keys;
    };

    template<typename HANDLE>
    class tag_map
    {
        static_assert(std::is_same<size_t,typename std::underlying_type<HANDLE>::type>::value);
    public:
        std::vector<HANDLE> get_keys_by_tag_query(const utf8_string& query) const
        {
            auto query_tags = query.normalized().split();
            auto keys = _tag_map.get_keys_by_tag_query(query_tags);

            std::vector<HANDLE> retval;
            retval.reserve(keys.size());
            for(auto currentKey : keys) {
                retval.push_back(static_cast<HANDLE>(currentKey));
            }
            return retval;
        }
        void remove_key(HANDLE key)
        {
            _tag_map.remove_key(static_cast<size_t>(key));
        }
        void add_key(HANDLE key, const std::vector<utf8_string>& tags)
        {
            std::vector<utf8_string> normalized_tags;
            normalized_tags.reserve(tags.size());

            for(const auto& tag : tags)
            {
                normalized_tags.push_back(tag.normalized());
            }

            _tag_map.add_key(static_cast<size_t>(key), normalized_tags);
        }
    private:
        tag_map_impl _tag_map;

    };
}
