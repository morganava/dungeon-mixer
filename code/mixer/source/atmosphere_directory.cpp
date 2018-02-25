 #include "precomp.hpp"

// tabletopify
#include "utilities.hpp"
#include "tag_map.hpp"
#include "audio.hpp"

namespace tabletopify
{
    single_threaded<atmosphere_directory> g_atmosphere_directory;

    /// Atmosphere Directory

    std::vector<atmosphere_id_t> atmosphere_directory::get_atmosphere_ids() const
    {
        return _tag_map.get_keys();
    }

    std::vector<atmosphere_id_t> atmosphere_directory::get_atmosphere_ids_by_title(const std::string& title_query) const
    {
        return _tag_map.get_keys_by_tag_query(title_query);
    }

    atmosphere_entry& atmosphere_directory::get_entry(atmosphere_id_t id)
    {
        return _tag_map.get_entry(id);
    }

    atmosphere_id_t atmosphere_directory::add_entry(atmosphere_entry&& entry)
    {
        std::vector<std::string> tags = split(entry.title, " ");
        for(auto& currentTag : tags)
        {
            currentTag = to_lower(currentTag);
        }

        return _tag_map.add_entry(std::move(entry), std::move(tags));
    }

    void atmosphere_directory::remove_entry(atmosphere_id_t id)
    {
        _tag_map.remove_entry(id);
    }
}