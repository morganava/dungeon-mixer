#include <smol/uuid.hpp>

namespace smol
{
    constexpr static const char uuid_format_string[] =
        "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}";


    size_t calc_hash(const uuid& id, size_t seed)
    {
        return calc_hash_raw(&id, sizeof(id), seed);
    }

    std::ostream& operator<<(std::ostream& out, const struct uuid& id)
    {
        const auto& bytes = id.bytes;

        fmt::print(out, uuid_format_string,
            bytes[0], bytes[1], bytes[2], bytes[3],
            bytes[4], bytes[5], bytes[6], bytes[7],
            bytes[8], bytes[9], bytes[10], bytes[11],
            bytes[12], bytes[13], bytes[14], bytes[15]);

        return out;
    }
}

namespace nlohmann
{
    void to_json(json& j, const smol::uuid& id)
    {
        const auto& bytes = id.bytes;
        j = fmt::format(
            smol::uuid_format_string,
            bytes[0], bytes[1], bytes[2], bytes[3],
            bytes[4], bytes[5], bytes[6], bytes[7],
            bytes[8], bytes[9], bytes[10], bytes[11],
            bytes[12], bytes[13], bytes[14], bytes[15]);
    }

    void from_json(const json& j, smol::uuid& id)
    {
        SMOL_THROW_IF_FALSE(j.is_string());

        id = smol::uuid(j.get<std::string>());
    }
}
