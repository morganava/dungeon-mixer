#pragma once

#include "service.hpp"

namespace dm {
    class config;
}

namespace nlohmann
{
    void to_json(nl::json& js, const dm::config&);
    void from_json(const nl::json& js, dm::config&);
}

namespace dm
{
    class config
    {
    public:
        config() = default;
        config(fs::path&& configPath);
    private:
        void set_default_settings();

#define CONFIG_ENTRY(NAME, DEFAULT_VALUE)\
    private:\
        decltype(DEFAULT_VALUE) NAME;\
    public:\
        decltype(DEFAULT_VALUE) const& get_##NAME() const;\
        void set_##NAME(decltype(DEFAULT_VALUE)&&);
#include "config.inl"
#undef CONFIG_ENTRY
    private:
        friend void nlohmann::to_json(nl::json& js, const dm::config&);
        friend void nlohmann::from_json(const nl::json& js, dm::config&);
    };
    MX_MAKE_SERVICE(config, worker_flag::any);
}