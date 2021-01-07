#include "uuid.hpp"
#include "hash.hpp"

size_t std::hash<dm::uuid>::operator()(const dm::uuid& id) const
{
    return calc_hash_raw(&id, sizeof(id));
}