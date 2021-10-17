#include "paths.hpp"

namespace dm
{
    // https://stackoverflow.com/questions/2552416/how-can-i-find-the-users-home-dir-in-a-cross-platform-manner-using-c
	fs::path paths::home()
	{
		static const fs::path home = []() -> fs::path
		{
			if constexpr (current_platform == platform::linux)
			{
				return {std::string(::getenv("HOME"))};
			}
		}();
		return home;
	}

	fs::path paths::temp()
	{
		static const fs::path temp = []() -> fs::path
		{
			if constexpr(current_platform == platform::linux)
			{
				return fs::path("/tmp");
			}
		}();
		return temp;
	}

	fs::path paths::config()
	{
		static const fs::path config = home() / ".dungeonmixer";
		return config;
	}

	fs::path paths::libraries()
	{
		static const fs::path libraries = config() / "libraries";
		return libraries;
	}

	fs::path paths::skins()
	{
		static const fs::path skins = config() / "skins";
		return skins;
	}

	fs::path paths::strings()
	{
		static const fs::path strings = config() / "strings";
		return strings;
	}

	fs::path paths::strings_file()
	{
		static const fs::path strings_file = strings() / "en-US.json";
		return strings_file;
	}

	fs::path paths::playlist_file()
	{
		static const fs::path playlist_file = config() / "playlist.json";
		return playlist_file;
	}
}