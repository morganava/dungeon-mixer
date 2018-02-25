#pragma once

namespace tabletopify
{
    template<typename KEY, typename VALUE>
    class tag_map
    {
    public:
        std::vector<KEY> get_keys() const
        {
            std::vector<KEY> result;
            for(const auto& it : _entries)
            {
                result.push_back(it.first);
            }
            return result;
        }

        std::vector<KEY> get_keys_by_tag_query(const std::string& query_string) const
        {
            std::vector<KEY> result;

            auto queries = split(query_string, " ");
            for(auto& str : queries)
            {
                str = to_lower(str);
            }

            for(const auto& it : _entries)
            {
                auto currentKey = it.first;
                auto range = _tag_map.equal_range(currentKey);

                // all query strings must be contained in the range
                bool totalMatch = true;
                for(auto currentQuery : queries)
                {
                    bool match = false;
                    for(auto it = range.first; it != range.second; ++it)
                    {
                        auto currentTag = it->second;
                        if(contains(currentTag, currentQuery))
                        {
                            match = true;
                            break;
                        }
                    }

                    if(match == false)
                    {
                        totalMatch = false;
                        break;
                    }
                }

                if(totalMatch)
                {
                    result.push_back(currentKey);
                }
            }

            return result;
        }

        VALUE& get_entry(KEY key)
        {
            auto it = _entries.find(key);
            assert(it != _entries.end());

            return it->second;
        }

        void remove_entry(KEY key)
        {
            _entries.erase(key);
            _tag_map.erase(key);
        }

        KEY add_entry(VALUE&& value, std::vector<std::string>&& tags)
        {
            const auto key = ++_next_key;
            _entries.insert({key, value});

            for(auto tag : tags)
            {
                _tag_map.insert({key, std::move(tag)});
            }

            return key;
        }

    private:
        KEY _next_key = 0;
        std::unordered_map<KEY, VALUE> _entries;
        std::unordered_multimap<KEY, std::string> _tag_map;
    };
}