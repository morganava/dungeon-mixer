#include "sfml.hpp"

using namespace std;
ostream& operator<<(ostream& out, const sf::Vector3f& vec)
{
    return out << "{ x: " << vec.x << ", y: " << vec.y << ", z: " << vec.z << " }";
}

