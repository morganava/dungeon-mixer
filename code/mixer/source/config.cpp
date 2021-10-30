#include "config.hpp"
#include "work_queue.hpp"
#include "logger.hpp"

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
    void config::set_##NAME(decltype(DEFAULT_VALUE)&& val) {\
        this->NAME = std::move(val);\
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
            nl::from_json(*it, retval.NAME);\
        } else {\
            retval.NAME = std::move(DEFAULT_VALUE);\
        }
        #include "config.inl"
        #undef CONFIG_ENTRY
        self = std::move(retval);
    }
}
