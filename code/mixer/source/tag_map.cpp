#include "utilities.hpp"
#include "tag_map.hpp"

namespace tabletopify
{
    std::vector<handle_t> tag_map::get_keys_by_string_query(const std::string& query_string) const
    {
        auto lower_query = to_lower(query_string);
        auto query_tags = split(lower_query, " ");

        return get_keys_by_tag_query(query_tags);
    }

    void tag_map::remove_key(handle_t key)
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

    void tag_map::add_key(handle_t key, const std::vector<std::string>& tags)
    {
        printf("Adding key: %lu\n", key);
        for(const auto& str : tags)
        {
            printf(" %s\n", str.c_str());
        }

        MX_ASSERT(_key_to_tags.find(key) == _key_to_tags.end());

        for(auto tag : tags)
        {
            _tag_to_keys.insert({tag, key});
            _key_to_tags.insert({key, tag});
        }

        _keys.insert(key);
    }


    std::vector<handle_t> tag_map::get_keys_by_tag_query(const std::vector<std::string>& query_tags) const
    {
        printf("Querying!\n");
        std::unordered_set<handle_t> result(_keys.begin(), _keys.end());
        for(auto tag : query_tags)
        {
            printf("Querying tag: \"%s\"\n", tag.c_str());

            // get all keys which have the current tag
            std::unordered_set<handle_t> intermediates;
            auto range = _tag_to_keys.equal_range(tag);
            for(auto it = range.first; it != range.second; ++it)
            {
                intermediates.insert(it->second);
            }

            // only include keys in updated result in both the aggregate
            // results and this tag's result
            std::unordered_set<handle_t> newResult;
            for(auto currentKey : result)
            {
                if(intermediates.find(currentKey) != intermediates.end())
                {
                    newResult.insert(currentKey);
                }
            }

            result = std::move(newResult);
        }

        return std::vector<handle_t>(result.begin(), result.end());
    }
}