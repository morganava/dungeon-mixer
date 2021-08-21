#include <smol/geometry.hpp>

using namespace smol;
namespace nlohmann
{
    void from_json(const nl::json& j, vec2& v)
    {
        v = vec2::zero;
        if (j.is_object())
        {
            if(auto it = j.find("x"); it != j.end())
            {
                SMOL_THROW_IF_FALSE(it->is_number());
                v.x = it->get<double>();
            }
            if(auto it = j.find("y"); it != j.end())
            {
                SMOL_THROW_IF_FALSE(it->is_number());
                v.y = it->get<double>();
            }
        }
        else if(j.is_array())
        {
            SMOL_THROW_IF_FALSE(j.size() == 2);
            SMOL_THROW_IF_FALSE(j[0].is_number());
            SMOL_THROW_IF_FALSE(j[1].is_number());
            v.x = j[0].get<double>();
            v.y = j[1].get<double>();
        }
    }

    void from_json(const nl::json& j, vec3& v)
    {
        v = vec3::zero;
        if (j.is_object())
        {
            if(auto it = j.find("x"); it != j.end())
            {
                SMOL_THROW_IF_FALSE(it->is_number());
                v.x = it->get<double>();
            }
            if(auto it = j.find("y"); it != j.end())
            {
                SMOL_THROW_IF_FALSE(it->is_number());
                v.y = it->get<double>();
            }
            if(auto it = j.find("z"); it != j.end())
            {
                SMOL_THROW_IF_FALSE(it->is_number());
                v.z = it->get<double>();
            }
        }
        else if(j.is_array())
        {
            SMOL_THROW_IF_FALSE(j.size() == 3);
            SMOL_THROW_IF_FALSE(j[0].is_number());
            SMOL_THROW_IF_FALSE(j[1].is_number());
            SMOL_THROW_IF_FALSE(j[2].is_number());

            v.x = j[0].get<double>();
            v.y = j[1].get<double>();
            v.z = j[2].get<double>();
        }
    }

    void from_json(const nl::json& j, corners& c)
    {
        if (j.is_object())
        {
            if(auto it = j.find("topLeft"); it != j.end())
            {
                SMOL_THROW_IF_FALSE(it->is_number());
                c.topLeft = it->get<double>();
            }
            if(auto it = j.find("topRight"); it != j.end())
            {
                SMOL_THROW_IF_FALSE(it->is_number());
                c.topRight = it->get<double>();
            }
            if(auto it = j.find("bottomRight"); it != j.end())
            {
                SMOL_THROW_IF_FALSE(it->is_number());
                c.bottomRight = it->get<double>();
            }
            if(auto it = j.find("bottomLeft"); it != j.end())
            {
                SMOL_THROW_IF_FALSE(it->is_number());
                c.bottomLeft = it->get<double>();
            }
        }
        else if(j.is_array())
        {
            SMOL_THROW_IF_FALSE(j.size() == 4);
            for(size_t i = 0; i < 4; ++i)
            {
                const auto& j_i = j[i];
                SMOL_THROW_IF_FALSE(j_i.is_number());
                c.raw[i] = j_i.get<double>();
            }
        }
        else if(j.is_number())
        {
            const auto val = j.get<double>();
            for(size_t i = 0; i < 4; ++i)
            {
                c.raw[i] = val;
            }
        }
    }

    void from_json(const nl::json& j, edges& e)
    {
        if (j.is_object())
        {
            if(auto it = j.find("top"); it != j.end())
            {
                SMOL_THROW_IF_FALSE(it->is_number());
                e.top = it->get<double>();
            }
            if(auto it = j.find("right"); it != j.end())
            {
                SMOL_THROW_IF_FALSE(it->is_number());
                e.right = it->get<double>();
            }
            if(auto it = j.find("bottom"); it != j.end())
            {
                SMOL_THROW_IF_FALSE(it->is_number());
                e.bottom = it->get<double>();
            }
            if(auto it = j.find("left"); it != j.end())
            {
                SMOL_THROW_IF_FALSE(it->is_number());
                e.left = it->get<double>();
            }
        }
        else if(j.is_array())
        {
            SMOL_THROW_IF_FALSE(j.size() == 4);
            for(size_t i = 0; i < 4; ++i)
            {
                const auto& j_i = j[i];
                SMOL_THROW_IF_FALSE(j_i.is_number());
                e.raw[i] = j_i.get<double>();
            }
        }
        else if(j.is_number())
        {
            const auto val = j.get<double>();
            for(size_t i = 0; i < 4; ++i)
            {
                e.raw[i] = val;
            }
        }
    }
}