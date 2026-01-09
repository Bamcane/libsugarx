#ifndef LIBSUGARX_UUID
#define LIBSUGARX_UUID

#include <array>
#include "sugar_string.h"

namespace libsugarx
{
    using uuid_string = fixed_string<37>;

    class uuid
    {
        std::array<std::byte, 16> data{};

        void set_version_and_variant(int version)
        {
            data[6] = std::byte((static_cast<unsigned char>(data[6]) & 0x0F) | (version << 4));
            data[8] = std::byte((static_cast<unsigned char>(data[8]) & 0x3F) | 0x80);
        }

        void from_string(uuid_string str);
    public:
        uuid() = default;
        uuid(uuid_string str) { from_string(str); }

        std::array<std::byte, 16> &raw_data() { return data; }
        const std::array<std::byte, 16> &raw_data() const { return data; }

        static uuid generate_v1() = delete;
        static uuid generate_v2() = delete;
        // MD5 based
        static uuid generate_v3(std::string_view name, uuid name_space);
        // random
        static uuid generate_v4();
        // Sha256 based
        static uuid generate_v5(std::string_view name, uuid name_space);
        static uuid generate_v6() = delete;
        // timestamp + random
        static uuid generate_v7();

        uuid_string to_string() const;

        auto operator<=>(const uuid &other) const = default;
    };
}

namespace std
{
    template<>
    struct formatter<libsugarx::uuid, char> : formatter<libsugarx::uuid_string, char>
    {
        auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
        {
            return formatter<libsugarx::uuid_string, char>::parse(ctx);
        }

        template<typename FormatContext>
        auto format(const libsugarx::uuid& uuid, FormatContext& ctx) const -> decltype(ctx.out())
        {
            return formatter<libsugarx::uuid_string, char>::format(uuid.to_string(), ctx);
        }
    };
}

#endif // LIBSUGARX_UUID

#ifdef LIBSUGARX_UUID_IMPLEMENTATION

#include <chrono>

#define OPENSSL_API_COMPAT 0x10101000L
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/rand.h>

namespace libsugarx
{
    uuid uuid::generate_v3(std::string_view name, uuid name_space)
    {
        uuid result;
        MD5_CTX ctx;
        std::array<std::byte, MD5_DIGEST_LENGTH> digest;

        MD5_Init(&ctx);
        MD5_Update(&ctx, name_space.data.data(), name_space.data.size());
        MD5_Update(&ctx, name.data(), name.size());
        MD5_Final(reinterpret_cast<unsigned char *>(digest.data()), &ctx);

        for(int i = 0; i < result.data.size(); i++)
            result.data[i] = digest[i];

        result.set_version_and_variant(3);
        return result;
    }

    uuid uuid::generate_v4()
    {
        uuid result;
        if(RAND_bytes(reinterpret_cast<unsigned char *>(result.data.data()), result.data.size()) != 1)
            throw std::runtime_error("uuid generate error");
        result.set_version_and_variant(4);
        return result;
    }

    uuid uuid::generate_v5(std::string_view name, uuid name_space)
    {
        uuid result;
        SHA_CTX ctx;
        std::array<std::byte, SHA_DIGEST_LENGTH> digest;

        SHA1_Init(&ctx);
        SHA1_Update(&ctx, name_space.data.data(), name_space.data.size());
        SHA1_Update(&ctx, name.data(), name.size());
        SHA1_Final(reinterpret_cast<unsigned char *>(digest.data()), &ctx);

        for(int i = 0; i < result.data.size(); i++)
            result.data[i] = digest[i];
        result.set_version_and_variant(5);
        return result;
    }

    uuid uuid::generate_v7()
    {
        uuid result;
        int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::utc_clock::now().time_since_epoch()).count();

        uint64_t timestamp = static_cast<uint64_t>(now_ms) & 0xFFFFFFFFFFFFULL; // 48 bits
        result.data[0] = std::byte((timestamp >> 40) & 0xFF);
        result.data[1] = std::byte((timestamp >> 32) & 0xFF);
        result.data[2] = std::byte((timestamp >> 24) & 0xFF);
        result.data[3] = std::byte((timestamp >> 16) & 0xFF);
        result.data[4] = std::byte((timestamp >> 8)  & 0xFF);
        result.data[5] = std::byte(timestamp & 0xFF);

        if(RAND_bytes(reinterpret_cast<unsigned char *>(result.data.data() + 6), 10) != 1)
            throw std::runtime_error("uuid generate error");

        result.set_version_and_variant(7);
        return result;
    }

    static int hex_char_to_int(char c)
    {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }

    void uuid::from_string(uuid_string str)
    {
        if(str.length() != 36)
            throw std::invalid_argument("invalid uuid string length");

        if (str[8] != '-' || str[13] != '-' || str[18] != '-' || str[23] != '-')
            throw std::invalid_argument("invalid uuid format: missing hyphens");

        std::array<char, 32> hex_chars{};
        size_t out_idx = 0;
        for(size_t i = 0; i < 36; ++i)
        {
            if(str[i] == '-') continue;
            if(out_idx >= 32) throw std::invalid_argument("too many hex digits");
            hex_chars[out_idx++] = str[i];
        }

        if(out_idx != 32) throw std::invalid_argument("too few hex digits");

        for(size_t i = 0; i < 16; ++i)
        {
            int high = hex_char_to_int(hex_chars[i * 2]);
            int low  = hex_char_to_int(hex_chars[i * 2 + 1]);
            if(high == -1 || low == -1)
                throw std::invalid_argument("invalid hex character in uuid");

            data[i] = std::byte(static_cast<unsigned char>((high << 4) | low));
        }
    }

    uuid_string uuid::to_string() const
    {
        uuid_string str;
        static const char hex_digits[] = "0123456789abcdef";

        str.clear();
        auto append_byte = [&](std::byte b)
        {
            unsigned char c = static_cast<unsigned char>(b);
            str.concat(hex_digits[c >> 4]);
            str.concat(hex_digits[c & 0x0F]);
        };

        append_byte(data[0]); append_byte(data[1]); append_byte(data[2]); append_byte(data[3]);
        str.concat('-');
        append_byte(data[4]); append_byte(data[5]);
        str.concat('-');
        append_byte(data[6]); append_byte(data[7]);
        str.concat('-');
        append_byte(data[8]); append_byte(data[9]);
        str.concat('-');
        append_byte(data[10]); append_byte(data[11]); append_byte(data[12]);
        append_byte(data[13]); append_byte(data[14]); append_byte(data[15]);
        return str;
    }
};

#endif