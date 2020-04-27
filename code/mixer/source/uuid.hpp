#pragma once

namespace tabletopify
{
    struct uuid
    {
        constexpr uuid()
        : qwords{0,0}
        { }

        constexpr uuid(uint64_t hi, uint64_t lo)
        : qwords{hi,lo}
        { }
        constexpr uuid(const uuid& that) = default;
        constexpr uuid(uuid&& that) = default;
        constexpr uuid& operator=(const uuid&) = default;
        constexpr uuid& operator=(uuid&&) = default;

        constexpr bool is_null() const
        {
            return qwords[0] == 0 && qwords[1] == 0;
        }

        constexpr bool operator==(const uuid& that) const
        {
            return this->qwords[0] == that.qwords[0] &&
                   this->qwords[1] == that.qwords[1];
        }

        union
        {
            uint8_t bytes[16];
            uint64_t qwords[2];
        };

        constexpr static size_t string_length = sizeof("00000000-0000-0000-0000-000000000000") - 1;
        constexpr static const char format_string[] =
            "%02hhx%02hhx%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx";
    };

    void to_json(nl::json& j, const struct uuid& id);
    void from_json(const nl::json& j, struct uuid& id);
}

std::ostream& operator<<(std::ostream& out, const struct tabletopify::uuid& id);
template <> struct std::hash<class tabletopify::uuid>
{
    size_t operator()(const class tabletopify::uuid&) const;
};
