#ifndef LIBSUGARX_ENDIAN_H
#define LIBSUGARX_ENDIAN_H

#include <bit>
#include <concepts>

namespace libsugarx
{
	consteval bool is_big_endian()
	{
		return std::endian::native == std::endian::big;
	}

	consteval bool is_little_endian()
	{
		return std::endian::native == std::endian::little;
	}

	consteval bool is_mixed_endian()
	{
		return !is_big_endian() && !is_little_endian();
	}

	static_assert(!is_mixed_endian(), "mixed-endian platforms are not supported.");

	template<std::integral T>
	constexpr T to_big_endian(T value)
	{
		if constexpr(is_little_endian())
			return std::byteswap(value);
		return value;
	}

	template<std::integral T>
	constexpr T to_little_endian(T value)
	{
		if constexpr(is_big_endian())
			return std::byteswap(value);
		return value;
	}
} // namespace libsugarx

#endif // LIBSUGARX_ENDIAN_H