#pragma once

namespace dm
{
    template<typename T>
    constexpr sf::Vector3f sfml(const smol::vec3_<T>& v)
    {
        return {static_cast<float>(v.x),static_cast<float>(v.y),static_cast<float>(v.z)};
    }
}

namespace sf {
    std::ostream& operator<<(std::ostream&, const Vector3f&);
}
