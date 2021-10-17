// #include "utilities.hpp"
#include <smol/error.hpp>
#include <smol/utf8_string.hpp>
#include <smol/tag_map.hpp>

namespace smol
{
    //
    // Tag Map Impl
    //

    std::unordered_set<size_t>  tag_map_impl::get_keys_by_tag_query(const std::vector<utf8_string>& query_tags) const
    {
        if (query_tags.size() == 0)
        {
            // return all handles
            return root.handles;
        }

        auto it = query_tags.begin();

        // get our initial set of results
        std::unordered_set<size_t> retval;
        if (auto keys = this->get_keys_impl(it->normalized()); keys != nullptr)
        {
            retval = *keys;
        }

        auto intersect = [](std::unordered_set<size_t> const* small, std::unordered_set<size_t> const* big)
        {
            std::unordered_set<size_t> retval;

            // ensure small is smaller
            if (small->size() > big->size())
            {
                std::swap(small, big);
            }

            for(auto handle : *small)
            {
                if (big->contains(handle))
                {
                    retval.insert(handle);
                }
            }

            return retval;
        };

        for(++it; !retval.empty() && it != query_tags.end(); ++it)
        {
            if (auto keys = this->get_keys_impl(it->normalized()); keys != nullptr)
            {
                retval = std::move(intersect(&retval, keys));
            }
            else
            {
                retval.clear();
            }
        }
        return retval;
    }

    std::unordered_set<size_t> const* tag_map_impl::get_keys_impl(const utf8_string& query) const
    {
        // walk down the tree until we get to the node representing thre query, or we are forced to stop short
        // if we find the node, return a pointer to its handles
        // otherwise return nullopt
        auto* parent = &this->root;
        for(auto cp : query)
        {
            auto it = parent->children.find(cp);
            if (it == parent->children.end())
            {
                return nullptr;
            }
            parent = it->second.get();
        }

        return &parent->handles;
    }

    void tag_map_impl::remove_key(size_t key)
    {
        this->remove_key_impl(this->root, key);
    }

    void tag_map_impl::remove_key_impl(tag_map_impl::node& n, size_t key)
    {
        if (n.handles.erase(key) == 1)
        {
            for(auto it = n.children.begin(); it != n.children.end();)
            {
                auto& child = it->second;
                remove_key_impl(*child, key);

                if(child->handles.empty())
                {
                    it = n.children.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    void tag_map_impl::add_key(size_t key, const std::vector<utf8_string>& tags)
    {
        for(const auto& tag : tags)
        {
            this->add_key(key, tag);
        }
    }

    void tag_map_impl::add_key(size_t key, const utf8_string& tag)
    {
        // add each substring to the trie so we can find substrings
        auto ntag = tag.normalized();
        for(auto it = ntag.begin(); it != ntag.end(); ++it)
        {
            this->add_key_impl(key, it, ntag.end());
        }
    }

    void tag_map_impl::add_key_impl(size_t key, utf8_string_iterator begin, utf8_string_iterator end)
    {
        auto find_or_create = [](node& n, utf8_codepoint cp) -> node&
        {
            auto it = n.children.find(cp);

            // codepoint not found, create new node, insert into our list and return it
            if (it == n.children.end())
            {
                auto [it, ok] = n.children.insert({cp, std::make_unique<node>(cp)});
                if (ok)
                {
                    auto& node = *it->second;
                    return node;
                }
            }
            // ensure the codepoint is set correctly on existing node
            auto& node = *it->second;
            SMOL_ASSERT(node.codepoint == cp);
            return node;
        };

        // repeatedly go down our tree and add the key for this tag
        auto* parent = &this->root;
        parent->handles.insert(key);
        for(auto it = begin; it != end; ++it)
        {
            auto& node = find_or_create(*parent, *it);
            node.handles.insert(key);

            parent = &node;
        }
    }
}