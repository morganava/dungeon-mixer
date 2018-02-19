#pragma once

namespace tabletopify
{
    // maps to audio data
	typedef size_t audio_id_t;
    // instantiated possibly playing audio tracks
    typedef size_t track_id_t;
    // collection of tracks
	typedef size_t atmosphere_id_t;

	constexpr audio_id_t INVALID_AUDIO_ID = ~audio_id_t(0);
    constexpr track_id_t INVALID_TRACK_ID = ~track_id_t(0);
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
        
        virtual audio_type_t get_type() = 0;
        virtual sf::SoundBuffer& get_sound_buffer(bool mono) = 0;
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

        audio_type_t get_type() override {return ONE_SHOT;}
        sf::SoundBuffer& get_sound_buffer(bool mono) override;
    private:
        void load() override;
        void unload() override;

        std::vector<std::string> _paths;
        std::vector<sf::SoundBuffer> _buffers;
    };

    class loop_data : public audio_data
    {
    public:    
        loop_data(std::string&& loop_path);
        ~loop_data() override;

        audio_type_t get_type() override {return LOOP;}
        sf::SoundBuffer& get_sound_buffer(bool mono) override;
    private:
        void load() override;
        void unload() override;

        std::string _loop_path;

        sf::SoundBuffer _loop_buffer;
        sf::SoundBuffer _mono_loop_buffer;
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

        audio_entry& get_entry(audio_id_t id);
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
	extern single_threaded<audio_directory> g_audio_directory;

    // description for how audio is to played in an atmosphere
    struct track_entry
    {
        std::string title;
        audio_id_t audioId;
        audio_type_t audioType;
        float volume;
        float atmosphereVolume = 1.0f;
        bool muted;
        bool atmosphereMuted = false;
        union
        {
            struct
            {

                float repeatMin;
                float repeatMax;
                union
                {
                    struct
                    {
                        float x;
                        float y;
                        float z;
                    };
                    float position[3];
                };
            } oneShot;
            struct
            {

            } loop;
        };
    };

    class track_directory
    {
    public:
        track_entry& get_entry(track_id_t id);
        track_id_t add_entry(track_entry&& entry);
        void remove_entry(track_id_t id);
    private:
        track_id_t _next_id = 0;
        std::unordered_map<track_id_t, track_entry> _entries;
    };
    extern single_threaded<track_directory> g_track_directory;

    struct atmosphere_entry
    {
    	// atmosphere unique id
    	std::string uuid;
    	// volume
    	float volume;
    	// query title
    	std::string title;
    	// tracks
    	std::vector<track_id_t> tracks;
    };

    class atmosphere_directory
    {
    public:
    	void update(float dt);

    	std::vector<atmosphere_id_t> get_atmosphere_ids() const;
    	std::vector<atmosphere_id_t> get_atmosphere_ids_by_title(const std::string& title_query) const;

    	atmosphere_entry& get_entry(atmosphere_id_t id);
    	atmosphere_id_t add_entry(atmosphere_entry&& entry);
    	void remove_entry(atmosphere_id_t id);
    private:
    	// counts up whenever atmospheres are added
    	atmosphere_id_t _next_id = 0;
    	// maps id to entry
    	std::unordered_map<atmosphere_id_t, atmosphere_entry> _entries;
    	// map a title to a set of atmosphere ids
    	std::unordered_multimap<std::string, audio_id_t> _title_map;
    };
    extern single_threaded<atmosphere_directory> g_atmosphere_directory;

    struct playback_entry
    {
        // properties of playing track
        float volume;
        bool seamlessLoop;
        float repeatMin;
        float repeatMax;
        bool positional;
        sf::Vector3f position;
        bool dirty; // when set volume, position, etc need to be synced to the sf::Sound

        float repeatDelay;
        float timeSincePlaybackEnd;        
        std::shared_ptr<audio_data> audioData;        
        sf::Sound sound;
    };

    class playback_manager
    {
    public:
        void update(float dt);

        /// old methods
        // begins playback of atmosphere
        void play(atmosphere_id_t atmosphereId);
        // stops playback of atmosphere
        void stop(atmosphere_id_t atmosphereId);
        // syncs track info from track_directory to internal data
        void sync(track_id_t);

        /// atmosphere methods
        void playAtmosphere(atmosphere_id_t atmosphereId);
        void stopAtmosphere(atmosphere_id_t atmosphereId);

        /// track methods
        void playTrack(track_id_t trackId);
        void stopTrack(track_id_t trackId);
        void syncTrack(track_id_t trackId);
        void stopAllTracks();
    private:
        std::unordered_map<track_id_t, playback_entry> _entries;
    };
    extern single_threaded<playback_manager> g_playback_manager;
}
