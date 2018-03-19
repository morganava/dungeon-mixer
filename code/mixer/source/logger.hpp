#pragma once

namespace tabletopify
{
	typedef enum
	{
		LogLevel_Error = 0,
		LogLevel_Info,

		LogLevel_Count,
	} log_level_t;

	void init_logger(log_level_t lvl);
	void log_message(log_level_t lvl,
					 std::chrono::system_clock::time_point now,
					 const char* msg);


	template<typename... Args> inline
	void log(log_level_t lvl, const char* format, const Args&... args)
	{
		auto now = std::chrono::system_clock::now();
		worker(Worker_Logging).enqueue([=]() -> void
		{
			auto msg = string_format(format, std::forward<const Args>(args)...);
			log_message(lvl, now, msg.c_str());
		});
	}

	inline
	void log(log_level_t lvl, const char* msg)
	{
		auto now = std::chrono::system_clock::now();
		worker(Worker_Logging).enqueue([=]() -> void
		{
			log_message(lvl, now, msg);
		});
	}

	inline
	void log(log_level_t lvl, std::string&& msg)
	{
		auto now = std::chrono::system_clock::now();
		worker(Worker_Logging).enqueue([lvl=lvl, now=now, msg=std::forward<std::string>(msg)]() -> void
		{
			log_message(lvl, now, msg.c_str());
		});
	}
}