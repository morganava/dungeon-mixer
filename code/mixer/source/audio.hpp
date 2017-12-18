#pragma once

namespace tabletopify
{
	typedef size_t audio_id_t;
	typedef size_t atmosphere_id_t;

	constexpr audio_id_t INVALID_AUDIO_ID = ~audio_id_t(0);
	constexpr atmosphere_id_t INVALID_ATMOSPHERE_ID = ~atmosphere_id_t(0);

	typedef enum
	{
		ONE_SHOT,
		LOOP,
	} audio_type_t;

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

	    friend class audio_directory;
	};

    class one_shot_data : public audio_data
    {
    public:
        one_shot_data(std::vector<std::string>&& paths);
        ~one_shot_data() override;
    private:
        void load() override;
        void unload() override;

        std::vector<std::string> _paths;
        std::vector<sf::SoundBuffer> _buffers;

        friend class one_shot_track;
    };

    class loop_data : public audio_data
    {
    public:    
        loop_data(std::string&& loop_path);
        loop_data(std::string&& intro_path, std::string&& loop_path);
        ~loop_data() override;
    private:
        void load() override;
        void unload() override;

        std::optional<std::string> _intro_path;
        std::string _loop_path;

        std::optional<sf::SoundBuffer> _intro_buffer;
        sf::SoundBuffer _loop_buffer;

        std::optional<sf::SoundBuffer> _mono_intro_buffer;
        std::optional<sf::SoundBuffer> _mono_loop_buffer;

        friend class loop_track;
    };

	struct audio_entry
	{
		// track unique id
		std::string uuid;
		// display title
		std::string title;
		// query title
		std::string lower_title;
		// query tags
		std::vector<std::string> tags;
		// backing SoundBuffers
		std::shared_ptr<audio_data> data;
		// time since able to unload data in seconds
		float age = 0.0f;
	};

	class audio_directory
	{
	public:
		void update(float dt);
		audio_id_t get_audio_id_by_uuid(const std::string& uuid) const;
		std::string get_title_by_audio_id(audio_id_t id) const;
		std::vector<std::string> get_tags_by_audio_id(audio_id_t id) const;
		std::shared_ptr<audio_data> get_audio_data_by_audio_id(audio_id_t id) const;
		std::vector<audio_id_t> get_audio_ids() const;
		std::vector<audio_id_t> get_audio_ids_by_title(const std::string& title_query) const;
		std::vector<audio_id_t> get_audio_ids_by_tag(const std::vector<std::string>& queries) const;
		void add_entry(std::string&& uuid,std::string&& title, std::vector<std::string>&& tags, std::shared_ptr<audio_data>&& data);
	private:
		std::vector<audio_entry> _entries;
		// map a tag to set of track ids
		std::unordered_map<std::string, std::unordered_set<audio_id_t>> _tag_map;
		// map track uuid to track id
		std::unordered_map<std::string, audio_id_t> _uuid_map;

		// amount of time an unreferenced track waits before unloading data and freeing memory
		constexpr static float unload_age = 10.0f;
	};
	extern audio_directory* g_audio_directory;

    struct track_description
    {
    	audio_id_t audio_id;
    	audio_type_t audio_type;
    	float volume;
    	sf::Vector3f position;
    	union
    	{
    		struct
    		{
    			float min_repeat;
    			float max_repeat;
    		} one_shot;
    		struct
    		{

    		} loop;
    	};
    };

    struct atmosphere_entry
    {
    	// atmosphere unique id
    	std::string uuid;
    	// volume
    	float volume;
    	// query title
    	std::string title;
    	// query tags
    	std::vector<std::string> tags;
    	// track descriptions
    	std::vector<track_description> tracks;
    };

    class atmosphere_directory
    {
    public:
    	void update(float dt);

    	std::vector<atmosphere_id_t> get_atmosphere_ids() const;
    	std::vector<atmosphere_id_t> get_atmosphere_ids_by_title(const std::string& title_query) const;
    	std::vector<atmosphere_id_t> get_atmosphere_ids_by_tag(const std::vector<std::string>& queries) const;

    	atmosphere_entry& get_entry(atmosphere_id_t id);
    	atmosphere_id_t add_entry(atmosphere_entry&& entry);
    	void remove_entry(atmosphere_id_t id);
    private:
    	// counts up whenever atmospheres are added
    	atmosphere_id_t _next_id = 0;
    	// maps id to entry
    	std::unordered_map<atmosphere_id_t, atmosphere_entry> _entries;
    	// map a tag to set of atmosphere ids
    	std::unordered_multimap<std::string, audio_id_t> _tag_map;
    	// map a title to a set of atmosphere ids
    	std::unordered_multimap<std::string, audio_id_t> _title_map;
    };
    extern atmosphere_directory* g_atmosphere_directory;
}
