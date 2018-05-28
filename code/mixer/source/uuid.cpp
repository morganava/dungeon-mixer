#include "uuid.hpp"
#include "utilities.hpp"

namespace tabletopify
{
    constexpr auto uuid_format_string =
        "%02hhx%02hhx%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx";

    const uuid uuid::null = {0};

    std::string uuid::str() const
    {
        std::string result = fmt::sprintf(
            uuid_format_string,
            bytes[0], bytes[1], bytes[2], bytes[3],
            bytes[4], bytes[5], bytes[6], bytes[7],
            bytes[8], bytes[9], bytes[10], bytes[11],
            bytes[12], bytes[13], bytes[14], bytes[15]);

        return result;
    }

    uuid::operator std::string() const
    {
        return str();
    }

    bool uuid::is_null() const
    {
        return qwords[0] == 0 && qwords[1] == 0;
    }

    bool uuid::operator==(const uuid& that) const
    {
        return this->qwords[0] == that.qwords[0] &&
               this->qwords[1] == that.qwords[1];
    }

    void to_json(nl::json& j, const struct uuid& id)
    {
        j = id.str();
    }

    void from_json(const nl::json& j, struct uuid& id)
    {
        MX_ASSERT(j.is_string());
        std::string uuidString = j;
        MX_ASSERT(uuidString.size() == 36);
        MX_ASSERT(16 == sscanf(uuidString.data(),
            uuid_format_string,
            &id.bytes[0], &id.bytes[1], &id.bytes[2], &id.bytes[3],
            &id.bytes[4], &id.bytes[5], &id.bytes[6], &id.bytes[7],
            &id.bytes[8], &id.bytes[9], &id.bytes[10], &id.bytes[11],
            &id.bytes[12], &id.bytes[13], &id.bytes[14], &id.bytes[15]));
    }
}

std::ostream& operator<<(std::ostream& out, const struct tabletopify::uuid& id)
{
    return out << id.str();
}