#include "color.hpp"

namespace tabletopify
{
  #define COLOR_ENTRY(NAME, RGB) const color color::NAME = color::RGB;
  #include "color.inl"
  #undef COLOR_ENTRY
}