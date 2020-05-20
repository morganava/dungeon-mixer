#pragma once

#include "utf8_string.hpp"

namespace tabletopify
{
    // todo: single size_t implementation backend
    template<typename HANDLE>
    class tag_map
    {
    public:
        std::vector<HANDLE> get_keys_by_tag_query(const std::vector<utf8_string>& query_tags) const
        {
            // initially fill with all our keys in case the query list is empty
            std::unordered_set<HANDLE> result(_keys.begin(), _keys.end());
            for(auto tag : query_tags)
            {
                // get all keys which have the current tag
                std::unordered_set<HANDLE> intermediates;
                auto range = _tag_to_keys.equal_range(tag);
                for(auto it = range.first; it != range.second; ++it)
                {
                    intermediates.insert(it->second);
                }

                // only include keys in updated result in both the aggregate
                // results and this tag's result
                std::unordered_set<HANDLE> newResult;
                for(auto currentKey : result)
                {
                    if(intermediates.find(currentKey) != intermediates.end())
                    {
                        newResult.insert(currentKey);
                    }
                }

                result = std::move(newResult);
            }

            return std::vector<HANDLE>(result.begin(), result.end());
        }
        void remove_key(HANDLE key)
        {
            // iterate over all tags for this key
            auto tag_range = _key_to_tags.equal_range(key);
            for(auto tag_it= tag_range.first; tag_it != tag_range.second;)
            {
                auto& tag = tag_it->second;

                // erase the tag/key pair
                auto key_range = _tag_to_keys.equal_range(tag);
                for(auto key_it = key_range.first; key_it != key_range.second; ++key_it)
                {
                    if(key_it->second == key)
                    {
                        _tag_to_keys.erase(key_it);
                        break;
                    }
                }

                // erase the key/tag pair
                tag_it = _key_to_tags.erase(tag_it);
            }

            _keys.erase(key);
        }
        void add_key(HANDLE key, const std::vector<utf8_string>& tags)
        {
            MX_ASSERT(_key_to_tags.find(key) == _key_to_tags.end());

            for(auto tag : tags)
            {
                _tag_to_keys.insert({tag, key});
                _key_to_tags.insert({key, tag});
            }

            _keys.insert(key);
        }
    private:
        std::unordered_multimap<utf8_string, HANDLE> _tag_to_keys;
        std::unordered_multimap<HANDLE, utf8_string> _key_to_tags;
        std::unordered_set<HANDLE> _keys;
    };
}