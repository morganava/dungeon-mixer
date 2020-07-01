#include "uuid.hpp"
#include "hash.hpp"

size_t std::hash<tabletopify::uuid>::operator()(const tabletopify::uuid& id) const
{
    return calc_hash_raw(&id, sizeof(id));
}