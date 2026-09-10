#ifndef LIBSUGARX_HASH_H
#define LIBSUGARX_HASH_H

#include <array>
#include <cstddef>
#include <cstdint>

#include "sugar_types.h"

namespace libsugarx::hash
{
	/*
		Standard CRC-32 (IEEE 802.3, reflected polynomial 0xEDB88320), equivalent
		to zlib crc32(0, data, size). Self contained and constexpr.
	*/
	[[nodiscard]]
	constexpr std::uint32_t crc32(const_data_span Data) noexcept
	{
		std::uint32_t Crc = 0xFFFFFFFFu;
		for(const std::byte Byte : Data)
		{
			Crc ^= static_cast<std::uint32_t>(std::to_integer<unsigned char>(Byte));
			for(int Bit = 0; Bit < 8; ++Bit)
				Crc = (Crc >> 1) ^ (0xEDB88320u & (0u - (Crc & 1u)));
		}
		return Crc ^ 0xFFFFFFFFu;
	}

	// SHA-256 over the whole buffer
	std::array<std::byte, 32> sha256(const_data_span Data);
} // namespace libsugarx::hash

#endif // LIBSUGARX_HASH_H

#ifdef LIBSUGARX_HASH_IMPL

#include <openssl/evp.h>

namespace libsugarx::hash
{
	std::array<std::byte, 32> sha256(const_data_span Data)
	{
		std::array<std::byte, 32> Digest{};
		unsigned int Length = 0;
		EVP_Digest(Data.data(), Data.size(), reinterpret_cast<unsigned char *>(Digest.data()), &Length, EVP_sha256(), nullptr);
		return Digest;
	}
} // namespace libsugarx::hash

#endif
