#include "sfml.hpp"

namespace sf {
    std::ostream& operator<<(std::ostream& out, const Vector3f& vec)
    {
        return out << "{ x: " << vec.x << ", y: " << vec.y << ", z: " << vec.z << " }";
    }
}

