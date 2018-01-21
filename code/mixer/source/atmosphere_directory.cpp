 #include "precomp.hpp"

// tabletopify
#include "utilities.hpp"
#include "audio.hpp"

namespace tabletopify
{
    single_threaded<atmosphere_directory> g_atmosphere_directory;

    /// Atmosphere Directory

    std::vector<atmosphere_id_t> atmosphere_directory::get_atmosphere_ids() const
    {
        std::vector<atmosphere_id_t> ids;
        for(const auto& it : _entries)
        {
            ids.push_back(it.first);
        }
        return ids;
    }

    std::vector<atmosphere_id_t> atmosphere_directory::get_atmosphere_ids_by_title(const std::string& title_query) const
    {
        std::vector<atmosphere_id_t> result;
        
        // first search for exact match
        for(const auto& it : _entries)
        {
            if(it.second.title == title_query)
            {
                auto id = it.first;
                result.push_back(id);
            }
        }
        // used to ensure we don't have duplicates
        std::unordered_set<atmosphere_id_t> resultSet(result.begin(), result.end());

        // next look for subset match
        for(const auto& it : _entries)
        {
            // first ensure we haven't added this result yet
            if(resultSet.find(it.first) == resultSet.end())
            {
                if(contains(to_lower(it.second.title), title_query))
                {
                    auto id = it.first;
                    result.push_back(id);
                    resultSet.insert(id);
                }
            }
        }

        // finally look for 'tag' match
        auto tags_query = split(to_lower(title_query), " ");
        for(const auto& it : _entries)
        {
            // first ensure we haven't added this result yet
            if(resultSet.find(it.first) == resultSet.end())
            {
                bool match = true;
                for(const auto& tag : tags_query)
                {
                    if(contains(to_lower(it.second.title), tag) == false)
                    {
                        match = false;
                        break;
                    }
                }

                if(match)
                {
                    auto id = it.first;
                    result.push_back(id);
                    // don't need to insert the current id into the resultSet, since
                    // we're guaranteed not to encounter it again
                }
            }
        }            

        return result;
    }

    atmosphere_entry& atmosphere_directory::get_entry(atmosphere_id_t id)
    {
        auto it = _entries.find(id);
        assert(it != _entries.end());

        return it->second;
    }

    atmosphere_id_t atmosphere_directory::add_entry(atmosphere_entry&& entry)
    {
        // update source and get id
        auto id = ++_next_id;

        // insert into map and increment the id source
        _entries.emplace(id, entry);

        // return id of entry just inserted
        return id;
    }

    void atmosphere_directory::remove_entry(atmosphere_id_t id)
    {
        // get reference to entry we're removing
        auto it = _entries.find(id);
        assert(it != _entries.end());

        // remove the entry 
        _entries.erase(it);
    }
}