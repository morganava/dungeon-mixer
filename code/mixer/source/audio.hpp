#pragma once

class audio_data
{
public:
    virtual ~audio_data() {}

protected:
    // loading data from disk
    virtual void load() = 0;
    // freeing audio data
    virtual void unload() = 0;
    bool _loaded = false;

    friend class track_directory;
};


class track
{
public:
    virtual ~track() = 0;

    // standard track getters
    const std::string& title() const {return _title;}
    const std::vector<std::string>& tags() const {return _tags;}
    float& volume() {return _volume;}
    sf::Vector3f position() {return _position;};

    virtual void update() = 0;
private:

    std::shared_ptr<audio_data> _data;

    std::string _title;
    std::vector<std::string> _tags;
    float _volume;
    sf::Vector3f _position;
};

struct track_entry
{
	// sha1sum of 
	std::string uuid;
	std::string title;
	std::string lower_title;
	std::vector<std::string> tags;
	std::shared_ptr<audio_data> data;
	float age = 0.0f;
};

class track_directory
{
public:
	track_directory();
	void update(float dt);
	std::string get_title_by_index(size_t idx) const;
	std::vector<std::string> get_tags_by_index(size_t idx) const;
	std::unique_ptr<track> get_track_by_index(size_t idx) const;
	std::vector<size_t> get_tracks_by_title(const std::string& title_query) const;
	std::vector<size_t> get_tracks_by_tag(const std::vector<std::string>& queries) const;
private:
	void add_entry(std::string&& uuid,std::string&& title, std::vector<std::string>&& tags, std::shared_ptr<audio_data>&& data);

	std::vector<track_entry> _entries;
	// map a tag to set of track ids
	std::unordered_map<std::string, std::unordered_set<size_t>> _tag_map;
	// map track uuid to track id
	std::unordered_map<std::string, size_t> _uuid_map;

	// amount of time an unreferenced track waits before unloading data and freeing memory
	constexpr static float unload_age = 10.0f;
};

#if 0

Notes motherfucker
	
	create a 'track directory' which contains title, tags and index into the audio data manager
	class track is a 'playing' track
		contains a shared_ptr to backing audio_data
		specialized versions handle frame by frame playing, has an update method
	tracks keyed off of a guid

#endif