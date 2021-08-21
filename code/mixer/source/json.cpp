#include "serialization.hpp"
#include "paths.hpp"
#include "stylesheet.hpp"
#include "nanovg.hpp"
#include "linear_constraint_solver.hpp"
#include "logger.hpp"

using namespace dm;

namespace nlohmann
{
    // style_resolver



    void from_json(const nl::json& js, constraint& out)
    {
        const auto parse_min = [](const nl::json& js) -> double
        {
            if (js.is_string())
            {
                auto str = js.get<std::string>();
                SMOL_THROW_IF_FALSE(str == "none");
                return 0.0;
            }
            SMOL_THROW_IF_FALSE(js.is_number());
            auto retval = js.get<double>();
            SMOL_THROW_IF_FALSE(retval >= 0.0);
            return retval;
        };

        const auto parse_max = [](const nl::json& js) -> double
        {
            if (js.is_string())
            {
                auto str = js.get<std::string>();
                SMOL_THROW_IF_FALSE(str == "none");
                return std::numeric_limits<double>::max();
            }
            SMOL_THROW_IF_FALSE(js.is_number());
            auto retval = js.get<double>();
            SMOL_THROW_IF_FALSE(retval >= 0.0);
            return retval;
        };

        const auto parse_scale = [](const nl::json& js) -> double
        {
            SMOL_THROW_IF_FALSE(js.is_number());
            auto retval = js.get<double>();
            SMOL_THROW_IF_FALSE(retval > 0.0);
            return retval;
        };

        constraint retval;

        if (js.is_array())
        {
            // purposeful fallthrough
            switch(js.size())
            {
                default:
                    static_assert(std::is_unsigned<decltype(js.size())>::value);
                case 3:
                    retval.ratio = parse_scale(js[2]);
                case 2:
                    retval.max = parse_max(js[1]);
                case 1:
                    retval.min = parse_min(js[0]);
                case 0:
                    break;
            }
        }
        else if (js.is_object())
        {
            if (auto it = js.find("ratio"); it != js.end())
            {
                retval.ratio = parse_scale(*it);
            }
            if (auto it = js.find("max"); it != js.end())
            {
                retval.max = parse_max(*it);
            }
            if (auto it = js.find("min"); it != js.end())
            {
                retval.min = parse_min(*it);
            }
        }
        else
        {
            SMOL_THROW_MSG("invalid constraint json");
        }

        MX_LOG_INFO("constraint {{ min : {}, max : {}, ratio : {} }}", retval.min, retval.max, retval.ratio);

        out = retval;
    }

    void from_json(const nl::json& js, linear_constraint_solver& out)
    {
        SMOL_THROW_IF_FALSE(js.is_array());
        auto constraints = js.get<smol::array<dm::constraint>>();
        out = {std::move(constraints)};
    }
}