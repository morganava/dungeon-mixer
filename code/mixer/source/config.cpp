#include "config.hpp"
#include "work_queue.hpp"
#include "logger.hpp"

using smol::utf8_string;

namespace dm
{
    config::config(fs::path&& path)
    {
        try
        {
            auto bytes = smol::load_file(path);
            nl::json configJson = smol::parse_json(bytes);
            auto parsedConfig = configJson.get<config>();
            *this = std::move(parsedConfig);
        }
        catch(...)
        {
            this->set_default_settings();
        }
    }

    utf8_string config::sanitize_log_filename(const utf8_string& logFilename)
    {
        auto path = fs::path(logFilename.c_str());
        return path.filename().string();
    }

    log_level_t config::sanitize_log_level(const log_level_t& level)
    {
        return level;
    }

    utf8_string config::sanitize_skin(const utf8_string& skin)
    {
        return skin;
    }

    void config::set_default_settings()
    {
        #define CONFIG_ENTRY(NAME, DEFAULT_VALUE)\
        this->NAME = std::move(DEFAULT_VALUE);

        #include "config.inl"
        #undef CONFIG_ENTRY
    }

    #define CONFIG_ENTRY(NAME, DEFAULT_VALUE)\
    decltype(DEFAULT_VALUE) const& config::get_##NAME() const {\
        return this->NAME;\
    }\
    void config::set_##NAME(const decltype(DEFAULT_VALUE)& val) {\
        auto sanitizedVal = config::sanitize_##NAME(val);\
        MX_LOG_INFO("Setting " #NAME " = \"{}\"", sanitizedVal);\
        this->NAME = std::move(sanitizedVal);\
    }
    #include "config.inl"
    #undef CONFIG_ENTRY
}

namespace nlohmann
{
    void to_json(nl::json& js, const dm::config& self)
    {
        nl::json retval = nl::json::object();
        #define CONFIG_ENTRY(NAME, DEFAULT_VALUE)\
        nl::to_json(retval[#NAME], self.NAME);
        #include "config.inl"
        #undef CONFIG_ENTRY
        js = std::move(retval);
    }

    void from_json(const nl::json& js, dm::config& self)
    {
        SMOL_THROW_IF_FALSE(js.is_object());
        dm::config retval;
        #define CONFIG_ENTRY(NAME, DEFAULT_VALUE)\
        if (auto it = js.find(#NAME); it != js.end()) {\
            decltype(retval.NAME) intermediate;\
            try {\
                nl::from_json(*it, intermediate);\
                retval.NAME = dm::config::sanitize_##NAME(intermediate);\
            } catch(...) {\
                retval.NAME = std::move(DEFAULT_VALUE);\
            }\
        } else {\
            retval.NAME = std::move(DEFAULT_VALUE);\
        }

        #include "config.inl"
        #undef CONFIG_ENTRY
        self = std::move(retval);
    }
}
