#pragma once

namespace tabletopify
{
    typedef size_t handle_t;

    class tag_map
    {
    public:
        std::vector<handle_t> get_keys_by_string_query(const std::string& query_string) const;
        void remove_key(handle_t key);
        void add_key(handle_t key, const std::vector<std::string>& tags);
    private:
        std::vector<handle_t> get_keys_by_tag_query(const std::vector<std::string>& query_tags) const;

        std::unordered_multimap<std::string, handle_t> _tag_to_keys;
        std::unordered_multimap<handle_t, std::string> _key_to_tags;
        std::unordered_set<handle_t> _keys;
    };
}