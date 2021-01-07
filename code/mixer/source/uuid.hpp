#pragma once

#include "utilities.hpp"

namespace dm
{
    struct uuid
    {
        constexpr static char null_uuid_string[] = "00000000-0000-0000-0000-000000000000";
        constexpr static const char format_string[] =
            "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}";

        constexpr uuid()
        { }

        constexpr uuid(const char* str)
        {
            MX_THROW_IF_FALSE(is_valid_uuid(str));

            auto char_to_hex = [](char c) -> uint8_t
            {
                if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
                if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
                else return static_cast<uint8_t>(c - 'A' + 10);
            };

            size_t byteIndex = 0;
            for(size_t index = 0; index < sizeof(null_uuid_string); index++)
            {
                switch(index)
                {
                    case dash0:
                    case dash1:
                    case dash2:
                    case dash3:
                    case null_terminator:
                        break;
                    default:
                        const auto& hi = str[index++];
                        const auto& lo = str[index];
                        const auto byte = char_to_hex(hi) * 16 + char_to_hex(lo);
                        bytes[byteIndex++] = byte;
                        break;
                }
            }
        }

        uuid(const std::string& str)
        : uuid(str.c_str())
        { }

        constexpr uuid(const uuid& that) = default;
        constexpr uuid(uuid&& that) = default;
        constexpr uuid& operator=(const uuid&) = default;
        constexpr uuid& operator=(uuid&&) = default;

        constexpr bool is_null() const
        {
            return std::all_of(std::begin(bytes), std::end(bytes), [](uint8_t b){return b == 0;});
        }

        constexpr bool operator==(const uuid& that) const
        {
            return std::equal(bytes, bytes + sizeof(bytes),
                              that.bytes, that.bytes + 16);
        }

        constexpr bool static is_valid_uuid(const char * str)
        {
            auto is_hex_digit = [](char c) -> bool
            {
                return (c >= '0' && c <= '9') ||
                       (c >= 'a' && c <= 'f') ||
                       (c >= 'A' && c <= 'F');
            };

            for(size_t index = 0; index < sizeof(null_uuid_string); index++)
            {
                const auto& byte = str[index];
                switch(index)
                {
                    case dash0:
                    case dash1:
                    case dash2:
                    case dash3:
                        if (byte != '-') return false;
                        break;
                    case null_terminator:
                        if (byte != 0) return false;
                        break;
                    default:
                        if (!is_hex_digit(byte)) return false;
                        break;
                }
            }

            return true;
        }

        uint8_t bytes[16] = {0};
    private:
        constexpr static auto time_low = 8;
        constexpr static auto time_mid = 4;
        constexpr static auto time_hi_and_version = 4;
        constexpr static auto clock_seq = 4;
        constexpr static auto node = 12;

        static_assert(sizeof(null_uuid_string) == (time_low + 1 + time_mid + 1 + time_hi_and_version + 1 + clock_seq + 1 + node + 1));

        constexpr static auto dash0 = time_low;
        constexpr static auto dash1 = dash0 + 1 + time_mid;
        constexpr static auto dash2 = dash1 + 1 + time_hi_and_version;
        constexpr static auto dash3 = dash2 + 1 + clock_seq;
        constexpr static auto null_terminator = dash3 + 1 + node;

        static_assert(null_terminator == (sizeof(null_uuid_string) - 1));

    };

    void to_json(nl::json& j, const struct uuid& id);
    void from_json(const nl::json& j, struct uuid& id);
}

std::ostream& operator<<(std::ostream& out, const struct dm::uuid& id);
template <> struct std::hash<dm::uuid>
{
    size_t operator()(const dm::uuid&) const;
};