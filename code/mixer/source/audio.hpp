#pragma once

#include "tag_map.hpp"
#include "singleton.hpp"

namespace dm
{
    // maps to audio data
    SMOL_MAKE_HANDLE_ENUM(audio_handle);
    // instantiated possibly playing audio tracks
    SMOL_MAKE_HANDLE_ENUM(track_handle);
    // collection of tracks
    SMOL_MAKE_HANDLE_ENUM(atmosphere_handle);
    // playlists
    SMOL_MAKE_HANDLE_ENUM(playlist_handle,
        all,
        favorites,
        playing);

    constexpr bool is_builtin_playlist(playlist_handle_t handle)
    {
        switch(handle)
        {
            case playlist_handle::all:
            case playlist_handle::favorites:
            case playlist_handle::playing:
                return true;
            default:
                return false;
        }
    }

    SMOL_MAKE_ENUM(audio_type, uint_fast8_t,
        one_shot,
        loop
    );

    SMOL_MAKE_ENUM(audio_data_state, uint_fast8_t,
        unloaded,
        loading,
        loaded,
        unloading,
        pending_unloaded);

    class audio_data
    {
    public:
        virtual ~audio_data() = default;

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
        std::atomic<audio_data_state_t> _state = audio_data_state::unloaded;

        friend class audio_directory;
    };

    class one_shot_data : public audio_data
    {
    public:
        one_shot_data(std::vector<fs::path>&& paths);

        audio_type_t get_type() override {return audio_type::one_shot;}
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

        audio_type_t get_type() override {return audio_type::loop;}
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
        smol::uuid id;
        // display title
        utf8_string title;
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
        audio_directory();
        void update(float dt);
        audio_handle_t get_audio_handle_by_id(const smol::uuid& id) const;
        std::vector<audio_handle_t> get_audio_handles_by_query(const utf8_string& query) const;

        audio_entry& get_entry(audio_handle_t handle);
        void add_entry(smol::uuid&& id, utf8_string&& title, std::vector<utf8_string>&& tags, std::shared_ptr<audio_data>&& data);

    private:
        std::vector<audio_entry> _entries;
        tag_map<audio_handle_t> _tag_map;
        // map track id to track handle
        std::unordered_map<smol::uuid, audio_handle_t> _id_map;

        // amount of time an unreferenced track waits before unloading data and freeing memory
        constexpr static float unload_age = 10.0f;
    };
    MX_MAKE_SINGLETON(audio_directory, worker_flag::audio);

    // description for how audio is to played in an atmosphere
    struct track_entry
    {
        audio_handle_t audio = audio_handle::invalid;
        double volume = 1.0;
        bool muted = false;
        struct
        {
            bool enabled = false;
            double min = 0.0;
            double max = 15.0;
        } repeat;
        struct
        {
            bool enabled = false;
            double elevation = 0.0;
            double azimuth = 0.0;
        } position;
    };

    class track_directory
    {
    public:
        const track_entry& get_entry(track_handle_t handle);
        track_handle_t add_entry(track_entry&& entry);
        void remove_entry(track_handle_t handle);

        // setters
        void set_title(track_handle_t, utf8_string&& title);
        void set_audio(track_handle_t, std::shared_ptr<audio_data>& audio);
        void set_volume(track_handle_t, double volume);
        void set_muted(track_handle_t, bool muted);
        void set_repeat_enabled(track_handle_t, bool enabled);
        void set_repeat_min(track_handle_t, double min);
        void set_repeat_max(track_handle_t, double max);
        void set_position_enabled(track_handle_t, bool enabled);
        void set_position_elevation(track_handle_t, double elevation);
        void set_position_azimuth(track_handle_t, double azimuth);
    private:
        std::unordered_map<track_handle_t, track_entry> _entries;
    };
    MX_MAKE_SINGLETON(track_directory, worker_flag::audio);

    struct atmosphere_entry
    {
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

        // entry management
        atmosphere_handle_t get_atmosphere_handle_by_track(track_handle_t track) const;
        const atmosphere_entry& get_entry(atmosphere_handle_t handle);
        atmosphere_handle_t add_entry(atmosphere_entry&& entry);
        void remove_entry(atmosphere_handle_t handle);

        // mutators
        void set_title(atmosphere_handle_t, utf8_string&& name);
        void set_volume(atmosphere_handle_t, float volume);
        void set_muted(atmosphere_handle_t, bool muted);
        void set_favorite(atmosphere_handle_t, bool favorite);
        void add_track(atmosphere_handle_t, track_handle_t track);
        void remove_track(atmosphere_handle_t, track_handle_t track);

    private:
        // map tag to atmosphere handle
        tag_map<atmosphere_handle_t> _tag_map;
        // map track handle to atmosphere handle
        std::unordered_map<track_handle_t, atmosphere_handle_t> _track_map;
        // atmoshere_handle_t to entry
        std::map<atmosphere_handle_t, atmosphere_entry> _entries;
    };
    MX_MAKE_SINGLETON(atmosphere_directory, worker_flag::audio)

    struct playlist_entry
    {
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
        std::vector<playlist_handle_t> get_user_playlist_handles() const;

        const playlist_entry& get_entry(playlist_handle_t);
        playlist_handle_t add_entry(playlist_entry&&);
        void remove_entry(playlist_handle_t);

        // playlist mutators
        void set_title(playlist_handle_t, utf8_string&& name);
        void add_atmosphere(playlist_handle_t, atmosphere_handle_t);
        void remove_atmosphere(playlist_handle_t, atmosphere_handle_t);
        void clear_playlist(playlist_handle_t);

        std::vector<atmosphere_handle_t> filtered_atmospheres(playlist_handle_t, const utf8_string& query);
    private:
        // init with our builtin playlists
        std::map<playlist_handle_t, playlist_entry> _entries =
        {
            {playlist_handle::all,       {"All",     {}}},
            {playlist_handle::favorites, {"Starred", {}}},
            {playlist_handle::playing,   {"Playing", {}}},
        };
    };
    MX_MAKE_SINGLETON(playlist_directory, worker_flag::audio)

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
        playback_manager();
        void update(float dt);

        /// Atmosphere methods
        void play_atmosphere(atmosphere_handle_t);
        void stop_atmosphere(atmosphere_handle_t);
        void sync_atmosphere(atmosphere_handle_t);

        /// Track methods
        void play_track(track_handle_t);
        void stop_track(track_handle_t);
        void sync_track(track_handle_t);
        void stop_all_tracks();
    private:
        void sync_track(const atmosphere_entry&, track_handle_t);
        std::unordered_map<track_handle_t, playback_entry> _entries;
    };
    MX_MAKE_SINGLETON(playback_manager, worker_flag::audio);
}
