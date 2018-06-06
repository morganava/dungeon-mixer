#pragma once

namespace tabletopify
{
    struct uuid
    {
        std::string str() const;
        operator std::string() const;
        bool is_null() const;
        bool operator==(const uuid& that) const;

        union
        {
            uint8_t bytes[16] = {0};
            uint64_t qwords[2];
        };

        const static uuid null;
    };

    void to_json(nl::json& j, const struct uuid& id);
    void from_json(const nl::json& j, struct uuid& id);

    struct uuid_hash
    {
        size_t operator()(const tabletopify::uuid& id) const;
    };
}

std::ostream& operator<<(std::ostream& out, const struct tabletopify::uuid& id);