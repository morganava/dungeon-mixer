#pragma once

namespace dm
{
	class paths
	{
	public:
		static fs::path home();
		static fs::path config();
		static fs::path temp();
		static fs::path libraries();
		static fs::path skins();
		static fs::path strings();

		static fs::path strings_file();
		static fs::path playlist_file();
	};
}