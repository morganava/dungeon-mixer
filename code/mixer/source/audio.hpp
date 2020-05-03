#pragma once

#include "tag_map.hpp"
#include "uuid.hpp"
#include "singleton.hpp"

namespace tabletopify
{
    constexpr handle_t MX_INVALID_HANDLE = ~handle_t(0);

    // maps to audio data
    typedef handle_t audio_handle_t;
    // instantiated possibly playing audio tracks
    typedef handle_t track_handle_t;
    // collection of tracks
    typedef handle_t atmosphere_handle_t;
    // playlists
    enum class playlist_handle : handle_t
    {
        invalid = MX_INVALID_HANDLE,
        all = 0,
        favorites,
        playing,
        first_custom,
    };
    typedef playlist_handle playlist_handle_t;

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
        utf8_string title;
        // query tags
        std::vector<utf8_string> tags;
        // backing SoundBuffers
        std::shared_ptr<audio_data> data;
        // time since able to unload data in seconds
        float age = 0.0f;
        audio_type_t type() const
        {
            return data->get_type();
        }
    };

    class audio_directory
    {
    public:
        void update(float dt);
        audio_handle_t get_audio_handle_by_id(const uuid& id) const;
        std::vector<audio_handle_t> get_audio_handles() const;
        std::vector<audio_handle_t> get_audio_handles_by_query(const utf8_string& query) const;

        audio_entry& get_entry(audio_handle_t handle);
        // audio_handle_t add_entry(audio_entry&& entry);
        void add_entry(uuid&& id, utf8_string&& title, std::vector<utf8_string>&& tags, std::shared_ptr<audio_data>&& data);

    private:
        std::vector<audio_entry> _entries;
        tag_map _tag_map;
        // map track id to track handle
        std::unordered_map<uuid, audio_handle_t> _id_map;

        // amount of time an unreferenced track waits before unloading data and freeing memory
        constexpr static float unload_age = 10.0f;
    };
    MX_MAKE_SINGLETON(audio_directory)

    // description for how audio is to played in an atmosphere
    struct track_entry
    {
        atmosphere_handle_t atmosphereHandle;
        // TODO: we might be able to replace these with the audio_handle?
        utf8_string audioTitle;
        std::shared_ptr<audio_data> audioData;
        float volume = 1.0f;
        bool muted = false;
        struct
        {
            float min;
            float max;
        } repeat;
        struct
        {
            bool enabled = false;
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
    MX_MAKE_SINGLETON(track_directory)

    struct atmosphere_entry
    {
        // unique id
        uuid id;
        // title
        utf8_string title;
        // volume
        float volume = 1.0f;
        bool muted = false;
        // favorited
        bool favorite = false;
        // tracks
        std::vector<track_handle_t> tracks;
    };

    class atmosphere_directory
    {
    public:
        std::vector<atmosphere_handle_t> get_atmosphere_handles() const;
        std::vector<atmosphere_handle_t> get_atmosphere_handles_by_query(const utf8_string& query) const;

        atmosphere_handle_t get_atmosphere_handle_by_id(const uuid& id) const;
        atmosphere_entry& get_entry(atmosphere_handle_t handle);
        atmosphere_handle_t add_entry(atmosphere_entry&& entry);
        void remove_entry(atmosphere_handle_t handle);
        void rename_entry(atmosphere_handle_t handle, const utf8_string& newName);
    private:
        static atmosphere_handle_t _next_handle;
        std::map<atmosphere_handle_t, atmosphere_entry> _entries;
        tag_map _tag_map;
        // map track id to atmosphere handle
        std::unordered_map<uuid, atmosphere_handle_t> _id_map;
    };
    MX_MAKE_SINGLETON(atmosphere_directory)

    struct playlist_entry
    {
        // unique id (for serialization)
        uuid id;
        // title
        utf8_string title;
        // atmospheres
        std::vector<atmosphere_handle_t> atmospheres;
    };

    class playlist_directory
    {
    public:
        playlist_directory();

        std::vector<playlist_handle_t> get_playlist_handles() const;

        playlist_handle_t create_playlist();
        playlist_entry& get_entry(playlist_handle_t);
        playlist_handle_t add_entry(playlist_entry&&);
        void delete_playlist(playlist_handle_t);
        void add_atmosphere(playlist_handle_t, atmosphere_handle_t);
        void remove_atmosphere(playlist_handle_t, atmosphere_handle_t);
        void favorite_atmosphere(atmosphere_handle_t);
        void unfavorite_atmosphere(atmosphere_handle_t);

        // updates all the playlist and atmospheres in ui
        void sync_all_playlists();
        // update existing playlist contents
        void sync_playlist(playlist_handle_t);
    private:

        playlist_handle_t _next_handle = playlist_handle::first_custom;
        // init with our builtin playlists
        std::map<playlist_handle_t, playlist_entry> _entries =
        {
            {playlist_handle::all,       {uuid(0,static_cast<uint64_t>(playlist_handle::all)),       "All",     {}}},
            {playlist_handle::favorites, {uuid(0,static_cast<uint64_t>(playlist_handle::favorites)), "Starred", {}}},
            {playlist_handle::playing,   {uuid(0,static_cast<uint64_t>(playlist_handle::playing)),   "Playing", {}}},
        };
    };
    MX_MAKE_SINGLETON(playlist_directory)

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
        void play_atmosphere(atmosphere_handle_t);
        void stop_atmosphere(atmosphere_handle_t);
        void set_atmosphere_volume(atmosphere_handle_t, float);
        void set_atmosphere_mute(atmosphere_handle_t, bool);
        void sync_atmosphere(atmosphere_handle_t);

        /// Track methods
        void play_track(track_handle_t);
        void stop_track(track_handle_t);
        void sync_track(track_handle_t);
        void stop_all_tracks();
    private:
        void sync_atmosphere(const atmosphere_entry& entry);
        std::unordered_map<track_handle_t, playback_entry> _entries;
    };
    MX_MAKE_SINGLETON(playback_manager);
}
