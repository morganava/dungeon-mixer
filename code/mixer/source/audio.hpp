#pragma once

#include "tag_map.hpp"
#include "uuid.hpp"

namespace tabletopify
{
    // maps to audio data
    typedef handle_t audio_handle_t;
    // instantiated possibly playing audio tracks
    typedef handle_t track_handle_t;
    // collection of tracks
    typedef handle_t atmosphere_handle_t;

    constexpr handle_t MX_INVALID_HANDLE = ~handle_t(0);

    typedef enum
    {
        ONE_SHOT,
        LOOP,
    } audio_type_t;

    typedef enum
    {
        UNLOADED,
        LOADING,
        LOADED,
        UNLOADING,
        PENDING_UNLOADED,
    } audio_data_state_t;

    class audio_data
    {
    public:
        virtual ~audio_data() {}

        virtual audio_type_t get_type() = 0;
        virtual sf::SoundBuffer& get_sound_buffer(bool positionable) = 0;
        static void load(std::shared_ptr<audio_data> audio);
        static void unload(std::shared_ptr<audio_data> audio);
        audio_data_state_t state() const {return _state;}
    protected:
        // loading data from disk
        virtual void load_impl() = 0;
        // freeing audio data
        virtual void unload_impl() = 0;
        volatile audio_data_state_t _state = UNLOADED;

        friend class audio_directory;
    };

    class one_shot_data : public audio_data
    {
    public:
        one_shot_data(std::vector<fs::path>&& paths);
        ~one_shot_data() override;

        audio_type_t get_type() override {return ONE_SHOT;}
        sf::SoundBuffer& get_sound_buffer(bool positionable) override;
    private:
        void load_impl() override;
        void unload_impl() override;

        std::vector<fs::path> _paths;
        std::vector<sf::SoundBuffer> _buffers;
    };

    class loop_data : public audio_data
    {
    public:
        loop_data(fs::path&& loop_path);
        ~loop_data() override;

        audio_type_t get_type() override {return LOOP;}
        sf::SoundBuffer& get_sound_buffer(bool positionable) override;
    private:
        void load_impl() override;
        void unload_impl() override;

        fs::path _loop_path;

        sf::SoundBuffer _loop_buffer;
        sf::SoundBuffer _mono_loop_buffer;
    };

    struct audio_entry
    {
        // track unique id
        uuid id;
        // display title
        std::string title;
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
        audio_handle_t get_audio_handle_by_id(const uuid& id) const;
        std::vector<audio_handle_t> get_audio_handles() const;
        std::vector<audio_handle_t> get_audio_handles_by_query(const std::string& query) const;

        audio_entry& get_entry(audio_handle_t handle);
        // audio_handle_t add_entry(audio_entry&& entry);
        void add_entry(uuid&& id, std::string&& title, std::vector<std::string>&& tags, std::shared_ptr<audio_data>&& data);

    private:
        std::vector<audio_entry> _entries;
        tag_map _tag_map;
        // map track id to track handle
        std::unordered_map<uuid, audio_handle_t> _id_map;

        // amount of time an unreferenced track waits before unloading data and freeing memory
        constexpr static float unload_age = 10.0f;
    };

    // description for how audio is to played in an atmosphere
    struct track_entry
    {
        std::string audioTitle;
        uuid audioId;
        std::shared_ptr<audio_data> audioData;
        audio_type_t audioType;
        float volume;
        float atmosphereVolume = 1.0f;
        bool muted;
        bool atmosphereMuted = false;
        struct
        {
            float min;
            float max;
        } repeat;
        struct
        {
            bool enabled;
            union
            {
                struct
                {
                    float x;
                    float y;
                    float z;
                };
                float raw[3];
            };
        } position;
    };

    class track_directory
    {
    public:
        track_entry& get_entry(track_handle_t handle);
        track_handle_t add_entry(track_entry&& entry);
        void remove_entry(track_handle_t handle);
    private:
        static track_handle_t _next_handle;
        std::unordered_map<track_handle_t, track_entry> _entries;
    };

    struct atmosphere_entry
    {
        // unique id
        uuid id;
        // title
        std::string title;
        // tracks
        std::vector<track_handle_t> tracks;
    };

    class atmosphere_directory
    {
    public:
        std::vector<atmosphere_handle_t> get_atmosphere_handles() const;
        std::vector<atmosphere_handle_t> get_atmosphere_handles_by_query(const std::string& query) const;

        atmosphere_handle_t get_atmosphere_handle_by_id(const uuid& id) const;
        atmosphere_entry& get_entry(atmosphere_handle_t handle);
        atmosphere_handle_t add_entry(atmosphere_entry&& entry);
        void remove_entry(atmosphere_handle_t handle);
        void rename_entry(atmosphere_handle_t handle, const std::string& newName);
    private:
        static atmosphere_handle_t _next_handle;
        std::map<atmosphere_handle_t, atmosphere_entry> _entries;
        tag_map _tag_map;
        // map track id to atmosphere handle
        std::unordered_map<uuid, atmosphere_handle_t> _id_map;
    };

    struct playback_entry
    {
        // properties of playing track
        float volume;
        bool seamlessLoop;
        float repeatMin;
        float repeatMax;
        bool positional;
        sf::Vector3f position;
        bool dirtyProperties; // when set volume, position, etc need to be synced to the sf::Sound
        bool dirtyBuffer; // when the sound buffer needs to be re-fetched

        float repeatDelay;
        float timeSincePlaybackEnd;
        std::shared_ptr<audio_data> audioData;
        sf::Sound sound;
    };

    class playback_manager
    {
    public:
        void update(float dt);

        /// Atmosphere methods
        void play_atmosphere(atmosphere_handle_t atmosphereHandle);
        void stop_atmosphere(atmosphere_handle_t atmosphereHandle);

        /// Track methods
        void play_track(track_handle_t trackHandle);
        void stop_track(track_handle_t trackHandle);
        void sync_track(track_handle_t trackHandle);
        void stop_all_tracks();
    private:
        std::unordered_map<track_handle_t, playback_entry> _entries;
    };

    /// getters

    audio_directory* get_audio_directory();
    track_directory* get_track_directory();
    atmosphere_directory* get_atmosphere_directory();
    playback_manager* get_playback_manager();

    void init_audio_directory();
    void init_track_directory(track_directory&&);
    void init_atmosphere_directory(atmosphere_directory&&);
    void init_playback_manager();

    void free_audio_directory();
    void free_track_directory();
    void free_atmosphere_directory();
    void free_playback_manager();
}
